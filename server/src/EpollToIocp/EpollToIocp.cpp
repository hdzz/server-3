#include "EpollToIocp.h"

#ifdef _EPOLL

#define MAXEPOLLSIZE 20000
#define MAXEVENTS 64
EpollToIocp::EpollToIocp(int mThreadSpace)
: idleThreadData(0)
, threadCount(0)
,useThreadPos(0)
,sockCount(0)
,enterBindFuncCount(0)
,accpetSockCount(0)
{
     threadDataList = (ThreadData**)malloc(sizeof(ThreadData*) * mThreadSpace);
     pthread_key_create (&key, _freeKey);
     pthread_create(&tpsThread, 0, _tpsThread, (void*)this);

      //spd::rotating_logger_mt("file_logger", "logs/mylogfile", 1048576 * 5, 3,true);

}

EpollToIocp::~EpollToIocp()
{
    //dtor
}


bool EpollToIocp::GetQueuedCompletionStatus(int& bytesTransfered, void** key, void** data)
{
	struct epoll_event* evs;
	struct epoll_event ev;
	int events;
	EpollIO* epollIO;
	int i;
	ThreadData* threadData = _InitThreadData();
	bool isReturn = true;
	int  ret = 1;
	int type = 0;

	do{
		//等待epoll事件的发生
		while (threadData->evfdCount <= 0)
			threadData->evfdCount = epoll_wait(threadData->epfd, threadData->evs, MAXEVENTS, -1);

        ret = 1;
        //处理所发生的事件
		ev = threadData->evs[threadData->evfdPos++];
		events = ev.events;
		epollIO = (EpollIO*)(ev.data.ptr);

        if (events & EPOLLERR)
		{
        printf("EPOLLERR = %d, EPOLLHUP = %d \n",  events & (EPOLLERR), events & (EPOLLHUP));
		printf("errorCount = %d,  epollIO = %u,  epollIO->keydata = %u\n", ++sockCount, (int)epollIO, (int)(epollIO->keyData));
		   //spd::get("file_logger")->info() << "EnterGetQueuedCompletionStatus_EPOLLERR"  ;
           ret = _Error(epollIO);
		}
		else if (events & EPOLLHUP)
		{
        printf("EPOLLERR = %d, EPOLLHUP = %d \n",  events & (EPOLLERR), events & (EPOLLHUP));
		printf("errorCount = %d,  epollIO = %u,  epollIO->keydata = %u\n", ++sockCount, (int)epollIO, (int)(epollIO->keyData));
		   //spd::get("file_logger")->info() << "EnterGetQueuedCompletionStatus_EPOLLERR"  ;
           ret = 1;
		}
		else if(events & EPOLLRDHUP)
		{
             ret = _Error(epollIO);
		}
		else if (events & EPOLLIN)
		{
			switch (epollIO->type)
			{
			case EPOLLIO_LISTEN:
			//spd::get("file_logger")->info() << "EnterGetQueuedCompletionStatus_EPOLLIN_LISTEN"  ;
				ret = _Listen(epollIO);
				break;

			case EPOLLIO_PIPE_EVENT:
				//spd::get("file_logger")->info() << "EnterGetQueuedCompletionStatus_EPOLLIN_PIPE_EVENT"  ;
				ret = _EventProcess(epollIO);
				break;

			case EPOLLIO_RECV_AND_SEND:
			//spd::get("file_logger")->info() << "EnterGetQueuedCompletionStatus_EPOLLIN_RECV_AND_SEND"  ;
				ret = _Recv(epollIO);
				type = 1;
				break;
				default:
				//spd::get("file_logger")->info() << "EnterGetQueuedCompletionStatus_EPOLLIN_UNKNOWN"  ;
				break;
			}
		}
		else if (events & EPOLLOUT)
		{
		//spd::get("file_logger")->info() << "EnterGetQueuedCompletionStatus_EPOLLOUT"  ;
			switch (epollIO->type)
			{
			case EPOLLIO_RECV_AND_SEND:
				ret = _Send(epollIO);
				type = 0;
				break;

			case EPOLLIO_CONNECT:
				ret = _Connect(epollIO);
				type = 0;
				break;
			}
		}

		switch (ret)
        {
                case -1:
					 bytesTransfered = 0;
					*data = epollIO->data;
					*key = epollIO->keyData;
					epollIO->sendBytesTransfered = 0;
					epollIO->data = 0;
					ret = false;
					isReturn = true;
					break;

				case 0:
                    switch (type)
			         {
			            case 0:
                                    bytesTransfered = epollIO->sendBytesTransfered;
                                    epollIO->sendBytesTransfered = 0;
                                    break;
                          case 1:
                                    bytesTransfered = epollIO->recvBytesTransfered;
                                    epollIO->recvBytesTransfered = 0;
                                    break;
                     }
					*data = epollIO->data;
					*key = epollIO->keyData;
					epollIO->data = 0;
					ret = true;
					isReturn = true;
					break;

                case 1:
					isReturn = false;
					break;
        }

		if (threadData->evfdPos == threadData->evfdCount){
			threadData->evfdCount = 0;
			threadData->evfdPos  = 0;
			}

	}while (!isReturn);

	return ret;
}

