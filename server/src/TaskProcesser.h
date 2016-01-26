#ifndef _TASKPROCESSER_H_
#define _TASKPROCESSER_H_

#include"taskthread.h"

class TaskProcesser
{
public:
	virtual bool Start() = 0;
	virtual void Stop() = 0;
	virtual int PostTask(task_func_cb taskfunc, task_func_cb tk_release_func, void* taskData, int delay) = 0;
	
};


class CommonTaskProcesser : public TaskProcesser
{
public:

	bool Start();
	void Stop();
	int PostTask(task_func_cb taskfunc, task_func_cb tk_release_func, void* taskData, int delay);
	
private:
	task_thread_t  taskThread;
};

#endif