#include"ServerContext.h"

#ifdef _IOCP

// 与远程服务器连接上后，进行处理
void* ServerContext::_DoConnectTask(void* data)
{
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx = socketCtx->serverCtx;
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
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx = socketCtx->serverCtx;
	//CmiLogger& log = serverCtx->log;

	SOCKADDR_IN* ClientAddr = NULL;
	SOCKADDR_IN* LocalAddr = NULL;
	int remoteLen = sizeof(SOCKADDR_IN), localLen = sizeof(SOCKADDR_IN);
	SocketType socketType = LISTEN_CLIENT_SOCKET;
	std::string* value = 0;
	int err = 0;
	SocketContext* newSocketCtx;

	//取得连入客户端的地址信息,通过 lpfnGetAcceptExSockAddrs
	//不但可以取得客户端和本地端的地址信息，还能顺便取出客户端发来的第一组数据
	
	serverCtx->lpfnGetAcceptExSockAddrs(
		ioCtx->packBuf.buf,
		0,
		sizeof(SOCKADDR_IN)+16,
		sizeof(SOCKADDR_IN)+16,
		(LPSOCKADDR*)&LocalAddr,
		&localLen,
		(LPSOCKADDR*)&ClientAddr,
		&remoteLen);


	err = setsockopt(
		ioCtx->sock,
		SOL_SOCKET,
		SO_UPDATE_ACCEPT_CONTEXT,	
		(char *)&(socketCtx->sock),
		sizeof(socketCtx->sock) );


	//LOG4CPLUS_INFO(log.GetInst(),"接受来自地址"<<inet_ntoa(ClientAddr->sin_addr)<<"的链接.");
	uint64_t timeStamp = GetTickCount64();
	newSocketCtx = serverCtx->CreateClientSocketCtx(timeStamp);
	newSocketCtx->sock = ioCtx->sock;
	newSocketCtx->socketType = socketType;
	newSocketCtx->UpdataTimeStamp(timeStamp);
	memcpy(&(newSocketCtx->sockAddr), ClientAddr, sizeof(SOCKADDR_IN));

	//LOG4CPLUS_INFO(log.GetInst(),"SocketCtx:"<<"["<<newSocketCtx<<"]"<< "时间戳:"<<newSocketCtx->timeStamp<<"ms");

	// 参数设置完毕，将这个Socket和完成端口绑定(这也是一个关键步骤)
	if (false == serverCtx->_AssociateWithIOCP(newSocketCtx))
	{
		serverCtx->_RemoveSocketContext(newSocketCtx);
		//LOG4CPLUS_ERROR(log.GetInst(), "绑定操作任务失败.重新投递Accept");
		goto end;
	}

	// 把这个有效的客户端信息，加入到ClientCtxList中去
	serverCtx->_AddToClientCtxList(newSocketCtx);
	//LOG4CPLUS_INFO(log.GetInst(),"SocketCtx:"<<"["<<newSocketCtx<<"]"<<"被添加到ClientCtxList表中");

	// 然后开始投递下一个WSARecv请求
	if (false == serverCtx->_PostRecv(newSocketCtx->GetNewIoContext()))
	{
		//LOG4CPLUS_INFO(log.GetInst(), "newSocketCtx:"<<"["<<socketCtx<<"]"<<"执行删除.");
		serverCtx->SendSocketAbnormal(newSocketCtx);
	}
	else
	{
		serverCtx->_AcceptedClientPack(newSocketCtx);
	}
	
end:
	// 投递新的AcceptEx
	if (false == serverCtx->_PostAccept(ioCtx))
	{
		//LOG4CPLUS_INFO(log.GetInst(), "Accept SocketCtx:"<<"["<<socketCtx<<"]"<<"执行删除.");
		serverCtx->SendSocketAbnormal(socketCtx);
	}

	return 0;
}


// 在有接收的数据到达的时候，进行处理
void* ServerContext::_DoRecvTask(void* data)
{
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx;

	if (socketCtx == 0)
		return 0;

	serverCtx = socketCtx->serverCtx;

	socketCtx->UpdataTimeStamp();
	//LOG4CPLUS_INFO(log.GetInst(),"来自地址"<<inet_ntoa(socketCtx->sockAddr.sin_addr)<<"的消息");

	socketCtx->SetPackContext((uint8_t*)ioCtx->buf, ioCtx->transferedBytes);
	serverCtx->_UnPack(socketCtx);

	socketCtx->ChangeToSplitState(ioCtx);

	// 然后开始投递下一个WSARecv请求
	if (false == serverCtx->_PostRecv(ioCtx))
	{
		//LOG4CPLUS_INFO(log.GetInst(), "Recv SocketCtx:"<<"["<<socketCtx<<"]"<<"执行删除.");
		serverCtx->SendSocketAbnormal(socketCtx);
		return 0;
	}

	return 0;
}

//已经发送完数据后调用处理
void* ServerContext::_DoSendTask(void* data)
{
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx;

	if (socketCtx == 0)
		return 0;

	serverCtx = socketCtx->serverCtx;
	//CmiLogger& log = serverCtx->log;

	socketCtx->ChangeToFreeState(ioCtx);
	return 0;
}


void* ServerContext::_ReAccpetTask(void* data)
{
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx = socketCtx->serverCtx;
	//CmiLogger& log = serverCtx->log;

	RELEASE_SOCKET(ioCtx->sock);
	ioCtx->ResetBuffer();

	if (false == serverCtx->_PostAccept(ioCtx)){
		//LOG4CPLUS_INFO(log.GetInst(), "Acept SocketCtx:"<<"["<<socketCtx<<"]"<<"执行删除.");
		serverCtx->SendSocketAbnormal(socketCtx);
	}

	return 0;
}

#endif
