#ifndef TASKTHREAD_H
#define TASKTHREAD_H

#include<pthread.h>
#include <string.h>
#include<stdio.h>
#include"utils.h"

#define TK_QUIT  1

typedef struct task_node_s     task_node_t;
typedef struct task_thread_s   task_thread_t;
typedef struct task_queue_s    task_queue_t;
typedef struct task_pump_s     task_pump_t;

typedef void* (*task_func_cb)(void *data);


struct task_node_s
{
	uint64_t                startTime;
	int                     delay;          //延迟执行时间
	task_func_cb            func;           //任务函数
	task_func_cb            release_func;   //数据释放函数
	void                   *data;           //任务数据
	unsigned int            msg;            //任务标志
	struct task_node_s     *up;
	struct task_node_s     *next;
};

struct task_thread_s
{
	pthread_t        pthread;
	void            *p;       //task_pump_t
};

struct task_queue_s
{
	int len;
	task_node_t* first;
	task_node_t* last;
};

struct task_pump_s
{
	int    isquit;
	task_queue_t*   read_queue;
	task_queue_t*   write_queue;
	task_queue_t*   timer_read_queue;
	task_queue_t*   timer_write_queue;	
	pthread_mutex_t lock;
	pthread_cond_t  cond;
};



task_node_t* task_create(
	task_func_cb tk_func,
	task_func_cb tk_release_func,
	void* tk_data);

task_node_t* buf_task_create(
	task_func_cb tk_func,
	char* buf,
	int len);

task_node_t* buf_task_create2(
	task_func_cb tk_func,
	char* buf);

void* release_buf(void* buf);

int task_thread_create(task_thread_t* tk_thread);
int task_thread_join(task_thread_t tk_thread);
int post_task(task_thread_t tk_thread, task_node_t* tk, int delay);
int post_quit_task(task_thread_t tk_thread);

void task_queue_add(task_queue_t* tk_queue, task_node_t* tk_node);
void task_queue_del(task_queue_t* tk_queue, task_node_t* tk_node);
void task_queue_release(task_queue_t* tk_queue);
void task_queue_merge(task_queue_t* tk_dst_queue, task_queue_t* tk_merge_queue);

#endif
