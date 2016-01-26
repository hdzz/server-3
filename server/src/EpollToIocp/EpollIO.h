#ifndef EPOLLIO_H
#define EPOLLIO_H

#include"EpollBaseType.h"

#ifdef _EPOLL

class EpollIO
{
    public:
        EpollIO();
        virtual ~EpollIO();

         IoBuffers* GetNewRecvIoBufs();
         IoBuffers* GetNewSendIoBufs();

         void PushBackNewRecvIoBufs(IoBuffers* newIoBufs);
         void PushFrontNewRecvIoBufs(IoBuffers* newIoBufs);
         void PushBackNewSendIoBufs(IoBuffers* newIoBufs);
          void PushFrontNewSendIoBufs(IoBuffers* newIoBufs);

          static IoBuffers* CreateIoBuffers(LPWSABUF lpBuffers,  int buffersCount, void* data);
          static void ReleaseIoBuffers(IoBuffers* ioBufs);
          static void ReleaseSendIoBuffers(IoBuffers* ioBufs);

    private:
     EpollIOType type;
     IoBuffersList  acceptIoBufsList;
     IoBuffersList  sendIoBufsList;
     IoBuffersList  recvIoBufsList;
     int sock;
     int epfd;
     int postEventFD[2];
     void* keyData;

     int recvBytesTransfered;
     int sendBytesTransfered;
     void* data;

     friend class EpollToIocp;
};

#endif

#endif // EPOLLIO_H
