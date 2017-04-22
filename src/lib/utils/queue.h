/*
 * ebusd - daemon for communication with eBUS heating systems.
 * Copyright (C) 2014-2017 John Baier <ebusd@ebusd.eu>
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

#ifndef LIB_UTILS_QUEUE_H_
#define LIB_UTILS_QUEUE_H_

#ifdef _WIN32
#include "lib/utils/lock.h"
#else
#include <pthread.h>
#include <errno.h>
#include "lib/utils/clock.h"
#endif

#include <list>

namespace ebusd {

/** \file lib/utils/queue.h */

using std::list;

/**
 * Thread safe template class for queuing items.
 * @param T the item type.
 */
template <typename T>
class Queue {
 public:
  /**
   * Constructor.
   */
  Queue() {
#ifndef _WIN32
    pthread_mutex_init(&m_mutex, NULL);
    pthread_cond_init(&m_cond, NULL);
#endif
  }

  /**
   * Destructor.
   */
  ~Queue() {
#ifndef _WIN32
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
#endif
  }


 private:
  /**
   * Hidden copy constructor.
   * @param src the object to copy from.
   */
  Queue(const Queue& src);


 public:
  /**
   * Add an item to the end of queue.
   * @param item the item to add.
   */
  void push(T item) {
#ifdef _WIN32
	  m_lock.Aquire();
#else
    pthread_mutex_lock(&m_mutex);
#endif
    m_queue.push_back(item);
#ifdef _WIN32
	m_lock.Set();
	m_lock.Release();
#else
    pthread_cond_broadcast(&m_cond);
    
	tex_unlock(&m_mutex);
#endif
  }

  /**
   * Remove the first item from the queue optionally waiting for the queue being non-empty.
   * @param timeout the maximum time in seconds to wait for the queue being filled, or 0 for no wait.
   * @return the item, or NULL if no item is available within the specified time.
   */
  T pop(int timeout = 0) {
    T item;
#ifdef _WIN32
	m_lock.Aquire();
#else
    pthread_mutex_lock(&m_mutex);
#endif
    if (timeout > 0) {
#ifdef _WIN32
		DWORD ms = timeout * 1000;
      while (m_queue.empty() && ms > 0) {
		  FILETIME start, end;
		  GetSystemTimeAsFileTime(&start);
		  DWORD ret = m_lock.Wait(ms);
		  if (ret != WAIT_OBJECT_0){
			  break;
		  }
		  GetSystemTimeAsFileTime(&end);
		  ms -= (DWORD)(((*reinterpret_cast<long long*>(&end)) - (*reinterpret_cast<long long*>(&start))) / 10000);
	  }
#else
		struct timespec t;
		clockGettime(&t);
		t.tv_sec += timeout;
		while (m_queue.empty()) {
			if (pthread_cond_timedwait(&m_cond, &m_mutex, &t) == ETIMEDOUT) {
				break;
			}
		}
#endif
    }
    if (m_queue.empty()) {
      item = NULL;
    } else {
      item = m_queue.front();
      m_queue.pop_front();
    }
#ifdef _WIN32
	m_lock.Release();
#else
    pthread_mutex_unlock(&m_mutex);
#endif
    return item;
  }

  /**
   * Remove the specified item from the queue optionally waiting for it to appear in the queue.
   * @param item the item to remove and optionally wait for.
   * @param wait true to wait for the item to appear in the queue.
   * @return whether the item was removed.
   */
  bool remove(T item, bool wait = false) {
    bool ret = false;
#ifdef _WIN32
	m_lock.Aquire();
#else
	pthread_mutex_lock(&m_mutex);
#endif
    do {
      size_t oldSize = m_queue.size();
      if (oldSize > 0) {
        m_queue.remove(item);
        if (m_queue.size() != oldSize) {
          ret = true;
          break;
        }
      }
#ifdef _WIN32
	  m_lock.Wait();
#else
      pthread_cond_wait(&m_cond, &m_mutex);
#endif
    } while (wait);
#ifdef _WIN32
	m_lock.Release();
#else
	pthread_mutex_unlock(&m_mutex);
#endif
    return ret;
  }

  /**
   * Return the first item in the queue without removing it.
   * @return the item, or NULL if no item is available.
   */
  T peek() {
    T item;
#ifdef _WIN32
	m_lock.Aquire();
#else
	pthread_mutex_lock(&m_mutex);
#endif
    if (m_queue.empty()) {
      item = NULL;
    } else {
      item = m_queue.front();
    }
#ifdef _WIN32
	m_lock.Release();
#else
	pthread_mutex_unlock(&m_mutex);
#endif
    return item;
  }


 private:
  /** the queue itself */
  list<T> m_queue;
#ifdef _WIN32
  Lock m_lock;
#else
  /** mutex variable for exclusive lock */
  pthread_mutex_t m_mutex;

  /** condition variable for exclusive lock */
  pthread_cond_t m_cond;
#endif
};

}  // namespace ebusd

#endif  // LIB_UTILS_QUEUE_H_
