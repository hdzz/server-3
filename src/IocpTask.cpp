#include"ServerContext.h"

#ifdef _IOCP

// ��Զ�̷����������Ϻ󣬽��д���
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

	//ȡ������ͻ��˵ĵ�ַ��Ϣ,ͨ�� lpfnGetAcceptExSockAddrs
	//��������ȡ�ÿͻ��˺ͱ��ض˵ĵ�ַ��Ϣ������˳��ȡ���ͻ��˷����ĵ�һ������
	
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


	//LOG4CPLUS_INFO(log.GetInst(),"�������Ե�ַ"<<inet_ntoa(ClientAddr->sin_addr)<<"������.");
	uint64_t timeStamp = GetTickCount64();
	newSocketCtx = serverCtx->CreateClientSocketCtx(timeStamp);
	newSocketCtx->sock = ioCtx->sock;
	newSocketCtx->socketType = socketType;
	newSocketCtx->UpdataTimeStamp(timeStamp);
	memcpy(&(newSocketCtx->sockAddr), ClientAddr, sizeof(SOCKADDR_IN));

	//LOG4CPLUS_INFO(log.GetInst(),"SocketCtx:"<<"["<<newSocketCtx<<"]"<< "ʱ���:"<<newSocketCtx->timeStamp<<"ms");

	// ����������ϣ������Socket����ɶ˿ڰ�(��Ҳ��һ���ؼ�����)
	if (false == serverCtx->_AssociateWithIOCP(newSocketCtx))
	{
		serverCtx->_RemoveSocketContext(newSocketCtx);
		//LOG4CPLUS_ERROR(log.GetInst(), "�󶨲�������ʧ��.����Ͷ��Accept");
		goto end;
	}

	// �������Ч�Ŀͻ�����Ϣ�����뵽ClientCtxList��ȥ
	serverCtx->_AddToClientCtxList(newSocketCtx);
	//LOG4CPLUS_INFO(log.GetInst(),"SocketCtx:"<<"["<<newSocketCtx<<"]"<<"����ӵ�ClientCtxList����");

	// Ȼ��ʼͶ����һ��WSARecv����
	if (false == serverCtx->_PostRecv(newSocketCtx->GetNewIoContext()))
	{
		//LOG4CPLUS_INFO(log.GetInst(), "newSocketCtx:"<<"["<<socketCtx<<"]"<<"ִ��ɾ��.");
		serverCtx->SendSocketAbnormal(newSocketCtx);
	}
	else
	{
		serverCtx->_AcceptedClientPack(newSocketCtx);
	}
	
end:
	// Ͷ���µ�AcceptEx
	if (false == serverCtx->_PostAccept(ioCtx))
	{
		//LOG4CPLUS_INFO(log.GetInst(), "Accept SocketCtx:"<<"["<<socketCtx<<"]"<<"ִ��ɾ��.");
		serverCtx->SendSocketAbnormal(socketCtx);
	}

	return 0;
}


// ���н��յ����ݵ����ʱ�򣬽��д���
void* ServerContext::_DoRecvTask(void* data)
{
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx;

	if (socketCtx == 0)
		return 0;

	serverCtx = socketCtx->serverCtx;

	socketCtx->UpdataTimeStamp();
	//LOG4CPLUS_INFO(log.GetInst(),"���Ե�ַ"<<inet_ntoa(socketCtx->sockAddr.sin_addr)<<"����Ϣ");

	socketCtx->SetPackContext((uint8_t*)ioCtx->buf, ioCtx->transferedBytes);
	serverCtx->_UnPack(socketCtx);

	socketCtx->ChangeToSplitState(ioCtx);

	// Ȼ��ʼͶ����һ��WSARecv����
	if (false == serverCtx->_PostRecv(ioCtx))
	{
		//LOG4CPLUS_INFO(log.GetInst(), "Recv SocketCtx:"<<"["<<socketCtx<<"]"<<"ִ��ɾ��.");
		serverCtx->SendSocketAbnormal(socketCtx);
		return 0;
	}

	return 0;
}

//�Ѿ����������ݺ���ô���
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
		//LOG4CPLUS_INFO(log.GetInst(), "Acept SocketCtx:"<<"["<<socketCtx<<"]"<<"ִ��ɾ��.");
		serverCtx->SendSocketAbnormal(socketCtx);
	}

	return 0;
}

#endif
