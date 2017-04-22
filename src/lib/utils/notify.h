/*
 * ebusd - daemon for communication with eBUS heating systems.
 * Copyright (C) 2014-2017 John Baier <ebusd@ebusd.eu>, Roland Jax 2012-2014 <ebusd@liwest.at>
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

#ifndef LIB_UTILS_NOTIFY_H_
#define LIB_UTILS_NOTIFY_H_

#ifndef _WIN32
#include <unistd.h>
#endif
#include <fcntl.h>

namespace ebusd {

/** \file lib/utils/notify.h */

/**
 * class to notify other thread per pipe.
 */
class Notify {
 public:
  /**
   * constructs a new instance and do notifying.
   */
  Notify() {
#ifdef _WIN32
	  //m_evt = CreateEvent(NULL, FALSE, FALSE, NULL);
	  m_evt = WSACreateEvent();
#else
    int pipefd[2];
    int ret = pipe(pipefd);

    if (ret == 0) {
      m_recvfd = pipefd[0];
      m_sendfd = pipefd[1];

      fcntl(m_sendfd, F_SETFL, O_NONBLOCK);
    }
#endif
  }

  /**
   * destructor.
   */
  ~Notify() {
#ifdef _WIN32
	  WSACloseEvent(m_evt);
#else
	  close(m_sendfd); close(m_recvfd); 
#endif
  }

  /**
   * file descriptor to watch for notify event.
   * @return the notification value.
   */
#ifdef _WIN32
  WSAEVENT notifyHandle() { return m_evt; }
#else
  int notifyFD() { return m_recvfd; }
#endif

  /**
   * write notify event to file descriptor.
   * @return result of writing notification.
   */
  ssize_t notify() const {
#ifdef _WIN32
	  WSASetEvent(m_evt);
	  return 1;
#else
	  return write(m_sendfd, "1", 1); 
#endif
  }

 private:
#ifdef _WIN32
	 WSAEVENT m_evt;
#else
  /** file descriptor to watch */
  int m_recvfd;

  /** file descriptor to notify */
  int m_sendfd;
#endif
};

}  // namespace ebusd

#endif  // LIB_UTILS_NOTIFY_H_
