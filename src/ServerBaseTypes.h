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
// winsock 2 ��ͷ�ļ��Ϳ�
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


// ÿһ���������ϲ������ٸ��߳�(Ϊ������޶ȵ��������������ܣ���������ĵ�)
#define WORKER_THREADS_PER_PROCESSOR 2
// ͬʱͶ�ݵ�Accept���������(���Ҫ����ʵ�ʵ�����������)
#define MAX_POST_ACCEPT              10

#define MAX_PATH          260
// ���������� (1024*4)
#define MAX_BUFFER_LEN        2048
#define MIDDLE_BUFFER_LEN     1024
// Ĭ�϶˿�
#define DEFAULT_PORT          8888
// Ĭ��IP��ַ
#define DEFAULT_IP            "127.0.0.1"

// ���ݸ�Worker�̵߳��˳��ź�
#define EXIT_CODE                    NULL

// �ͷ�ָ���
#define RELEASE(x)                      {if(x != NULL ){delete x;x=NULL;}}

// �ͷ�����ָ���
#define RELEASE_GROUP(x)                {if(x != NULL ){delete [] x;x=NULL;}}

// �ͷž����
#define RELEASE_HANDLE(x)               {if(x != NULL && x!=INVALID_HANDLE_VALUE){ CloseHandle(x);x = NULL;}}


// �ͷ�Socket��
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


// ����ɶ˿���Ͷ�ݵ�I/O����
enum PostIoType
{
	POST_IO_CONNECT = 0,
	POST_IO_ACCEPT,                     // Accept����
	POST_IO_RECV,                       // ��������
	POST_IO_SEND,                       // ��������
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
	PENDDING_SOCKET,               //����δ�����ӵ�SOCKET
	LISTEN_SOCKET,                 //��������SOCKET
	LISTEN_CLIENT_SOCKET,          //������Զ�̿ͻ���֮��ͨ����SOCKET
	CONNECT_SERVER_SOCKET,         //������Զ��Զ�̷�����֮�����ڽ������ӵ�SOCKET
	CONNECTED_SERVER_SOCKET        //������Զ��Զ�̷�����֮�������ӵ�SOCKET
};

enum SocketVerifType
{
	NO_VERIF,
	ALREADY_VERIF
};


// �������̵߳��̲߳���
typedef struct _tagThreadParams_WORKER
{
	ServerContext* serverCtx;                                   // ��ָ�룬���ڵ������еĺ���
	int         nThreadNo;                                    // �̱߳��
}THREADPARAMS_WORKER, *PTHREADPARAM_WORKER;


struct TaskData
{
	ServerContext* serverCtx;
	SocketContext* socketCtx;
	IoContext* ioCtx;
	void* dataPtr;
};

#endif