#include "EpollIO.h"

#ifdef _EPOLL

EpollIO::EpollIO()
:keyData(0)
,recvBytesTransfered(0)
,sendBytesTransfered(0)
,data(0)
,sock(-1)
,epfd(-1)
{
     postEventFD[0] = -1;
     postEventFD[1] = -1;
}

EpollIO::~EpollIO()
{

}

IoBuffers*  EpollIO::GetNewRecvIoBufs()
{
        IoBuffers*   p = 0;
        if(!recvIoBufsList.isempty())
        {
              void* i = recvIoBufsList.get_first_node();
               p = *recvIoBufsList.node(i);
               recvIoBufsList.delete_node(i);
        }
        return p;
}


IoBuffers*  EpollIO::GetNewSendIoBufs()
{
       IoBuffers*   p = 0;
        if(!sendIoBufsList.isempty())
        {
              void* i =sendIoBufsList.get_first_node();
               p = *sendIoBufsList.node(i);
              sendIoBufsList.delete_node(i);
        }
        return p;
}


void EpollIO::PushBackNewRecvIoBufs(IoBuffers* newIoBufs)
{
        recvIoBufsList.push_back(newIoBufs);
}

void EpollIO::PushFrontNewRecvIoBufs(IoBuffers* newIoBufs)
{
        recvIoBufsList.push_front(newIoBufs);
}


void EpollIO::PushBackNewSendIoBufs(IoBuffers* newIoBufs)
{
        sendIoBufsList.push_back(newIoBufs);
}

void EpollIO::PushFrontNewSendIoBufs(IoBuffers* newIoBufs)
{
        sendIoBufsList.push_front(newIoBufs);
}

IoBuffers*  EpollIO::CreateIoBuffers(LPWSABUF lpBuffers,  int buffersCount, void* data)
 {
        IoBuffers*  iobufs = (IoBuffers*)malloc(sizeof(IoBuffers));
        iobufs->lpwsabuf = lpBuffers;
        iobufs->bufCount = buffersCount;
        iobufs->data= data;
        iobufs->bufPos = 0;

        return iobufs;
 }

void EpollIO::ReleaseIoBuffers(IoBuffers* ioBufs)
{
       free(ioBufs);
}


void EpollIO::ReleaseSendIoBuffers(IoBuffers* ioBufs)
{
      for(int i  =0 ;i< ioBufs->bufCount; i++)
     {
         free(ioBufs->lpwsabuf[i].buf);
     }

     free(ioBufs->lpwsabuf);
     free(ioBufs);
}

#endif
