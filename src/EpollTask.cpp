#include"ServerContext.h"

#ifdef _EPOLL

// ��Զ�̷����������Ϻ󣬽��д���
void* ServerContext::_DoConnectTask(void* data)
{
	TaskData* task = (TaskData*)data;
	ServerContext* serverCtx = task->serverCtx;
	SocketContext* socketCtx = task->socketCtx;
	IoContext* ioCtx = task->ioCtx;
	//CmiLogger& log = serverCtx->log;


	socketCtx->socketType = CONNECTED_SERVER_SOCKET;
	socketCtx->UpdataTimeStamp();
	socketCtx->ChangeToFreeState(ioCtx);

	serverCtx->_AddToRemoteServerInfoList(socketCtx->remoteServerInfo);

	//LOG4CPLUS_INFO(log.GetInst(),"SoctectCtx:"<<"["<<socketCtx<<"]"<< "��������Զ�̷�����."<<inet_ntoa(socketCtx->sockAddr.sin_addr));
	// Ȼ��ʼͶ����һ��WSARecv����
	if (false == serverCtx->_PostRecv(socketCtx->GetNewIoContext()))
	{
		//LOG4CPLUS_INFO(log.GetInst(), "Recv SocketCtx:"<<"["<<socketCtx<<"]"<<"ִ��ɾ��.");
		serverCtx->SendSocketAbnormal(socketCtx);
	}
	else
	{
		serverCtx->_ConnectedPack(socketCtx);
	}

	if (serverCtx->useWaitConnectRemoteServer)
		serverCtx->waitConnectRemoteServerSem.SetSem();

	return 0;
}

void* ServerContext::_DoAcceptTask(void* data)
{
	TaskData* task = (TaskData*)data;
	ServerContext* serverCtx = task->serverCtx;
	SocketContext* socketCtx = task->socketCtx;
	IoContext* ioCtx = task->ioCtx;
	//CmiLogger& log = serverCtx->log;

    SocketContext* newSocketCtx;
	SocketType socketType = LISTEN_CLIENT_SOCKET;
	int s;
	bool ret;

	while (1)
	{

		struct sockaddr in_addr;
		socklen_t in_len;
		SOCKET insock;

		char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
		in_len = sizeof in_addr;
		insock = accept(serverCtx->listenCtx->sock, &in_addr, &in_len);

		if (insock == -1)
		{
			if ((errno == EAGAIN) ||
				(errno == EWOULDBLOCK))
			{
				//�����Ѿ����������д��������
				return 0;
			}
			else
			{
				//LOG4CPLUS_ERROR(log.GetInst(), "Accept���ִ���!");
				break;
			}
		}

		s = getnameinfo(&in_addr, in_len,
			hbuf, sizeof (hbuf),
			sbuf, sizeof (sbuf),
			NI_NUMERICHOST | NI_NUMERICSERV);
		if (s == 0)
		{
		    //LOG4CPLUS_INFO(log.GetInst(),"�������Ե�ַ"<<hbuf<<":"<<sbuf<<"������.");
		}

		s = _SetNonBlocking(insock);
		if (s == -1)
		{
		    RELEASE_SOCKET(insock);
		}
		else
		{
			newSocketCtx = serverCtx->CreateClientSocketCtx();
			newSocketCtx->sock = insock;
			newSocketCtx->socketType = socketType;
			newSocketCtx->UpdataTimeStamp();
			memcpy(&(newSocketCtx->sockAddr), &in_addr, sizeof(struct sockaddr));

			// �������Ч�Ŀͻ�����Ϣ�����뵽ClientCtxList��ȥ
			serverCtx->_AddToClientCtxList(newSocketCtx);
			//LOG4CPLUS_INFO(log.GetInst(),"SocketCtx:"<<"["<<newSocketCtx<<"]"<<"����ӵ�ClientCtxList����");
		}

		// Ȼ��ʼͶ����һ��WSARecv����
		if (false == serverCtx->_PostRecv(newSocketCtx->GetNewIoContext()))
		{
			//LOG4CPLUS_INFO(log.GetInst(), "Recv SocketCtx:"<<"["<<socketCtx<<"]"<<"ִ��ɾ��.");
			serverCtx->SendSocketAbnormal(newSocketCtx);
		}
		else
		{
			serverCtx->_AcceptedClientPack(newSocketCtx);
		}
	}

	if (false == _PostAccept(ioCtx))
	{
		//LOG4CPLUS_INFO(log.GetInst(), "Accept SocketCtx:"<<"["<<socketCtx<<"]"<<"ִ��ɾ��.");
		serverCtx->SendSocketAbnormal(serverCtx->listenCtx);
	}
}

// ���н��յ����ݵ����ʱ�򣬽��д���
void* ServerContext::_DoRecvTask(void* data)
{
	TaskData* task = (TaskData*)data;
	ServerContext* serverCtx = task->serverCtx;
	SocketContext* socketCtx = task->socketCtx;
	IoContext* ioCtx = task->ioCtx;
	////CmiLogger& log = serverCtx->log;
	char* buf = ioCtx->buf;
	int bufSize = ioCtx->packBuf.len;
	int count;
	int done = 0;

	if (socketCtx->sock == INVALID_SOCKET)
	{
		serverCtx->_NotPassRecvPack(socketCtx);
		return 0;
	}

	socketCtx->UpdataTimeStamp();

	////LOG4CPLUS_INFO(log.GetInst(),"���Ե�ַ"<<inet_ntoa(socketCtx->sockAddr.sin_addr)<<"����Ϣ");

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
		serverCtx->_UnPack(socketCtx);
	}

	if (!done)
	{
	
		// Ͷ����һ��WSARecv����
		if (false == serverCtx->_PostRecv(ioCtx))
		{
			serverCtx->_AddToFreeList(ioCtx);
			serverCtx->SendSocketAbnormal(socketCtx);
			return 0;
		}
	}
	else
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "�ر�����:"<< inet_ntoa(socketCtx->sockAddr.sin_addr));
		serverCtx->SendSocketAbnormal(socketCtx);
		return 0;
	}

	return 0;
}

//����������
void* ServerContext::_DoSendTask(void* data)
{
	TaskData* task = (TaskData*)data;
	ServerContext* serverCtx = task->serverCtx;
	SocketContext* socketCtx = task->socketCtx;
	IoContext* ioCtx = task->ioCtx;

	//CmiLogger& log = serverCtx->log;
	int count;

	while (1)
	{
		count = send(socketCtx->sock, ioCtx->packBuf.buf, ioCtx->packBuf.len, 0);

		if (count < 0)
		{
			if (errno == EAGAIN ||
				errno == EWOULDBLOCK ||     //��ʾ��ǰ������д��
				errno == EINTR)            //��ʾ���ж���
			{
				return 0;
			}
			else
			{
				//LOG4CPLUS_ERROR(log.GetInst(), "�ر�����:" << inet_ntoa(socketCtx->sockAddr.sin_addr));
				serverCtx->SendSocketAbnormal(socketCtx);
				return 0;
			}
		}
		else if (count == ioCtx->packBuf.len)
		{
			socketCtx->ChangeToFreeState(ioCtx);
			break;
		}
		else
		{
			ioCtx->packBuf.buf = ioCtx->buf + count;
			ioCtx->packBuf.len -= count;
		}
	}

	if (!socketCtx->getIoCtxList().isempty())
	{
		ioCtx = *socketCtx->getIoCtxList().begin();
		if (false == _ModWithEpoll(ioCtx, EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT))
		{
			serverCtx->SendSocketAbnormal(socketCtx);
		}
	}

	return 0;
}

#endif