int EpollToIocp::BindSocket(int sock,  void* data)
{
      int postEventFD = CreatePostEventFD(sock);
     return PostEventData(postEventFD, EV_BIND_SOCKET, sock, data);
}

int EpollToIocp::WSAConnect(int sock, __CONST_SOCKADDR_ARG __addr, socklen_t __len,  WSABUF* buffer,  void* data)
{
    int ret = connect(sock, __addr, __len);
	if(ret < 0){
		if( errno != EINPROGRESS ){
			return -1;
		}
	}

	ret = PostEventData(EV_CONNECT, sock,  buffer,  1, data);
    return ret;
}


int EpollToIocp::WSAAccept(int sock, LPWSABUF lpBuffers,  int buffersCount, void* data)
{
    int ret = PostEventData(EV_ADD_NEW_ACCEPT_IOBUFS, sock,  lpBuffers, buffersCount, data);
    return ret;
}

int EpollToIocp::WSASend(int sock, LPWSABUF lpBuffers,  int buffersCount, void* data)
{
    LPWSABUF cpy_lpwsabuf = (LPWSABUF)malloc(sizeof(WSABUF) * buffersCount);

    for(int i  =0 ;i< buffersCount; i++)
    {
            cpy_lpwsabuf[i].buf = (char*)malloc(sizeof(char) * lpBuffers[i].len);
            memcpy(cpy_lpwsabuf[i].buf, lpBuffers[i].buf, lpBuffers[i].len * sizeof(char));
            cpy_lpwsabuf[i].len =  lpBuffers[i].len ;
    }

    int ret = PostEventData(EV_ADD_NEW_SEND_IOBUFS, sock,  cpy_lpwsabuf , buffersCount, data);
    return ret;
}

int EpollToIocp::WSARecv(int sock, LPWSABUF lpBuffers,  int buffersCount, void* data)
{
    int ret = PostEventData(EV_ADD_NEW_RECV_IOBUFS, sock,  lpBuffers, buffersCount, data);
    return ret;
}

int EpollToIocp::PostQueuedCompletionStatus()
{
   int  ret;
   for(int i= 0; i<threadCount ;i++)
   {
       ret = PostEventData(threadDataList[i]->postEventFD[1], EV_EXIT,  -1,  0);
    }
    return 0;
}

int EpollToIocp::PostEventData(EventType event, int sock,  void* data)
{
        int postEventFD =GetPostEventFD(sock);

        if(postEventFD == -1)
             return - 1;

        return PostEventData(postEventFD, event, sock, data);
}

int EpollToIocp::PostEventData(EventType event, int sock, LPWSABUF lpBuffers,  int buffersCount, void* data)
{
     int postEventFD =GetPostEventFD(sock);

     if(postEventFD == -1)
             return - 1;

     return PostEventData(postEventFD, event, sock, lpBuffers,  buffersCount, data);
}

