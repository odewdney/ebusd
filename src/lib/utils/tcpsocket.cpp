/*
 * ebusd - daemon for communication with eBUS heating systems.
 * Copyright (C) 2015-2017 John Baier <ebusd@ebusd.eu>, Roland Jax 2012-2014 <ebusd@liwest.at>
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

#include "lib/utils/tcpsocket.h"

#ifdef _WIN32
#include <Windows.h>
#include <Ws2tcpip.h>
typedef u_short in_port_t;
#endif

#include <fcntl.h>
#ifndef _WIN32
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <string.h>
#include <cstdlib>

namespace ebusd {
#ifdef _WIN32
int inet_aton(const char *addr, struct in_addr*in)
{
	return InetPtonA(AF_INET, addr, in) == 1;
}
#endif

TCPSocket::TCPSocket(int sfd, socketaddress* address) : m_sfd(sfd) {
  char ip[17];
  inet_ntop(AF_INET, (struct in_addr*)&(address->sin_addr.s_addr), ip, (socklen_t)sizeof(ip)-1);
  m_ip = ip;
  m_port = (uint16_t)ntohs(address->sin_port);
}

bool TCPSocket::isValid() {
#ifdef _WIN32
	DWORD err = 0;
	int errlen = sizeof(err);
	return getsockopt(m_sfd, SOL_SOCKET, SO_ERROR, (char*)&err, &errlen) == 0 && err == 0;
#else
  return fcntl(m_sfd, F_GETFL) != -1;
#endif
}


TCPSocket* TCPClient::connect(const string& server, const uint16_t& port) {
  socketaddress address;
  int ret;

  memset(reinterpret_cast<char*>(&address), 0, sizeof(address));

  if (inet_addr(server.c_str()) == INADDR_NONE) {
    struct hostent* he;

    he = gethostbyname(server.c_str());
    if (he == NULL) {
      return NULL;
    }
    memcpy(&address.sin_addr, he->h_addr_list[0], he->h_length);
  } else {
    ret = inet_aton(server.c_str(), &address.sin_addr);
    if (ret == 0) {
      return NULL;
    }
  }

  address.sin_family = AF_INET;
  address.sin_port = (in_port_t)htons(port);

  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd < 0) {
    return NULL;
  }
  ret = ::connect(sfd, (struct sockaddr*) &address, sizeof(address));
  if (ret < 0) {
    return NULL;
  }
  return new TCPSocket(sfd, &address);
}


int TCPServer::start() {
  if (m_listening) {
    return 0;
  }
  m_lfd = socket(AF_INET, SOCK_STREAM, 0);
  socketaddress address;

  memset(&address, 0, sizeof(address));

  address.sin_family = AF_INET;
  address.sin_port = (in_port_t)htons(m_port);

  if (m_address.size() > 0) {
    inet_pton(AF_INET, m_address.c_str(), &(address.sin_addr));
  } else {
    address.sin_addr.s_addr = INADDR_ANY;
  }
  int optval = 1;
  setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));

  int result = bind(m_lfd, (struct sockaddr*) &address, sizeof(address));
  if (result != 0) {
    return result;
  }
  result = listen(m_lfd, 5);
  if (result != 0) {
    return result;
  }
  m_listening = true;
  return result;
}

TCPSocket* TCPServer::newSocket() {
  if (!m_listening) {
    return NULL;
  }
  socketaddress address;
  socklen_t len = sizeof(address);

  memset(&address, 0, sizeof(address));

  int sfd = accept(m_lfd, (struct sockaddr*) &address, &len);
  if (sfd < 0) {
    return NULL;
  }
  return new TCPSocket(sfd, &address);
}

}  // namespace ebusd
