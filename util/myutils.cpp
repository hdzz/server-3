#include"myutils.h"

void threadlock_init(threadlock_t* tlock)
{
#ifdef _WIN32
	InitializeCriticalSection(&tlock->m_lock);
#else
	pthread_mutex_init(&tlock->m_lock, NULL);
#endif
}

void threadlock_close(threadlock_t* tlock)
{
#ifdef _WIN32
	DeleteCriticalSection(&tlock->m_lock);
#else
	pthread_mutex_destroy(&tlock->m_lock);
#endif
}

void threadlock_lock(threadlock_t* tlock)
{
#ifdef _WIN32
	EnterCriticalSection(&tlock->m_lock);
#else
	pthread_mutex_lock(&tlock->m_lock);
#endif
}

void threadlock_unlock(threadlock_t* tlock)
{
#ifdef _WIN32
	LeaveCriticalSection(&tlock->m_lock);
#else
	pthread_mutex_unlock(&tlock->m_lock);
#endif
}

/////////////////////////////////////////////////////////////////////////
void waitsem_init(waitsem_t*  waitsem)
{
#ifdef _WIN32
	waitsem->handle = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
	sem_init(&waitsem->sem, 0, 0);
#endif
}

void waitsem_close(waitsem_t*  waitsem)
{
#ifdef _WIN32
	CloseHandle(waitsem->handle);
#else
	sem_destroy(&waitsem->sem);
#endif
}

void waitsem_set_sem(waitsem_t*  waitsem)
{
#ifdef _WIN32
	SetEvent(waitsem->handle);
#else
	sem_post(&waitsem->sem);
#endif
}

int waitsem_wait_sem(waitsem_t*  waitsem, long  delayMillisecond)
{

#ifdef _WIN32
	if (WAIT_OBJECT_0 != WaitForSingleObject(waitsem->handle, delayMillisecond))
		return   TIMEDOUT;
	else
		return  WAITSEM;
#else
	struct timeval ts;
	int s;

	if (delayMillisecond < 0)
	{
		while ((s = sem_wait(&waitsem->sem)) == -1 && errno == EINTR)
			continue;       /* Restart if interrupted by handler */

		if (s == -1) {
			if (errno == ETIMEDOUT)
				return TIMEDOUT;
		}
		return WAITSEM;
	}
	else
	{

	if(clock_gettime(1, &ts)  == -1)
		{
		}

		ts.tv_sec += delayMillisecond / 1000;

		while ((s = sem_timedwait(&waitsem->sem, &ts)) == -1 && errno == EINTR)
			continue;       /* Restart if interrupted by handler */

		if (s == -1) {
			if (errno == ETIMEDOUT)
				return TIMEDOUT;
		}
		return WAITSEM;

	}
#endif
}