int EpollToIocp::PostEventData(int postEventFD, EventType event, int sock, LPWSABUF lpBuffers,  int buffersCount, void* data)
{
        char s[28] ={0};

        EventType* ev = (EventType*)s;
        *ev = event;

         int* psock= (int*)(s + sizeof(EventType));
        *psock = sock;

        LPWSABUF* pbufs = (LPWSABUF*)((char*)psock + sizeof(int));
        *pbufs = lpBuffers;

        int* pbufsCount = (int*)((char*)pbufs + sizeof(LPWSABUF));
        *pbufsCount = buffersCount;

        void** pdata = (void**)((char*)pbufsCount + sizeof(int));
        *pdata = data;

        int size =  sizeof(EventType)  + sizeof(int) + sizeof(LPWSABUF) + sizeof(int)  + sizeof(void*);
        int count = write(postEventFD , ev, size);

         if (count < 0)
		{
			if (errno == EAGAIN ||
				errno == EWOULDBLOCK ||     //表示当前缓冲区写满
				errno == EINTR)            //表示被中断了,但当收到信号时,可以继续写
			{
				return 1;
			}
			else  //发送出错，不可恢复
			{
				return -1;  //数据发送出现不可恢复错误
			}
		}

        return 0;
}

int EpollToIocp::PostEventData(int postEventFD, EventType event, int sock,  void* data)
{
        char s[20] ={0};
        int size;

         EventType* ev = (EventType*)s;
        *ev = event;

         int* psock= (int*)(s + sizeof(EventType));
        *psock = sock;

       if(data)
       {
            void** pdata = (void**)((char*)psock+ sizeof(int));
             *pdata = data;
             size =   sizeof(EventType) + sizeof(int)  + sizeof(void*);
        }
        else{
              size =   sizeof(EventType) + sizeof(int);
        }

        int count = write(postEventFD , ev, size);

        if (count < 0)
		{
			if (errno == EAGAIN ||
				errno == EWOULDBLOCK ||     //表示当前缓冲区写满
				errno == EINTR)            //表示被中断了,但当收到信号时,可以继续写
			{
				return 1;
			}
			else  //发送出错，不可恢复
			{
				return -1;  //数据发送出现不可恢复错误
			}
		}

        return 0;
}

int  EpollToIocp::GetPostEventFD(int sock)
{
      int postEventFD = -1;
      ThreadData* threadData;

      sockmapLock.Lock();

      map<int, int>::iterator it;
      it =  sockPostEventFDMap.find(sock) ;

      if(it == sockPostEventFDMap.end())
      {
            return -1;
      }
     else
    {
        postEventFD = it->second;
    }

      sockmapLock.UnLock();

      return postEventFD;
}

int  EpollToIocp::CreatePostEventFD(int sock)
{
      int postEventFD = -1;
      ThreadData* threadData;

      sockmapLock.Lock();

      threadData = _GetLoadBalanceThreadData();
      postEventFD = threadData->postEventFD[1];
      sockPostEventFDMap[sock] = postEventFD;

      sockmapLock.UnLock();

      return postEventFD;
}


void EpollToIocp::_ReleaseSockPostEventFDMap(int sock)
{
	sockmapLock.Lock();

	map<int, int>::iterator it;
	it =  sockPostEventFDMap.find(sock) ;
	if(it != sockPostEventFDMap.end())
		sockPostEventFDMap.erase(it);
	sockmapLock.UnLock();
}

ThreadData* EpollToIocp::_InitThreadData()
{
          	ThreadData* threadData = (ThreadData*)pthread_getspecific (key);

          	if(threadData)
              	return threadData;

           threadData =  new ThreadData();
           pthread_setspecific (key,  threadData);

            threadData->event = EV_UNKNOWN;
            threadData->eventBufPos = 0;

            threadData->epfd = epoll_create(MAXEPOLLSIZE);

            int* postEventFD = threadData->postEventFD;
            pipe(postEventFD);
            fcntl(postEventFD[0], F_SETFL, O_NONBLOCK);


            threadData->evs = (struct epoll_event *)calloc (MAXEVENTS, sizeof(struct epoll_event ));

            int tCount;
            tCount = __sync_add_and_fetch(&threadCount,1);
            threadDataList[tCount - 1] = threadData;

            EpollIO* epollIO = new EpollIO();
            epollIO->epfd = threadData->epfd;
            epollIO->sock = postEventFD[0];
            epollIO->type = EPOLLIO_PIPE_EVENT;
            epollIO->postEventFD[0] = postEventFD[0];
            epollIO->postEventFD[1] = postEventFD[1];
            threadData->sockmap[epollIO->sock] =  epollIO;

            _EpollCtlByAdd(epollIO, EPOLLIN);

            waitSem.SetSem();

            return threadData;
}

