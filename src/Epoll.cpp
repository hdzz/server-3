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

	//LOG4CPLUS_INFO(log.GetInst(), "�������߳�������ID:" << nThreadNo);

	while(WAITSEM != waitSem.WaitSem(0))
	{
		//�ȴ�epoll�¼��ķ���
		nfds = epoll_wait(epfd, evs, MAXEVENTS, -1);

		//�����������������¼�
		for (i = 0; i < nfds; ++i)
		{
			ev = evs [i];
			events = ev.events;
			
			if (events & (EPOLLERR | EPOLLHUP))
			{
				socketCtx = (SocketContext*)(ev.data.ptr);
				//LOG4CPLUS_ERROR(log.GetInst(), "epoll_wait()����������! ���������ر�����SOCKET:" << socketCtx->sock);
				//LOG4CPLUS_ERROR(log.GetInst(), "�������������" << socketCtx->sock << "�������ر���ɵ�.�������:" << errno);

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
		
				if (serverCtx->listenCtx == socketCtx) //����¼�⵽һ��SOCKET�û����ӵ��˰󶨵�SOCKET�˿ڣ������µ�����
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
			else if (events & EPOLLOUT) // ��������ݷ���
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

	//LOG4CPLUS_TRACE(log.GetInst(), "�������߳�" <<nThreadNo<< "���˳�.");

	// �ͷ��̲߳���
	RELEASE(param);

	return 0;
}

// ��ʼ����ɶ˿�
bool ServerContext::_InitializeEPOLL()
{

	epfd = epoll_create(MAXEPOLLSIZE);
	events = (struct epoll_event *)gcalloc (MAXEVENTS, sizeof(struct epoll_event ));

	pipe(fd);
	quitSocketCtx = new SocketContext(this);
	quitSocketCtx->sock = fd[0];
	IoContext* ioCtx = quitSocketCtx->GetNewIoContext();
	_AddWithEpoll(ioCtx, EPOLLIN | EPOLLET | EPOLLONESHOT);

	// ���ݱ����еĴ�����������������Ӧ���߳���
	//nThreads = WORKER_THREADS_PER_PROCESSOR * GetNumProcessors();
	nThreads = 1;

	// Ϊ�������̳߳�ʼ�����
	workerThreads = new pthread_t[nThreads];

	// ���ݼ�����������������������߳�
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
       //LOG4CPLUS_ERROR(log.GetInst(), "ִ��epoll_ctl()���ִ���.������룺");
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
       //LOG4CPLUS_ERROR(log.GetInst(), "ִ��epoll_ctl()���ִ���.");
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
       //LOG4CPLUS_ERROR(log.GetInst(), "ִ��epoll_ctl()���ִ���.");
        return false;
    }

	return true;
}


// Ͷ�ݽ�������
bool ServerContext::_PostAccept(IoContext* ioCtx)
{
	if (false == _ModWithEpoll(ioCtx, EPOLLIN | EPOLLET | EPOLLONESHOT))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "Ͷ��recvʧ��!");
		return false;
	}

	return true;
}


// Ͷ�ݷ�����������
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


// Ͷ�ݽ�����������
bool ServerContext::_PostRecv(IoContext* ioCtx)
{
	if (false == _ModWithEpoll(ioCtx, EPOLLIN | EPOLLET | EPOLLONESHOT))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "Ͷ��recvʧ��!");
		return false;
	}

	return true;
}


// ���н��յ����ݵ����ʱ�򣬽��д���
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
			//���errno ==EAGAIN,��˼�����Ƕ�ȡ�����е����ݣ������˳�ѭ����
			if (errno != EAGAIN)
			{
				//LOG4CPLUS_ERROR(log.GetInst(), "read()����!");
				done = 1;
			}
			break;
		}
		else if (count == 0)
		{
			//�ļ�������Զ���Ѿ��ر�������
			done = 1;
			break;
		}

		socketCtx->SetPackContext((uint8_t*)buf, count);
		_UnPack(socketCtx);
	}

	if (!done)
	{
		//Ͷ����һ��WSARecv����
		PostRecvPack(ioCtx);
	}
	else
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "�ر�����:"<< inet_ntoa(socketCtx->sockAddr.sin_addr));
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
