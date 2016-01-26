#ifndef _CMIWAITSEM_H_
#define _CMIWAITSEM_H_

#ifdef _WIN32
#include <Windows.h>
#else
#include<time.h>
#include <errno.h>
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
		pthread_mutex_lock(&lock);
		iscond = true;
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&lock);
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
pthread_mutex_lock(&lock);
while (iscond == false)   {
	pthread_cond_wait(&cond, &lock);
}
pthread_mutex_unlock(&lock);

  return WAITSEM;
}
else
{
  struct timespec ts;
  int s;
  struct timeval now;
  struct timespec outtime;

  pthread_mutex_lock(&lock);
  int sec = delayMillisecond / 1000;
  int usec = (delayMillisecond % 1000) * 1000;
  gettimeofday(&now, NULL);
  outtime.tv_sec = now.tv_sec + sec;
  outtime.tv_nsec = (now.tv_usec + usec) * 1000;

  while (iscond == false)  {
	  s = pthread_cond_timedwait(&cond, &lock, &outtime);
	  if(s != 0)
		  break;
  }
  pthread_mutex_unlock(&lock);

  if(s!=0)
	  return TIMEDOUT;

  return WAITSEM;

  #endif
}


private:

#ifdef _WIN32
	HANDLE  handle;
#else
	pthread_cond_t  cond;
	pthread_mutex_t lock;
	bool iscond;
#endif

	void Init(){
#ifdef _WIN32
		handle = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
		pthread_mutex_init(&lock,NULL);
		pthread_cond_init(&cond, NULL);
		iscond = false;
#endif
	}

	void Close(){
#ifdef _WIN32
		CloseHandle(handle);
#else
		pthread_cond_destroy(&cond);
		pthread_mutex_destroy(&lock);
#endif
	}
};

#endif // _CMIWAITSEM_H_