int  EpollToIocp::_EventProcess(EpollIO* epollIO)
{
        ThreadData* threadData = (ThreadData*)pthread_getspecific (key);
         char* buf = 0;
          EventType ev ;
          int ret = 0;
          int count;

          if(threadData->event == EV_UNKNOWN)
          {
                count = _EventRead(epollIO->postEventFD[0],  &buf,  sizeof(EventType));
                if(count == -1){ return -1;}
                else if(!buf){return 1;}
                ev =  *(EventType*)buf;
                threadData->event = ev;
          }
          else{
                ev = threadData->event;
          }

         switch(ev)
         {
          case EV_BIND_SOCKET:
               ret =  _EventProcessBindSocket(epollIO);
            break;

            case EV_ADD_NEW_ACCEPT_IOBUFS:
            case EV_ADD_NEW_SEND_IOBUFS:
            case EV_ADD_NEW_RECV_IOBUFS:
            case EV_CONNECT:
                ret =  _EventProcessAddIoBufs(epollIO, ev);
            break;

			case EV_SEND_ERROR:
			case EV_RECV_ERROR:
			ret = _EventProcessError(epollIO, ev);
			break;

			case EV_EXIT:
               ret = _EventProcessExit(epollIO);
            break;

			default:
			ret = 1;
			break;

         }

         return ret;
}


int EpollToIocp::_EventRead(int fd,  char** buf, int readSize)
{
         ThreadData* threadData = (ThreadData*)pthread_getspecific (key);
         int count = read(fd,  threadData->eventBuf + threadData->eventBufPos, readSize);

         *buf = 0;

         if(count == readSize)
         {
                 threadData->eventBufPos = 0;
                *buf = threadData->eventBuf;
                return count;
         }
		else if (count == -1)
		{
			//如果errno ==EAGAIN,意思是我们读取了所有的数据，可以退出循环了
			if (errno == EAGAIN)
			{
                threadData->eventBufPos += count;
			    threadData->eventBufSize = readSize - count;
			    return 1;
            }

			return -1; //读取出现不可恢复错误
		}
		else if (count == 0)
		{
			return -1; //远程已经关闭了连接
		}

        *buf = threadData->eventBuf;
		return count;
}


int EpollToIocp::_EventProcessBindSocket(EpollIO* epollIO)
{
          //spd::get("file_logger")->info() << "enterBindFuncCount=" <<++enterBindFuncCount ;

         ThreadData* threadData = (ThreadData*)pthread_getspecific (key);
         map<int, EpollIO*>& sockmap = threadData->sockmap;

        char* buf = 0;
        int readSize = sizeof(int) + sizeof(void*);

        if(threadData->eventBufPos > 0)
            readSize =  threadData->eventBufSize;

        int count = _EventRead(epollIO->postEventFD[0], &buf, readSize);

         if(count == -1){return -1;}
         else if(!buf){return 1;}
         threadData->event = EV_UNKNOWN;

         int sock =  *(int*)buf;
         void* data =  *(void**)(buf + sizeof(int));


          epollIO = new EpollIO();
          epollIO->epfd = threadData->epfd;
          epollIO->sock = sock;
          epollIO->type = EPOLLIO_NOT_USED;
          epollIO->keyData = data;
          sockmap[sock] = epollIO;

          //spd::get("file_logger")->info() << "sockCount=" <<sockCount ;


           printf("Bind_Data,  sock = %u\n",epollIO->sock);


           _EpollCtlByAdd(epollIO, EPOLLET);


          return 1;
}

