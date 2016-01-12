#include"TaskProcesser.h"


bool CommonTaskProcesser::Start()
{
	if (0 != task_thread_create(&taskThread))
	{
		return false;
	}
	return true;
}

void CommonTaskProcesser::Stop()
{
	task_thread_join(taskThread);
}

int CommonTaskProcesser::PostTask(task_func_cb taskfunc, task_func_cb tk_release_func, void* taskData, int delay)
{
	return post_task(taskThread, task_create(taskfunc, tk_release_func, taskData), delay);
}


