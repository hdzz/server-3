#ifndef _SERVERBASETYPES_H_
#define _SERVERBASETYPES_H_

#ifdef _WIN32
#undef _EPOLL
#define _IOCP
#else 
#undef _IOCP
#define _EPOLL
#endif


#ifdef _IOCP
// winsock 2 的头文件和库
#include<winsock2.h>
#include<MSWSock.h>
#endif


#ifdef _EPOLL
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

#include"taskthread.h"
#include"utils.h"
#include"CmiVector.h"
#include"CmiThreadLock.h"
#include"CmiWaitSem.h"
#include<string>
//#include<CmiLogger.h>


// 每一个处理器上产生多少个线程(为了最大限度的提升服务器性能，详见配套文档)
#define WORKER_THREADS_PER_PROCESSOR 2
// 同时投递的Accept请求的数量(这个要根据实际的情况灵活设置)
#define MAX_POST_ACCEPT              10

#define MAX_PATH          260
// 缓冲区长度 (1024*4)
#define MAX_BUFFER_LEN        2048
#define MIDDLE_BUFFER_LEN     1024
// 默认端口
#define DEFAULT_PORT          8888
// 默认IP地址
#define DEFAULT_IP            "127.0.0.1"

// 传递给Worker线程的退出信号
#define EXIT_CODE                    NULL

// 释放指针宏
#define RELEASE(x)                      {if(x != NULL ){delete x;x=NULL;}}

// 释放数组指针宏
#define RELEASE_GROUP(x)                {if(x != NULL ){delete [] x;x=NULL;}}

// 释放句柄宏
#define RELEASE_HANDLE(x)               {if(x != NULL && x!=INVALID_HANDLE_VALUE){ CloseHandle(x);x = NULL;}}


// 释放Socket宏
#ifdef _IOCP
#define RELEASE_SOCKET(x)               {if(x !=INVALID_SOCKET) { closesocket(x);x=INVALID_SOCKET;}}
#endif

#ifdef _EPOLL
#define RELEASE_SOCKET(x)               {if(x !=INVALID_SOCKET) { close(x);x=INVALID_SOCKET;}}
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct hostent HOSTENT;
#define  INVALID_SOCKET 0
#define SOCKET_ERROR -1
#endif

class RemoteServerInfo;
struct IoContext;
class SocketContext;
class ServerContext;

typedef CmiVector<IoContext*> IoCtxList;
typedef CmiVector<SocketContext*> SocketCtxList;
typedef CmiVector<RemoteServerInfo*> RemoteServerInfoList;

typedef void(*DataReleaseFunc)(void *data);
typedef void (*CreateSendPack)(IoContext* ioCtx);


#ifdef _IOCP
typedef WSABUF PackBuf;


// 在完成端口上投递的I/O类型
enum PostIoType
{
	POST_IO_CONNECT = 0,
	POST_IO_ACCEPT,                     // Accept操作
	POST_IO_RECV,                       // 接收数据
	POST_IO_SEND,                       // 发送数据
	POST_IO_NULL

};

#else
struct PackBuf
{
	int len;
	char* buf;
};
#endif

enum SocketType
{
	PENDDING_SOCKET,               //本机未决连接的SOCKET
	LISTEN_SOCKET,                 //本机监听SOCKET
	LISTEN_CLIENT_SOCKET,          //本机与远程客户端之间通连的SOCKET
	CONNECT_SERVER_SOCKET,         //本机与远程远程服务器之间正在进行连接的SOCKET
	CONNECTED_SERVER_SOCKET        //本机与远程远程服务器之间已连接的SOCKET
};

enum SocketVerifType
{
	NO_VERIF,
	ALREADY_VERIF
};


// 工作者线程的线程参数
typedef struct _tagThreadParams_WORKER
{
	ServerContext* serverCtx;                                   // 类指针，用于调用类中的函数
	int         nThreadNo;                                    // 线程编号
}THREADPARAMS_WORKER, *PTHREADPARAM_WORKER;


struct TaskData
{
	ServerContext* serverCtx;
	SocketContext* socketCtx;
	IoContext* ioCtx;
	void* dataPtr;
};

#endif