int EpollToIocp::_EventProcessAddIoBufs(EpollIO* epollIO, EventType ev)
{
         ThreadData* threadData = (ThreadData*)pthread_getspecific (key);
          map<int, EpollIO*>& sockmap = threadData->sockmap;
          IoBuffers* iobufs;

         char* buf = 0;
         int readSize = sizeof(int) * 2 + sizeof(LPWSABUF) + sizeof(void*);

        if(threadData->eventBufPos > 0)
            readSize =  threadData->eventBufSize;

         int count = _EventRead(epollIO->postEventFD[0], &buf, readSize);
         if(count == -1){return -1;}
         else if(!buf){return 1;}
         threadData->event = EV_UNKNOWN;

         int sock =  *(int*)buf;
         LPWSABUF buffers =*(LPWSABUF*)(buf + sizeof(int));
         int buffersCount = *(int*)(buf + sizeof(int)  + sizeof(LPWSABUF));
         void* data =  *(void**)(buf + sizeof(int) * 2 + sizeof(LPWSABUF));


          map<int, EpollIO*>::iterator item;
          item =  sockmap.find(sock) ;

          if(item == sockmap.end()){
              return -2;   //sock没有绑定keydata
          }

          epollIO = item->second;
          iobufs = EpollIO::CreateIoBuffers(buffers, buffersCount, data);

          switch(ev)
          {
          case EV_ADD_NEW_ACCEPT_IOBUFS:
            epollIO->type = EPOLLIO_LISTEN;
            epollIO->PushBackNewRecvIoBufs(iobufs);
          break;

          case EV_ADD_NEW_SEND_IOBUFS:
            epollIO->type = EPOLLIO_RECV_AND_SEND;
            epollIO->PushBackNewSendIoBufs(iobufs);
          break;

          case EV_ADD_NEW_RECV_IOBUFS:
            epollIO->type = EPOLLIO_RECV_AND_SEND;
            epollIO->PushBackNewRecvIoBufs(iobufs);
          break;

          case EV_CONNECT:
              epollIO->type = EPOLLIO_CONNECT;
              epollIO->PushBackNewSendIoBufs(iobufs);
          break;
          }

            _ModAcceptOrRecvOrSend(epollIO);
            return 1;
}

int EpollToIocp::_EventProcessError(EpollIO* epollIO, EventType ev)
{
     //spd::get("file_logger")->info() << "_EventProcessError" ;
	ThreadData* threadData = (ThreadData*)pthread_getspecific(key);
	map<int, EpollIO*>& sockmap = threadData->sockmap;

	char* buf = 0;
	int readSize = sizeof(int) ;

	if(threadData->eventBufPos > 0)
		readSize =  threadData->eventBufSize;

	int count = _EventRead(epollIO->postEventFD[0], &buf, readSize);

	if(count == -1){return -3;}
	else if(!buf){return 1;}
	threadData->event = EV_UNKNOWN;

	int sock =  *(int*)buf;

	map<int, EpollIO*>::iterator item;
	item =  sockmap.find(sock);

	if(item == sockmap.end()){
		return 1;   //sock
	}

	IoBuffers* ioBufs;
	EpollIO* errorEpollIO = item->second;
	epollIO->keyData = errorEpollIO->keyData;

	switch(ev)
	{
	case EV_RECV_ERROR:
		ioBufs = errorEpollIO->GetNewRecvIoBufs();
		EpollIO::ReleaseIoBuffers(ioBufs);
		epollIO->data = ioBufs->data;

		if(!errorEpollIO->recvIoBufsList.isempty())
			PostEventData(threadData->postEventFD[1], EV_RECV_ERROR, errorEpollIO->sock, 0);
		break;

	case EV_SEND_ERROR:
		ioBufs = errorEpollIO->GetNewSendIoBufs();
		EpollIO::ReleaseSendIoBuffers(ioBufs);
		epollIO->data = ioBufs->data;

		if(!errorEpollIO->sendIoBufsList.isempty())
			PostEventData(threadData->postEventFD[1], EV_SEND_ERROR, errorEpollIO->sock, 0);
		break;
	}

	if(errorEpollIO->recvIoBufsList.isempty() &&
		errorEpollIO->sendIoBufsList.isempty())
	{
		sockmap.erase(item);
		_ReleaseSockPostEventFDMap(errorEpollIO->sock);
		delete errorEpollIO;
	}

	return -1;
}

