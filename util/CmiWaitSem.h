#ifndef _CMIWAITSEM_H_
#define _CMIWAITSEM_H_

#ifdef _WIN32
#include <Windows.h>
#else
#include<time.h>
#include <errno.h>
#include<semaphore.h>
#include<pthread.h>
#include <sys/time.h>
#endif


#ifndef  MYUTILS_H
#define TIMEDOUT 1
#define WAITSEM  0
#endif

class CmiWaitSem
{
public:
	CmiWaitSem(void){
		Init();
	}
	~CmiWaitSem(){
		Close();
	}

	void SetSem()
	{
	#ifdef _WIN32
	SetEvent(handle);
	#else
	 sem_post(&sem);
	#endif
	}

int WaitSem(long  delayMillisecond)
{
#ifdef _WIN32
if(WAIT_OBJECT_0 != WaitForSingleObject(handle, delayMillisecond))
           return  TIMEDOUT;
else
           return  WAITSEM;
#else
if(delayMillisecond < 0)
{
int s;
      while ((s = sem_wait(&sem)) == -1 && errno == EINTR)
        continue;       /* Restart if interrupted by handler */

    if (s == -1) {
        if (errno == ETIMEDOUT)
            return TIMEDOUT;
    }
    return WAITSEM;
}
else
{
  struct timespec ts;
  int s;

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
    }

   ts.tv_sec +=  delayMillisecond / 1000 ;

  while ((s = sem_timedwait(&sem, &ts)) == -1 && errno == EINTR)
        continue;       /* Restart if interrupted by handler */

    if (s == -1) {
        if (errno == ETIMEDOUT)
            return TIMEDOUT;
    }
    return WAITSEM;

  }
  #endif
}


private:

#ifdef _WIN32
	HANDLE  handle;
#else
    sem_t   sem;
#endif

	void Init(){
#ifdef _WIN32
		handle = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
		sem_init(&sem,0,0);
#endif
	}

	void Close(){
#ifdef _WIN32
		CloseHandle(handle);
#else
        sem_destroy(&sem);
#endif
	}
};

#endif // _CMIWAITSEM_H_
