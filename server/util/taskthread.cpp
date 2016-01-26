#include"taskthread.h"
#include"utils.h"
#include <stdlib.h>

#ifdef _WIN32
static DWORD WINAPI run(LPVOID param);
#else
static void* run(void* param);
#endif

static task_pump_t* task_pump_create();
static void task_pump_release(task_pump_t* pump);

//task_node_t
task_node_t* task_create(
	task_func_cb tk_func,
	task_func_cb tk_release_func,
	void* tk_data)
{
	task_node_t* tk = (task_node_t*)malloc(sizeof(task_node_t));
	memset(tk, 0, sizeof(task_node_t));

	tk->func = tk_func;
	tk->release_func = tk_release_func;
	tk->data = tk_data;

	return tk;
}

task_node_t* buf_task_create(
	task_func_cb tk_func,
	char* buf,
	int len)
{
	task_node_t* tk = (task_node_t*)malloc(sizeof(task_node_t));
	memset(tk, 0, sizeof(task_node_t));

	tk->func = tk_func;
	tk->release_func = release_buf;

	if (buf <= 0)
		len = strlen(buf);

	tk->data = (void*)malloc(len);
	memcpy(tk->data, buf, len);

	return tk;
}

task_node_t* buf_task_create2(task_func_cb tk_func, char* buf)
{
	int len;
	task_node_t* tk = (task_node_t*)malloc(sizeof(task_node_t));
	memset(tk, 0, sizeof(task_node_t));

	tk->func = tk_func;
	tk->release_func = release_buf;
	len = strlen(buf);

	tk->data = (void*)malloc(len);
	memcpy(tk->data, buf, len);

	return tk;
}

void* release_buf(void* buf)
{
	 free(buf);
	 return 0;
}


//task_thread_t
int task_thread_create(task_thread_t* tk_thread)
{
	task_pump_t* pump = task_pump_create();

	if (pump == 0)
		return -1;

#ifdef _WIN32
	tk_thread->handle = CreateThread(NULL, 0, run, (void*)pump, 0, NULL);
	if (0 == tk_thread->handle){
		task_pump_release(pump);
		return -1;
	}
#else
	int err = 0;
	err = pthread_create(&tk_thread->pthread, NULL, run, (void*)pump);
	if (0 != err){
		task_pump_release(pump);
		return -1;
	}
#endif

	tk_thread->p = (void*)pump;
	return 0;
}


int post_task(task_thread_t tk_thread, task_node_t* tk, int delay)
{
#ifdef _WIN32
	int ret = 0;
	task_pump_t* pump = (task_pump_t*)tk_thread.p;

	tk->delay = delay;
	if (delay > 0)
		tk->startTime = GetTickCount64();

	pump->lock->Lock();

	if (!pump->isquit)
	{
		if (tk->msg == TK_QUIT)
			pump->isquit = 1;

		task_queue_add(pump->write_queue, tk);

		pump->lock->UnLock();
		pump->sem->SetSem();
		pump->lock->Lock();
	}
	else
	{
		ret = -1;
	}

	pump->lock->UnLock();

	return ret;

#else
	int ret = 0;
	task_pump_t* pump = (task_pump_t*)tk_thread.p;

	if (pthread_kill(tk_thread.pthread, 0) != ESRCH)
	{
		tk->delay = delay;
		if (delay > 0)
			tk->startTime = GetTickCount64();

		pump->lock->Lock();

		if (!pump->isquit)
		{
			if (tk->msg == TK_QUIT)
				pump->isquit = 1;

			task_queue_add(pump->write_queue, tk);
			pthread_cond_signal(&pump->cond);
		}
		else
		{
			ret = -1;
		}

		pump->lock->UnLock();

		return ret;
	}

	return -1;
#endif
}


int task_thread_join(task_thread_t tk_thread)
{
    void* status;
	int ret = 0;
	task_pump_t* pump = (task_pump_t*)tk_thread.p;

	if (0 != post_quit_task(tk_thread))
		return -1;

	//等待任务线程结束
#ifdef _WIN32
	pump->quit_sem->WaitSem(-1);
#else
	ret = pthread_join(tk_thread.pthread, &status);
#endif

	task_pump_release(pump);

	return ret;
}


int post_quit_task(task_thread_t tk_thread)
{
	task_node_t* tk = (task_node_t*)calloc(1, sizeof(task_node_t));
	tk->msg = TK_QUIT;

	if (-1 == post_task(tk_thread, tk, 0))
	{
		free(tk);
		return -1;
	}
	return 0;
}

