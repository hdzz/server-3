#ifndef _CMITHREADLOCK_H_
#define _CMITHREADLOCK_H_

#ifdef _WIN32
#include <Windows.h>
#else
#include<pthread.h>
#endif

class CmiThreadLock
{
public:
	CmiThreadLock(void){
		Init();
	}
	~CmiThreadLock(){
		Close();
	}
	void Lock(){
#ifdef _WIN32
		EnterCriticalSection(&m_lock);
#else
		pthread_mutex_lock(&m_lock);
#endif
	}
	void UnLock(){
#ifdef _WIN32
		LeaveCriticalSection(&m_lock);
#else
		pthread_mutex_unlock(&m_lock);
#endif
	}
	//protected:
private:

	#ifdef _WIN32
CRITICAL_SECTION m_lock;
#else
pthread_mutex_t m_lock;
#endif

	void Init(){
#ifdef _WIN32
		InitializeCriticalSection(&m_lock);
#else
		pthread_mutex_init(&m_lock,NULL);
#endif
	}
	void Close(){
#ifdef _WIN32
		DeleteCriticalSection(&m_lock);
#else
		pthread_mutex_destroy(&m_lock);
#endif
	}
};

///////////////////////////////////////////////////////////////////////
//定义一个自动加锁的类
class CmiAutoLock
{
public:
	CmiAutoLock(CmiThreadLock *pThreadLock){
		m_pThreadLock = pThreadLock;
		if (NULL != m_pThreadLock)
		{
			m_pThreadLock->Lock();
		}
	}
	~CmiAutoLock(){
		if (NULL != m_pThreadLock)
		{
			m_pThreadLock->UnLock();
		}
	}
private:
	CmiThreadLock * m_pThreadLock;
};


#endif
