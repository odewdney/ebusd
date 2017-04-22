

#ifndef LIB_UTILS_LOCK_H_
#define LIB_UTILS_LOCK_H_

#include <Windows.h>

namespace ebusd {

	class Lock {
	public:
		Lock()
		{
			InitializeCriticalSection(&m_cs);
			m_evt = CreateEvent(NULL, FALSE, FALSE, NULL);
		}

		~Lock()
		{
			DeleteCriticalSection(&m_cs);
			CloseHandle(m_evt);
		}

		void Aquire()
		{
			EnterCriticalSection(&m_cs);
		}

		void Release()
		{
			LeaveCriticalSection(&m_cs);
		}

		DWORD Wait(DWORD ms = INFINITE)
		{
			LeaveCriticalSection(&m_cs);
			DWORD ret = WaitForSingleObject(m_evt, ms);
			EnterCriticalSection(&m_cs);
			return ret;
		}

		void Set()
		{
			SetEvent(m_evt);
		}
	private:
		HANDLE m_evt;
		CRITICAL_SECTION m_cs;

};

}  // namespace ebusd

#endif  // LIB_UTILS_LOCK_H_