//task_pump_t
#ifdef _WIN32
static DWORD WINAPI run(LPVOID param)
#else
static void* run(void* param)
#endif
{
    task_pump_t* pump = ( task_pump_t*)param;
	task_queue_t* rque = pump->read_queue;
	task_queue_t* wque = pump->write_queue;
	task_node_t* cur = rque->first;
	unsigned int msg;
	task_node_t* tmp;
	task_queue_t timer_que;
	memset(&timer_que, 0, sizeof(task_queue_t));
	uint64_t curTime = GetTickCount64();
	uint32_t delay = 0xFFFFFFFF;
	uint32_t curdelay;

	for (;;)
	{
		if (cur)
		{
			if (cur->delay > 0 &&
				curTime - cur->startTime < cur->delay)
			{
				tmp = cur->next;
				task_queue_del(rque, cur);
				task_queue_add(&timer_que, cur);

				curdelay = cur->delay - (curTime - cur->startTime);

				if (curdelay < delay)
					delay = curdelay;

				cur = tmp;
				continue;
			}

			msg = cur->msg;

			if (cur->func)
				cur->func(cur->data);

			if (cur->release_func)
				cur->release_func(cur->data);

			tmp = cur;
			cur = cur->next;
			rque->len--;
			rque->first = cur;

			if (rque->len == 0)
				rque->last = 0;

			if (cur)
				cur->up = 0;

			free(tmp);

			if (msg == TK_QUIT){
#ifdef _WIN32
				pump->quit_sem->SetSem();
#endif
				return 0;
			}

		}
		else
		{
			pump->lock->Lock();

#ifdef _WIN32
			while (wque->len == 0)
			{
				if (delay == 0xFFFFFFFF){
					pump->lock->UnLock();
					pump->sem->WaitSem(-1);
					pump->lock->Lock();
				}
				else{
					pump->sem->WaitSem(delay);
					break;
				}
			}
#else
			while (wque->len == 0)
			{
				if (delay == 0xFFFFFFFF){
					pthread_cond_wait(&pump->cond, &pump->lock->GetMutex());
				}
				else{
					struct timeval now;
					struct timespec outtime;
					int sec = delay / 1000;
					int usec = (delay % 1000) * 1000;
					gettimeofday(&now, NULL);
					outtime.tv_sec = now.tv_sec + sec;
					outtime.tv_nsec = (now.tv_usec + usec) * 1000;
					pthread_cond_timedwait(&pump->cond, &pump->lock->GetMutex(), &outtime);
					break;
				}
			}
#endif

			if (wque->len > 0)
			{
				if (timer_que.len > 0)
					task_queue_merge(wque, &timer_que);

				pump->write_queue = rque;
				pump->read_queue = wque;

				rque = pump->read_queue;
				wque = pump->write_queue;

				cur = rque->first;
			}
			else
			{
				if (timer_que.len > 0){
					task_queue_merge(rque, &timer_que);
					cur = rque->first;
				}
			}

			pump->lock->UnLock();

			curTime = GetTickCount64();
		}

	}

#ifdef _WIN32
	pump->quit_sem->SetSem();
#endif
	return 0;
}

static task_pump_t* task_pump_create()
{
	task_pump_t* pump = 0;
	pump = (task_pump_t*)calloc(1, sizeof(task_pump_t));
	pump->read_queue = (task_queue_t*)calloc(1, sizeof(task_queue_t));
	pump->write_queue = (task_queue_t*)calloc(1, sizeof(task_queue_t));

	pump->lock = new CmiThreadLock();

#ifdef _WIN32
	pump->sem = new CmiWaitSem();
	pump->quit_sem = new CmiWaitSem();
#else
	pthread_cond_init(&pump->cond, NULL);
#endif

	return pump;
}

static void task_pump_release(task_pump_t* pump)
{
	task_queue_release(pump->read_queue);
	free(pump->read_queue);
	pump->read_queue = 0;

	task_queue_release(pump->write_queue);
	free(pump->write_queue);
	pump->write_queue = 0;

	delete pump->lock;

#ifdef _WIN32
	delete pump->sem;
	delete pump->quit_sem;
#else
	pthread_cond_destroy(&pump->cond);
#endif
	free(pump);
}


//task_queue_t
void task_queue_add(task_queue_t* tk_queue, task_node_t* tk_node)
{
	task_node_t* last = tk_queue->last;
	tk_queue->len++;

	if (last)
	{
		last->next = tk_node;
		tk_node->up = last;
		tk_queue->last = tk_node;
	}
	else
	{
		tk_queue->last = tk_queue->first = tk_node;
	}
}

void task_queue_del(task_queue_t* tk_queue, task_node_t* tk_node)
{
	task_node_t* up = tk_node->up;
	task_node_t* next = tk_node->next;

	if (up)
		up->next = next;
	else
		tk_queue->first = next;

	if (next)
		next->up = up;
	else
		tk_queue->last = up;

	tk_node->next = 0;
	tk_node->up = 0;

	tk_queue->len--;
}


void task_queue_release(task_queue_t* tk_queue)
{
	task_node_t* node = tk_queue->first;
	task_node_t* tmp;
	for (; node;)
	{
		tmp = node;
		node = node->next;
		free(tmp);
	}
	tk_queue->first = tk_queue->last = 0;
	tk_queue->len = 0;
}


void task_queue_merge(task_queue_t* tk_dst_queue, task_queue_t* tk_merge_queue)
{
	if (tk_merge_queue->len == 0)
		return;

	if (tk_dst_queue->len == 0)
	{
		memcpy(tk_dst_queue, tk_merge_queue, sizeof(task_queue_t));
		memset(tk_merge_queue, 0, sizeof(task_queue_t));
		return;
	}

	task_node_t* dstLast = tk_dst_queue->last;
	task_node_t* mergeFirst = tk_merge_queue->first;

	dstLast->next = mergeFirst;
	mergeFirst->up = dstLast;

	tk_dst_queue->last = tk_merge_queue->last;
	tk_dst_queue->len += tk_merge_queue->len;

	memset(tk_merge_queue, 0, sizeof(task_queue_t));
}
