#ifndef THREADDATA_H
#define THREADDATA_H

#include"EpollBaseType.h"

#ifdef _EPOLL
class ThreadData
{
    public:
        ThreadData();
        virtual ~ThreadData();

    private:
     int epfd;
     int postEventFD[2];
     uint32_t  tps;
     int fdCount;
     map<int, EpollIO*>  sockmap;
     struct epoll_event* evs;
     int evfdPos;
     int evfdCount;

     //
     char eventBuf[128];
     int eventBufPos;
     int eventBufSize;
     EventType event;

     friend class EpollToIocp;
};

#endif
#endif // THREADDATA_H
