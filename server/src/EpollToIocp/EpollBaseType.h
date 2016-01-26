#ifndef EPOLLBASETYPE_H
#define EPOLLBASETYPE_H

#if defined(_WIN32) || defined(_WIN64)

#undef _EPOLL

#ifndef _IOCP
#define _IOCP
#endif // _IOCP

#elif defined(_linux)
#undef _IOCP

#ifndef _EPOLL
#define _EPOLL
#endif

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
#include <signal.h>
#include"CmiVector.h"
#include<map>
#include <iostream>
using namespace std;

class EpollIO;

enum EventType
{
   EV_UNKNOWN,
   EV_BIND_SOCKET,
   EV_CONNECT,
   EV_ADD_NEW_ACCEPT_IOBUFS,
   EV_ADD_NEW_SEND_IOBUFS,
   EV_ADD_NEW_RECV_IOBUFS,
   EV_ACCEPT_ERROR,
   EV_RECV_ERROR,
   EV_SEND_ERROR,
   EV_EXIT
};

enum EpollIOType
{
         EPOLLIO_NOT_USED,
         EPOLLIO_PIPE_EVENT,
         EPOLLIO_LISTEN,
         EPOLLIO_RECV_AND_SEND,
         EPOLLIO_CONNECT
};

typedef struct _WSABUF {
    uint32_t len;
    char* buf;
} WSABUF,  *LPWSABUF;

struct IoBuffers
{
     LPWSABUF lpwsabuf;
     int  bufCount;
     int  bufPos;
     void* data;
};

class EpollIO;

typedef CmiVector<EpollIO*> EpollIOList;
typedef CmiVector<IoBuffers*> IoBuffersList;

#endif

#endif // EPOLLBASETYPE_H
