/*
 * ebusd - daemon for communication with eBUS heating systems.
 * Copyright (C) 2015-2017 John Baier <ebusd@ebusd.eu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "lib/ebus/device.h"

#ifdef _WIN32
#include <Windows.h>
#include <Ws2tcpip.h>
typedef u_short in_port_t;
#define strdup _strdup
#endif

#include <fcntl.h>
#ifndef _WIN32
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif
#include <errno.h>
#ifdef HAVE_PPOLL
#  include <poll.h>
#endif
#include <cstdlib>
#include <cstring>
#include <fstream>
#include "lib/ebus/data.h"

namespace ebusd {

#ifdef _WIN32
	int inet_aton(const char *addr, struct in_addr*in);
#endif

Device::~Device() {
  close();
}

Device* Device::create(const char* name, bool checkDevice, bool readOnly, bool initialSend) {
  if (strchr(name, '/') == NULL && strchr(name, ':') != NULL) {
    char* in = strdup(name);
    bool udp = false;
    char* addrpos = in;
    char* portpos = strchr(addrpos, ':');
    if (portpos == addrpos+3 && (strncmp(addrpos, "tcp", 3) == 0 || (udp=(strncmp(addrpos, "udp", 3) == 0)))) {
      addrpos += 4;
      portpos = strchr(addrpos, ':');
    }
    if (portpos == NULL) {
      free(in);
      return NULL;  // invalid protocol or missing port
    }
    result_t result = RESULT_OK;
    unsigned int port = parseInt(portpos+1, 10, 1, 65535, &result);
    if (result != RESULT_OK) {
      free(in);
      return NULL;  // invalid port
    }
    struct sockaddr_in address;
    memset(reinterpret_cast<char*>(&address), 0, sizeof(address));
    *portpos = 0;
    if (inet_aton(addrpos, &address.sin_addr) == 0) {
      struct hostent* h = gethostbyname(addrpos);
      if (h == NULL) {
        free(in);
        return NULL;  // invalid host
      }
      memcpy(&address.sin_addr, h->h_addr_list[0], h->h_length);
    }
    free(in);
    address.sin_family = AF_INET;
    address.sin_port = (in_port_t)htons((uint16_t)port);
    return new NetworkDevice(name, address, readOnly, initialSend, udp);
  }
  return new SerialDevice(name, checkDevice, readOnly, initialSend);
}


void Device::close() {
	m_isOpen = false;
}

bool Device::isValid() {
	if (!m_isOpen) {
    return false;
  }
  if (m_checkDevice) {
    checkDevice();
  }
  return m_isOpen;
}

result_t Device::send(symbol_t value) {
  if (!isValid()) {
    return RESULT_ERR_DEVICE;
  }
  if (m_readOnly || write(value) != 1) {
    return RESULT_ERR_SEND;
  }
  if (m_listener != NULL) {
    m_listener->notifyDeviceData(value, false);
  }
  return RESULT_OK;
}

result_t Device::recv(unsigned int timeout, symbol_t* value) {
  if (!isValid()) {
    return RESULT_ERR_DEVICE;
  }
  if (!available() && timeout > 0) {
    int ret;
#if HAVE_PPOLL || HAVE_PSELECT
	struct timespec tdiff;

    // set select timeout
    tdiff.tv_sec = timeout/1000000;
    tdiff.tv_nsec = (timeout%1000000)*1000;
#endif
#ifdef HAVE_PPOLL
    nfds_t nfds = 1;
    struct pollfd fds[nfds];

    memset(fds, 0, sizeof(fds));

    fds[0].fd = m_fd;
    fds[0].events = POLLIN;

    ret = ppoll(fds, nfds, &tdiff, NULL);
#else
#ifdef HAVE_PSELECT
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(m_fd, &readfds);

    ret = pselect(m_fd + 1, &readfds, NULL, NULL, &tdiff, NULL);
#else
    ret = 1;  // ignore timeout if neither ppoll nor pselect are available
#endif
#endif
    if (ret == -1) {
      return RESULT_ERR_DEVICE;
    }
    if (ret == 0) {
      return RESULT_ERR_TIMEOUT;
    }
  }

  // directly read byte from device
  ssize_t nbytes = read(*value);
  if (nbytes == 0) {
    return RESULT_ERR_EOF;
  }
  if (nbytes < 0) {
    return RESULT_ERR_DEVICE;
  }
  if (m_listener != NULL) {
    m_listener->notifyDeviceData(*value, true);
  }
  return RESULT_OK;
}


result_t SerialDevice::open() {
  if (m_isOpen) {
    close();
  }

#ifdef _WIN32
  char comName[100];
  strcpy_s(comName, sizeof(comName), "\\\\.\\");
  strncat_s(comName, sizeof(comName), m_name, sizeof(comName));
  m_fh = ::CreateFileA(comName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
  if ( m_fh == NULL){
	  return RESULT_ERR_NOTFOUND;
  }
  DCB comDCB = {0};
  comDCB.DCBlength = sizeof(comDCB);
  if (!GetCommState(m_fh, &comDCB))
  {
	  close();
	  return RESULT_ERR_DEVICE;
  }
  comDCB.BaudRate = CBR_2400;
  comDCB.ByteSize = 8;
  comDCB.Parity = NOPARITY;
  comDCB.StopBits = ONESTOPBIT;
  comDCB.fParity = false;
  comDCB.fBinary = true;
  comDCB.fNull = false;
  comDCB.fInX = false;
  comDCB.fOutX = false;
  comDCB.fDsrSensitivity = false;
  comDCB.fDtrControl = false;
  comDCB.fOutxCtsFlow = false;
  comDCB.fOutxDsrFlow = false;
  comDCB.fDtrControl = DTR_CONTROL_ENABLE;
  comDCB.fRtsControl = RTS_CONTROL_ENABLE;

  SetCommState(m_fh, &comDCB);



#else
  struct termios newSettings;

  // open file descriptor
  m_fd = ::open(m_name, O_RDWR | O_NOCTTY);

  if (m_fd < 0) {
    return RESULT_ERR_NOTFOUND;
  }
  if (isatty(m_fd) == 0) {
    close();
    return RESULT_ERR_NOTFOUND;
  }

  if (flock(m_fd, LOCK_EX|LOCK_NB)) {
    close();
    return RESULT_ERR_DEVICE;
  }

  // save current settings
  tcgetattr(m_fd, &m_oldSettings);

  // create new settings
  memset(&newSettings, '\0', sizeof(newSettings));

  newSettings.c_cflag |= (B2400 | CS8 | CLOCAL | CREAD);
  newSettings.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  // non-canonical mode
  newSettings.c_iflag |= IGNPAR;  // ignore parity errors
  newSettings.c_oflag &= ~OPOST;

  // non-canonical mode: read() blocks until at least one byte is available
  newSettings.c_cc[VMIN]  = 1;
  newSettings.c_cc[VTIME] = 0;

  // empty device buffer
  tcflush(m_fd, TCIFLUSH);

  // activate new settings of serial device
  tcsetattr(m_fd, TCSAFLUSH, &newSettings);

  // set serial device into blocking mode
  fcntl(m_fd, F_SETFL, fcntl(m_fd, F_GETFL) & ~O_NONBLOCK);

#endif
  if (m_initialSend && write(ESC) != 1) {
	  return RESULT_ERR_SEND;
  }
  m_isOpen = true;
  return RESULT_OK;
}

void SerialDevice::close()
{
#ifdef _WIN32
	if ( m_fh != NULL)
	{
		CloseHandle(m_fh);
		m_fh = NULL;
	}
#else
  if (m_fd != -1) {
    // empty device buffer
    tcflush(m_fd, TCIOFLUSH);

    // restore previous settings of the device
    tcsetattr(m_fd, TCSANOW, &m_oldSettings);
	::close(m_fd);
	m_fd = -1;
  }
#endif
  Device::close();
}

ssize_t SerialDevice::write(const symbol_t value)
{
#ifdef _WIN32
	DWORD w;
	if (::WriteFile(m_fh, &value, 1, &w, NULL )) {
		return w;
	}
	return 0;
#else
	return ::write(m_fd, &value, 1);
#endif
}

ssize_t SerialDevice::read(symbol_t& value)
{
#ifdef _WIN32
	DWORD w;
	if (::ReadFile(m_fh, &value, 1, &w, NULL)) {
		return w;
	}
	return 0;
#else
	return ::read(m_fd, &value, 1);
#endif
}


void SerialDevice::checkDevice() {
#ifdef _WIN32
	DWORD evt;
	if (!GetCommMask(m_fh, &evt)) {
		close();
	}
#else
  int port;
  if (ioctl(m_fd, TIOCMGET, &port) == -1) {
    close();
  }
#endif
}


result_t NetworkDevice::open() {
  if (m_isOpen) {
    close();
  }
  m_fs = socket(AF_INET, m_udp ? SOCK_DGRAM : SOCK_STREAM, 0);
  if (m_fs < 0) {
    return RESULT_ERR_GENERIC_IO;
  }
  int ret;
  if (m_udp) {
    struct sockaddr_in address = m_address;
    address.sin_addr.s_addr = INADDR_ANY;
    ret = bind(m_fs, (struct sockaddr*)&address, sizeof(address));
  } else {
    int value = 1;
#ifdef _WIN32
    ret = setsockopt(m_fs, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&value), sizeof(value));
#else
	ret = setsockopt(m_fs, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<void*>(&value), sizeof(value));
#endif
	value = 1;
#ifdef _WIN32
	setsockopt(m_fs, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char*>(&value), sizeof(value));
#else
	setsockopt(m_fs, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<void*>(&value), sizeof(value));
#endif
  }
  if (ret == 0) {
    ret = connect(m_fs, (struct sockaddr*)&m_address, sizeof(m_address));
  }
  if (ret < 0) {
    close();
    return RESULT_ERR_GENERIC_IO;
  }
#ifdef _WIN32
  u_long cnt;
  if (ioctlsocket(m_fs, FIONREAD, &cnt) == 0 && cnt > 1) {
#else
  int cnt;
  if (ioctl(m_fs, FIONREAD, &cnt) >= 0 && cnt > 1) {
#endif
		  // skip buffered input
    symbol_t buf[256];
#ifdef _WIN32
	while (::recv(m_fs, (char*)buf, 256, 0) > 0) {
	}
#else
    while (::read(m_fs, &buf, 256) > 0) {
    }
#endif
  }
  if (m_bufSize == 0) {
    m_bufSize = MAX_LEN+1;
    m_buffer = reinterpret_cast<symbol_t*>(malloc(m_bufSize));
    if (!m_buffer) {
      m_bufSize = 0;
    }
  }
  m_bufLen = 0;
  m_isOpen = true;
  if (m_initialSend && write(ESC) != 1) {
    return RESULT_ERR_SEND;
  }
  return RESULT_OK;
}


void NetworkDevice::close() {
	if (m_fs != -1) {
#ifdef _WIN32
		::closesocket(m_fs);
#else
		::close(m_fs);
#endif
		m_fs = -1;
	}

	Device::close();
}

void NetworkDevice::checkDevice() {
#ifdef _WIN32
	char value;
  ssize_t c = ::recv(m_fs, &value, 1, MSG_PEEK);
  if (c == 0 || (c < 0 && errno != EAGAIN)) {
#else
	symbol_t value;
	ssize_t c = ::recv(m_fs, &value, 1, MSG_PEEK | MSG_DONTWAIT);
  if (c == 0 || (c < 0 && GetLastError() != WSAEWOULDBLOCK)) {
#endif
    m_bufLen = 0;  // flush read buffer
    close();
  }
}

bool NetworkDevice::available() {
  return m_buffer && m_bufLen > 0;
}

ssize_t NetworkDevice::write(symbol_t value) {
  m_bufLen = 0;  // flush read buffer
#ifdef _WIN32
  return ::send(m_fs, (char*)value, 1, 0);
#else
  return ::write(m_fs, value, 1);
#endif
  //return Device::write(value);
}

ssize_t NetworkDevice::read(symbol_t& value) {
  if (available()) {
    value = m_buffer[m_bufPos];
    m_bufPos = (m_bufPos+1)%m_bufSize;
    m_bufLen--;
    return 1;
  }
  if (m_bufSize > 0) {
#ifdef _WIN32
    ssize_t size = ::recv(m_fs, (char*)m_buffer, m_bufSize, 0);
#else
	ssize_t size = ::read(m_fs, m_buffer, m_bufSize);
#endif
	if (size <= 0) {
      return size;
    }
    value = m_buffer[0];
    m_bufPos = 1;
    m_bufLen = size-1;
    return size;
  }
#ifdef _WIN32
  return ::recv(m_fs, (char*)value, 1, 0);
#else
  return ::read(m_fs, value, 1);
#endif
  //return Device::read(value);
}

}  // namespace ebusd
