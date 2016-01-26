#ifndef EPOLLTOIOCP_H
#define EPOLLTOIOCP_H

#include"EpollBaseType.h"
#include"EpollIO.h"
#include"ThreadData.h"
#include"CmiWaitSem.h"
#include"CmiThreadLock.h"
//#include <iostream>
//#include "spdlog/spdlog.h"

#ifdef _EPOLL

class EpollToIocp
{
public:
    EpollToIocp(int mThreadSpace = 100);
     ~EpollToIocp();

    bool GetQueuedCompletionStatus(int& bytesTransfered, void** key, void** data);

    int BindSocket(int sock,  void* data);
    int WSAConnect(int sock, __CONST_SOCKADDR_ARG __addr, socklen_t __len,  WSABUF* buffer,  void* data);
    int WSAAccept(int sock, LPWSABUF lpBuffers,  int buffersCount, void* data);
    int WSASend(int sock, LPWSABUF lpBuffers,  int buffersCount, void* data);
    int WSARecv(int sock, LPWSABUF lpBuffers,  int buffersCount, void* data);
    int PostQueuedCompletionStatus();

    int PostEventData(EventType event,  int sock, void* data);
    int PostEventData(EventType event, int sock, LPWSABUF lpBuffers,  int buffersCount, void* data);
    int PostEventData(int postEventFD, EventType event, int sock, LPWSABUF lpBuffers,  int buffersCount, void* data);
    int PostEventData(int postEventFD, EventType event, int sock,  void* data);

    int CreatePostEventFD(int sock);
    int GetPostEventFD(int sock);


private:
       ThreadData* _InitThreadData();

       int  _EventProcess(EpollIO* epollIO);
       int _EventProcessListen(EpollIO* epollIO);
       int _EventProcessBindSocket(EpollIO* epollIO);
       int _EventProcessAddIoBufs(EpollIO* epollIO, EventType ev);
       int _EventProcessError(EpollIO* epollIO, EventType ev);
       int _EventProcessExit(EpollIO* epollIO);
       int _EventRead(int fd,  char** buf, int readSize);

      int _Listen(EpollIO* epollIO);
      int _Connect(EpollIO* epollIO);
      int  _Send(EpollIO* epollIO);
      int _Recv(EpollIO* epollIO);
      int _Error(EpollIO* epollIO);

      bool _ModAcceptOrRecvOrSend(EpollIO* epollIO);
      bool _EpollCtlByAdd(EpollIO* epollIO, __uint32_t events);
      bool _EpollCtlByMod(EpollIO* epollIO, __uint32_t events);
      bool _EpollCtlByDel(EpollIO* epollIO);

      int _SetNonBlocking(int sockfd);

      ThreadData* _GetLoadBalanceThreadData();

      	void _ReleaseSockPostEventFDMap(int sock);

      static void _freeKey (void *value);
      static void* _tpsThread(void* pParam);



  private:
        map<int, int>   sockPostEventFDMap;
        ThreadData**     threadDataList;
         int                          threadCount;
        int                            useThreadPos;
        ThreadData*          idleThreadData;
        pthread_key_t           key;
        pthread_t                    tpsThread;
        CmiThreadLock         sockmapLock;
        CmiWaitSem               waitSem;

        int sockCount;
        int enterBindFuncCount;
        int accpetSockCount;
};

#endif

#endif