int EpollToIocp::_EventProcessExit(EpollIO* epollIO)
{
//spd::get("file_logger")->info() << "_EventProcessExit" ;
        ThreadData* threadData = (ThreadData*)pthread_getspecific(key);
         map<int, EpollIO*>& sockmap = threadData->sockmap;

        char* buf = 0;
         int readSize = sizeof(int) ;

          if(threadData->eventBufPos > 0)
            readSize =  threadData->eventBufSize;

         int count = _EventRead(epollIO->postEventFD[0], &buf, readSize);
         if(count == -1){return -1;}
         else if(!buf){return 1;}
         threadData->event = EV_UNKNOWN;

         epollIO->keyData = 0;
         _EpollCtlByMod(epollIO, 0);
        return 0;
}

int EpollToIocp::_Listen(EpollIO* epollIO)
{
    ThreadData* threadData = (ThreadData*)pthread_getspecific(key);
	int s;
	bool ret;

		struct sockaddr in_addr;
		socklen_t in_len;
		int insock;

		char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
		in_len = sizeof in_addr;

       //spd::get("file_logger")->info() << "accept" ;
		insock = accept(epollIO->sock, &in_addr, &in_len);

		if (insock == -1)
		{
			if ((errno == EAGAIN) ||
				(errno == EWOULDBLOCK))
			{
				//我们已经处理了所有传入的连接
				//spd::get("file_logger")->info() << "listen error!!" ;
				return 1;
			}
			else
			{
				//Accept出现错误!
				//spd::get("file_logger")->info() << "listen error!!" ;
				return 1;
			}
		}

		s = getnameinfo(&in_addr, in_len,
			hbuf, sizeof (hbuf),
			sbuf, sizeof (sbuf),
			NI_NUMERICHOST | NI_NUMERICSERV);
		if (s == 0)
		{
		    //LOG4CPLUS_INFO(log.GetInst(),"接受来自地址"<<hbuf<<":"<<sbuf<<"的链接.");
		}

		s = _SetNonBlocking(insock);
		if (s == -1)
		{
		   //spd::get("file_logger")->info() << "listen error!!" ;
		    close(insock);
		    return 1;
		}
		else
		{
		    //spd::get("file_logger")->info() << "accpetSockCount=" <<++accpetSockCount ;
                IoBuffers* ioBufs = epollIO->GetNewRecvIoBufs();
	            WSABUF* wsabuf = &(ioBufs->lpwsabuf[0]);
	            char* buf =  wsabuf->buf;
	            int* psock = (int*)buf;
	            *psock = insock;

                epollIO->sendBytesTransfered = sizeof(int);
	            epollIO->data = ioBufs->data;
				EpollIO::ReleaseIoBuffers(ioBufs);
				_ModAcceptOrRecvOrSend(epollIO);
				return 0;
		}

       //spd::get("file_logger")->info() << "listen error!!" ;
		return 1;
}


int EpollToIocp::_Connect(EpollIO* epollIO)
{
       int err;
		socklen_t len;
		if(0 == getsockopt(epollIO->sock, SOL_SOCKET, SO_ERROR, &err, &len))
		{
		      if(err == 0)
		      {
		             epollIO->type = EPOLLIO_RECV_AND_SEND;
		             return  _Send(epollIO);
              }
        }

		return 1;
}

