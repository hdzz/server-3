#include "ThreadData.h"

#ifdef _EPOLL
ThreadData::ThreadData()
: epfd(-1)
, tps(0)
, fdCount(0)
,evs(0)
 ,evfdPos(0)
,evfdCount(0)
,eventBufPos(0)
, eventBufSize(0)
,event(EV_UNKNOWN)
{
    //ctor
}

ThreadData::~ThreadData()
{
    //dtor
}
#endif
