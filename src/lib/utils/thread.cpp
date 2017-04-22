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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef _WIN32
#include <Windows.h>
#include <process.h>
#endif

#include "lib/utils/thread.h"
#include "lib/utils/clock.h"

namespace ebusd {

#ifdef _WIN32
unsigned int Thread::runThread(void* arg) {
  reinterpret_cast<Thread*>(arg)->enter();
  return NULL;
}
#else
void* Thread::runThread(void* arg) {
	reinterpret_cast<Thread*>(arg)->enter();
	return NULL;
}
#endif
Thread::~Thread() {
  if (m_started) {
#ifdef _WIN32
	  TerminateThread(m_threadHandle,1);
	  CloseHandle(m_threadHandle);
#else
    pthread_cancel(m_threadid);
    pthread_detach(m_threadid);
#endif
  }
}

bool Thread::start(const char* name) {
#ifdef _WIN32
	int result = 0;
	//m_threadHandle = CreateThread(NULL, 0, runThread, this, 0, &m_threadid);
	m_threadHandle = (HANDLE) _beginthreadex(NULL, 0, runThread, this, 0, &m_threadid);
	if (m_threadHandle == NULL)
		result = GetLastError();
#else
	int result = pthread_create(&m_threadid, NULL, runThread, this);
#endif
	if (result == 0) {
#ifdef HAVE_PTHREAD_SETNAME_NP
#ifndef __MACH__
    pthread_setname_np(m_threadid, name);
#endif
#endif
    m_started = true;
    return true;
  }
  return false;
}

bool Thread::join() {
  int result = -1;
  if (m_started) {
    m_stopped = true;
#ifdef _WIN32
	result = WaitForSingleObject(m_threadHandle, INFINITE) != WAIT_OBJECT_0;
#else
    result = pthread_join(m_threadid, NULL);
#endif
    if (result == 0) {
      m_started = false;
    }
  }
  return result == 0;
}

void Thread::enter() {
  m_running = true;
  run();
  m_running = false;
}


WaitThread::WaitThread()
  : Thread() {
#ifdef _WIN32
	m_waitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
  pthread_mutex_init(&m_mutex, NULL);
  pthread_cond_init(&m_cond, NULL);
#endif
}

WaitThread::~WaitThread() {
#ifdef _WIN32
	CloseHandle(m_waitEvent);
#else
	pthread_mutex_destroy(&m_mutex);
  pthread_cond_destroy(&m_cond);
#endif
}

void WaitThread::stop() {
#ifdef _WIN32
	SetEvent(m_waitEvent);
#else
	pthread_mutex_lock(&m_mutex);
  pthread_cond_signal(&m_cond);
  pthread_mutex_unlock(&m_mutex);
#endif
  Thread::stop();
}

bool WaitThread::join() {
#ifdef _WIN32
	SetEvent(m_waitEvent);
#else
	pthread_mutex_lock(&m_mutex);
  pthread_cond_signal(&m_cond);
  pthread_mutex_unlock(&m_mutex);
#endif
  return Thread::join();
}

bool WaitThread::Wait(int seconds) {
#ifdef _WIN32
	WaitForSingleObject(m_waitEvent, seconds * 1000);
#else
  struct timespec t;
  clockGettime(&t);
  t.tv_sec += seconds;
  pthread_mutex_lock(&m_mutex);
  pthread_cond_timedwait(&m_cond, &m_mutex, &t);
  pthread_mutex_unlock(&m_mutex);
#endif
  return isRunning();
}

}  // namespace ebusd