int EpollToIocp::_Send(EpollIO* epollIO)
{
	ThreadData* threadData = (ThreadData*)pthread_getspecific(key);

	//获取一个发送iobufs(iobufs中携带了要发送的数据)
	IoBuffers* ioBufs = epollIO->GetNewSendIoBufs();
	int i = ioBufs->bufPos;
	int bufCount = ioBufs->bufCount;
	WSABUF* wsabuf = &(ioBufs->lpwsabuf[i]);
	int count;

	while (1)
	{
		count = send(epollIO->sock, wsabuf->buf, wsabuf->len, 0);

		if(count >= 0){
			threadData->tps += count; //线程吞吐量累加发送字节数量
			epollIO->sendBytesTransfered += count; //累加发送字节数量
		}

		if (count < 0)
		{
			if (errno == EAGAIN ||
				errno == EWOULDBLOCK ||     //表示当前缓冲区写满
				errno == EINTR)            //表示被中断了,但当send收到信号时,可以继续写
			{
				//ioBufs未被完整发送，重新添加到epollio的sendiobufslist中，
				//等待epoll的发送信号继续发送iobufs中剩余的数据
				epollIO->PushFrontNewRecvIoBufs(ioBufs);
				return 1;   //数据未发送完全，还需要继续发送
			}
			else  //发送出错，不可恢复
			{
				epollIO->data = ioBufs->data;
				EpollIO::ReleaseIoBuffers(ioBufs);

				if(!epollIO->sendIoBufsList.isempty())
					PostEventData(threadData->postEventFD[1], EV_SEND_ERROR, epollIO->sock, 0);

				return -1;  //数据发送出现不可恢复错误
			}
		}
		//ioBufs中的一个buf完整发送完成，获取其中的下一个(如果存在的话)
		else if (count == wsabuf->len)
		{
			//依次获取ioBufs中其它buf进行发送
			if(--bufCount > 0) {
				i++;
				wsabuf = &(ioBufs->lpwsabuf[i]);
				ioBufs->bufPos = i;
			}
			else{ //buf全部发送完成
				epollIO->data = ioBufs->data;
				EpollIO::ReleaseSendIoBuffers(ioBufs);
				_ModAcceptOrRecvOrSend(epollIO);
				break;
			}
		}
		else
		{
			//根据当前buf发送数据数量count,移动buf的位置
			wsabuf->buf += count;
			wsabuf->len -= count;
		}
	}

	return 0; //当前获取的ioBufs数据包完全发送成功
}

int EpollToIocp::_Recv(EpollIO* epollIO)
{
	ThreadData* threadData = (ThreadData*)pthread_getspecific(key);

	//获取一个接受数据的iobufs(iobufs用来存放对端发送过来的数据)
	IoBuffers* ioBufs = epollIO->GetNewRecvIoBufs();
	int i = ioBufs->bufPos;
	int bufCount = ioBufs->bufCount;
	WSABUF* wsabuf = &(ioBufs->lpwsabuf[i]);

	char* buf =  wsabuf->buf;
	int bufSize = wsabuf->len;
	int orgSize = wsabuf->len;
	int count;
	int bufReadLen = 0;

	//spd::get("file_logger")->info() << "_Recv"  ;
	//spd::get("file_logger")->info()<<"start epollIO->recvBytesTransfered = " <<epollIO->recvBytesTransfered ;

	while (1)
	{
		count = read(epollIO->sock, buf, bufSize);

        //spd::get("file_logger")->info() << "_Recv() count="<< count  ;

		if (count == -1)
		{
			//如果errno ==EAGAIN,意思是我们读取了所有的数据，可以退出循环了
			if (errno == EAGAIN){
			 //spd::get("file_logger")->info() << "_Recv() errorn == EAGAIN" ;
				break;
				}

         //spd::get("file_logger")->info() << "_Recv() errorn  != EAGAIN" ;
			epollIO->data = ioBufs->data;
			EpollIO::ReleaseIoBuffers(ioBufs);

			if(!epollIO->recvIoBufsList.isempty())
				PostEventData(threadData->postEventFD[1], EV_RECV_ERROR, epollIO->sock, 0);

			return -1; //读取出现不可恢复错误
		}
		else if (count == 0)
		{
			epollIO->data = ioBufs->data;
			EpollIO::ReleaseIoBuffers(ioBufs);

			if(!epollIO->recvIoBufsList.isempty())
				PostEventData(threadData->postEventFD[1], EV_RECV_ERROR, epollIO->sock, 0);

			return -1; //远程已经关闭了连接
		}

		bufReadLen += count;

		if(threadData){
			threadData->tps += count;
			epollIO->recvBytesTransfered += count;
		}

		//根据当前buf接受的数据数量count,移动buf的位置到空位
		buf += count;
		bufSize = orgSize - bufReadLen;

		//ioBufs中的一个buf空间已完全接受了数据，获取ioBufs的下一个wsabuf(如果存在的话)
		if(bufSize == 0)
		{
			if(--bufCount > 0)
			{
				i++;
				wsabuf = &(ioBufs->lpwsabuf[i]);
				ioBufs->bufPos = i;
				orgSize = wsabuf->len;
				bufSize = wsabuf->len;
				wsabuf->len = 0;
			}
			else{
				break;
			}
		}
	}

    //spd::get("file_logger")->info()<<"epollIO->recvBytesTransfered = " <<epollIO->recvBytesTransfered ;
	epollIO->data = ioBufs->data;
	EpollIO::ReleaseIoBuffers(ioBufs);
	_ModAcceptOrRecvOrSend(epollIO);
	return 0;
}

