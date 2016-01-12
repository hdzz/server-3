#include"ServerContext.h"

#ifdef _EPOLL

// 与远程服务器连接上后，进行处理
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

	//LOG4CPLUS_INFO(log.GetInst(),"SoctectCtx:"<<"["<<socketCtx<<"]"<< "已连接上远程服务器."<<inet_ntoa(socketCtx->sockAddr.sin_addr));
	// 然后开始投递下一个WSARecv请求
	if (false == serverCtx->_PostRecv(socketCtx->GetNewIoContext()))
	{
		//LOG4CPLUS_INFO(log.GetInst(), "Recv SocketCtx:"<<"["<<socketCtx<<"]"<<"执行删除.");
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
				//我们已经处理了所有传入的连接
				return 0;
			}
			else
			{
				//LOG4CPLUS_ERROR(log.GetInst(), "Accept出现错误!");
				break;
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
		    RELEASE_SOCKET(insock);
		}
		else
		{
			newSocketCtx = serverCtx->CreateClientSocketCtx();
			newSocketCtx->sock = insock;
			newSocketCtx->socketType = socketType;
			newSocketCtx->UpdataTimeStamp();
			memcpy(&(newSocketCtx->sockAddr), &in_addr, sizeof(struct sockaddr));

			// 把这个有效的客户端信息，加入到ClientCtxList中去
			serverCtx->_AddToClientCtxList(newSocketCtx);
			//LOG4CPLUS_INFO(log.GetInst(),"SocketCtx:"<<"["<<newSocketCtx<<"]"<<"被添加到ClientCtxList表中");
		}

		// 然后开始投递下一个WSARecv请求
		if (false == serverCtx->_PostRecv(newSocketCtx->GetNewIoContext()))
		{
			//LOG4CPLUS_INFO(log.GetInst(), "Recv SocketCtx:"<<"["<<socketCtx<<"]"<<"执行删除.");
			serverCtx->SendSocketAbnormal(newSocketCtx);
		}
		else
		{
			serverCtx->_AcceptedClientPack(newSocketCtx);
		}
	}

	if (false == _PostAccept(ioCtx))
	{
		//LOG4CPLUS_INFO(log.GetInst(), "Accept SocketCtx:"<<"["<<socketCtx<<"]"<<"执行删除.");
		serverCtx->SendSocketAbnormal(serverCtx->listenCtx);
	}
}

// 在有接收的数据到达的时候，进行处理
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

	////LOG4CPLUS_INFO(log.GetInst(),"来自地址"<<inet_ntoa(socketCtx->sockAddr.sin_addr)<<"的消息");

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
		serverCtx->_UnPack(socketCtx);
	}

	if (!done)
	{
	
		// 投递下一个WSARecv请求
		if (false == serverCtx->_PostRecv(ioCtx))
		{
			serverCtx->_AddToFreeList(ioCtx);
			serverCtx->SendSocketAbnormal(socketCtx);
			return 0;
		}
	}
	else
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "关闭连接:"<< inet_ntoa(socketCtx->sockAddr.sin_addr));
		serverCtx->SendSocketAbnormal(socketCtx);
		return 0;
	}

	return 0;
}

//处理发送数据
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
				errno == EWOULDBLOCK ||     //表示当前缓冲区写满
				errno == EINTR)            //表示被中断了
			{
				return 0;
			}
			else
			{
				//LOG4CPLUS_ERROR(log.GetInst(), "关闭连接:" << inet_ntoa(socketCtx->sockAddr.sin_addr));
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



