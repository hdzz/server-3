#include"ServerContext.h"

#ifdef _EPOLL

#define MAXEPOLLSIZE 20000
#define MAXEVENTS 64

void* ServerContext::_WorkerThread(void* pParam)
{
	THREADPARAMS_WORKER* param = (THREADPARAMS_WORKER*)pParam;
	ServerContext* serverCtx = (ServerContext*)param->serverCtx;
	CmiWaitSem& waitSem = serverCtx->waitSem;
	int nThreadNo = (int)param->nThreadNo;
	int epfd = serverCtx->epfd;

	struct epoll_event* evs = serverCtx->events;
	struct epoll_event ev;
	int events;
	//CmiLogger& log = serverCtx->log;
	SocketContext* socketCtx;
	int nfds, i;

	//LOG4CPLUS_INFO(log.GetInst(), "工作者线程启动，ID:" << nThreadNo);

	while(WAITSEM != waitSem.WaitSem(0))
	{
		//等待epoll事件的发生
		nfds = epoll_wait(epfd, evs, MAXEVENTS, -1);

		//处理所发生的所有事件
		for (i = 0; i < nfds; ++i)
		{
			ev = evs [i];
			events = ev.events;
			
			if (events & (EPOLLERR | EPOLLHUP))
			{
				socketCtx = (SocketContext*)(ev.data.ptr);
				//LOG4CPLUS_ERROR(log.GetInst(), "epoll_wait()非正常返回! 服务器将关闭问题SOCKET:" << socketCtx->sock);
				//LOG4CPLUS_ERROR(log.GetInst(), "这个可能是由于" << socketCtx->sock << "非正常关闭造成的.错误代码:" << errno);

				if (serverCtx->useMultithreading)
					serverCtx->_OfflineSocket(socketCtx);
				else
					serverCtx->_PostTask(_OfflineSocketTask, socketCtx);
				continue;
			}
			else if (events & EPOLLIN)
			{
				IoContext* ioCtx = (IoContext*)(ev.data.ptr);
				socketCtx = ioCtx->socketCtx;
		
				if (serverCtx->listenCtx == socketCtx) //如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接
				{
					serverCtx->_PostTask(_DoAcceptTask, socketCtx, ioCtx);
				}
				else if(socketCtx == serverCtx->quitSocketCtx)
				{
					_ModWithEpoll(ioCtx, EPOLLIN | EPOLLET | EPOLLONESHOT);
					break;
				}
				else
				{
					if(serverCtx->useMultithreading)
						serverCtx->_DoRecv(ioCtx);
					else
						serverCtx->_PostTask(_DoRecvTask, socketCtx, ioCtx);
				}
			}
			else if (events & EPOLLOUT) // 如果有数据发送
			{
				IoContext* ioCtx = (IoContext*)(ev.data.ptr);
				socketCtx = ioCtx->socketCtx;

				if(socketCtx->socketType ==  CONNECT_SERVER_SOCKET)
				{
				    int err;
				    socklen_t len;
					if(0 == getsockopt(socketCtx->sock, SOL_SOCKET, SO_ERROR, &err, &len)){
						if(err == 0){
							serverCtx->_PostTask(_DoConnectTask, socketCtx, ioCtx);
						}
					}
				}
				else
				{
					serverCtx->_PostTask(_DoSendTask, ioCtx->socketCtx, ioCtx);
				}
			}
		}
	}

	//LOG4CPLUS_TRACE(log.GetInst(), "工作者线程" <<nThreadNo<< "号退出.");

	// 释放线程参数
	RELEASE(param);

	return 0;
}

// 初始化完成端口
bool ServerContext::_InitializeEPOLL()
{

	epfd = epoll_create(MAXEPOLLSIZE);
	events = (struct epoll_event *)gcalloc (MAXEVENTS, sizeof(struct epoll_event ));

	pipe(fd);
	quitSocketCtx = new SocketContext(this);
	quitSocketCtx->sock = fd[0];
	IoContext* ioCtx = quitSocketCtx->GetNewIoContext();
	_AddWithEpoll(ioCtx, EPOLLIN | EPOLLET | EPOLLONESHOT);

	// 根据本机中的处理器数量，建立对应的线程数
	//nThreads = WORKER_THREADS_PER_PROCESSOR * GetNumProcessors();
	nThreads = 1;

	// 为工作者线程初始化句柄
	workerThreads = new pthread_t[nThreads];

	// 根据计算出来的数量建立工作者线程
	for (int i = 0; i < nThreads; i++)
	{
		THREADPARAMS_WORKER* pThreadParams = new THREADPARAMS_WORKER;
		pThreadParams->serverCtx = this;
		pThreadParams->nThreadNo = i + 1;

		pthread_create(&workerThreads[i], 0, _WorkerThread, (void*)pThreadParams);
	}

	return true;
}