int EpollToIocp::_Error(EpollIO* epollIO)
{
  ThreadData* threadData = (ThreadData*)pthread_getspecific(key);

     _EpollCtlByDel(epollIO);

    if(!epollIO->recvIoBufsList.isempty())
        PostEventData(threadData->postEventFD[1], EV_RECV_ERROR, epollIO->sock, 0);

   if(!epollIO->sendIoBufsList.isempty())
       PostEventData(threadData->postEventFD[1], EV_SEND_ERROR, epollIO->sock, 0);

    return 1;
}

bool EpollToIocp::_ModAcceptOrRecvOrSend(EpollIO* epollIO)
{
     __uint32_t events = 0;

     if(!epollIO->recvIoBufsList.isempty())
    {
            events |= EPOLLIN;
    }

      if(!epollIO->sendIoBufsList.isempty())
      {
        events |= EPOLLOUT;
      }

      if(events == 0)
      {
          events = EPOLLET;
      }

      return _EpollCtlByMod(epollIO, events);
}

bool EpollToIocp::_EpollCtlByAdd(EpollIO* epollIO, __uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
	ev.data.ptr = epollIO;

	if (epoll_ctl(epollIO->epfd, EPOLL_CTL_ADD, epollIO->sock, &ev) < 0)
	{
        return false;
    }

	return true;
}

bool EpollToIocp::_EpollCtlByMod(EpollIO* epollIO, __uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
	ev.data.ptr = epollIO;

	if (epoll_ctl(epollIO->epfd, EPOLL_CTL_MOD, epollIO->sock, &ev) < 0)
	{
        return false;
    }

	return true;
}

bool EpollToIocp::_EpollCtlByDel(EpollIO* epollIO)
{
	struct epoll_event event_del;
	event_del.events = 0;
	event_del.data.ptr = epollIO;

    if (epoll_ctl(epollIO->epfd, EPOLL_CTL_DEL, epollIO->sock, &event_del) < 0)
	{
        return false;
    }

	return true;
}

int EpollToIocp::_SetNonBlocking(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1)
	{
        return -1;
    }
    return 0;
}

void EpollToIocp::_freeKey (void *value)
{
     ThreadData* threadData = (ThreadData*)value;
     delete threadData;
}

ThreadData* EpollToIocp::_GetLoadBalanceThreadData()
{
     if(useThreadPos >= threadCount)
            useThreadPos = 0;

        if(threadDataList[useThreadPos] == 0)
                  waitSem.WaitSem( -1);

      return threadDataList[useThreadPos++];

/*
     if(idleThreadData == 0){

        if(threadCount == 0)
                  waitSem.WaitSem( -1);
           idleThreadData = threadDataList[0];
       }
    return  idleThreadData;
    */
}


void* EpollToIocp::_tpsThread(void* pParam)
{
     EpollToIocp* epollToIocp = (EpollToIocp*)pParam;
    ThreadData** threadDataList ;
     ThreadData*  idleThreadData;
    int threadCount;
    ThreadData* threadData;

     while(1)
     {
         sleep(12);

/*
       threadDataList =  epollToIocp->threadDataList;
      idleThreadData =  epollToIocp->idleThreadData;
     threadCount = epollToIocp->threadCount;

        for(int i= 0; i < threadCount; i++)
        {
           threadData =  threadDataList[i];

             if( idleThreadData->tps < threadData->tps  &&
                  threadData->tps <  idleThreadData->tps +1000)
            {
                   if(threadData->fdCount < idleThreadData->fdCount){
                         idleThreadData =  threadData;
                         continue;
                    }
            }
            else if( idleThreadData->tps > threadData->tps){
                  idleThreadData =  threadData;
             }else{
                  threadData->tps = 0;
            }
        }

        if(idleThreadData){
            idleThreadData->tps = 0;
            epollToIocp->idleThreadData = idleThreadData;
            }
            */
     }
}

#endif
