#ifndef  MYUTILS_H
#define  MYUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <Windows.h>
#else
#include<time.h>
#include <errno.h>
#include<semaphore.h>
#include<pthread.h>
#include <sys/time.h>
#endif

#ifndef _CMIWAITSEM_H_
#define TIMEDOUT 1
#define WAITSEM  0
#endif


//////////////////////////////////
typedef struct threadlock_s
{

#ifdef _WIN32
CRITICAL_SECTION m_lock;
#else
pthread_mutex_t m_lock;
#endif

} threadlock_t;

void threadlock_init(threadlock_t* tlock);
void threadlock_close(threadlock_t* tlock);
void threadlock_lock(threadlock_t* tlock);
void threadlock_unlock(threadlock_t* tlock);




//////////////////////////////////
typedef struct waitsem_s
{
#ifdef _WIN32
	HANDLE      handle;
#else
	sem_t           sem;
#endif

} waitsem_t;

void waitsem_init(waitsem_t*  waitsem);
void waitsem_close(waitsem_t*  waitsem);
void waitsem_set_sem(waitsem_t*  waitsem);
int waitsem_wait_sem(waitsem_t*  waitsem, long  delayMillisecond);

#ifdef __cplusplus
}
#endif
#endif