int ServerContext::_SetNonBlocking(SOCKET sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1)
	{
        return -1;
    }
    return 0;
}

bool ServerContext::_AddWithEpoll(IoContext* ioCtx, __uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
	ev.data.ptr = ioCtx;
	ioCtx->socketCtx->events = events;

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, ioCtx->socketCtx->sock, &ev) < 0)
	{
       //LOG4CPLUS_ERROR(log.GetInst(), "执行epoll_ctl()出现错误.错误代码：");
        return false;
    }

	return true;
}

bool ServerContext::_ModWithEpoll(IoContext* ioCtx, __uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
	ev.data.ptr = ioCtx;
	ioCtx->socketCtx->events = events;

	if (epoll_ctl(epfd, EPOLL_CTL_MOD, ioCtx->socketCtx->sock, &ev) < 0)
	{
       //LOG4CPLUS_ERROR(log.GetInst(), "执行epoll_ctl()出现错误.");
        return false;
    }

	return true;
}


bool ServerContext::_DelWithEpoll(SocketContext* socketCtx)
{
	struct epoll_event event_del;
	event_del.data.fd = socketCtx->sock;
	event_del.events = 0;
	socketCtx->events = 0;

    if (epoll_ctl( epfd, EPOLL_CTL_DEL, socketCtx->sock, &event_del) < 0)
	{
       //LOG4CPLUS_ERROR(log.GetInst(), "执行epoll_ctl()出现错误.");
        return false;
    }

	return true;
}


// 投递接受请求
bool ServerContext::_PostAccept(IoContext* ioCtx)
{
	if (false == _ModWithEpoll(ioCtx, EPOLLIN | EPOLLET | EPOLLONESHOT))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "投递recv失败!");
		return false;
	}

	return true;
}


// 投递发送数据请求
void ServerContext::_PostSend(IoContext* ioCtx)
{
	SocketContext* socketCtx = ioCtx->socketCtx;
	socketCtx->AddToList(ioCtx);

	if (socketCtx->getIoCtxList().size() == 1)
	{
		if (false == _ModWithEpoll(ioCtx, EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT))
		{
			SendSocketAbnormal(socketCtx);
		}
	}

	return true;
}


// 投递接受数据请求
bool ServerContext::_PostRecv(IoContext* ioCtx)
{
	if (false == _ModWithEpoll(ioCtx, EPOLLIN | EPOLLET | EPOLLONESHOT))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "投递recv失败!");
		return false;
	}

	return true;
}


// 在有接收的数据到达的时候，进行处理
void ServerContext::_DoRecv(IoContext* ioCtx)
{
	SocketContext* socketCtx = ioCtx->socketCtx;
	char* buf = ioCtx->buf;
	int bufSize = ioCtx->packBuf.len;
	int count;
	int done = 0;

	socketCtx->UpdataTimeStamp();

	while (1)
	{
		count = read(socketCtx->sock, buf, bufSize);

		if (count == -1)
		{
			//如果errno ==EAGAIN,意思是我们读取了所有的数据，可以退出循环了
			if (errno != EAGAIN)
			{
				//LOG4CPLUS_ERROR(log.GetInst(), "read()出错!");
				done = 1;
			}
			break;
		}
		else if (count == 0)
		{
			//文件结束，远程已经关闭了连接
			done = 1;
			break;
		}

		socketCtx->SetPackContext((uint8_t*)buf, count);
		_UnPack(socketCtx);
	}

	if (!done)
	{
		//投递下一个WSARecv请求
		PostRecvPack(ioCtx);
	}
	else
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "关闭连接:"<< inet_ntoa(socketCtx->sockAddr.sin_addr));
		PostSocketAbnormal(socketCtx);
		return 0;
	}

	return 0;
}


void ServerContext::_OfflineSocket(SocketContext* socketCtx)
{
	if (socketCtx->isRelease == true)
		return;

	char buf[10];
	PackBuf wsabuf = { 10, buf };
	CreatePack_OffLine(&wsabuf);
	UnPack(socketCtx, (uint8_t*)buf);
	socketCtx->isRelease = true;

	PostRemoveSocketCtx(socketCtx);
}


#endif
