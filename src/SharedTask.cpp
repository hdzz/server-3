#include"ServerContext.h"

void* ServerContext::_OfflineSocketTask(void* data)
{
	SocketContext* socketCtx = (SocketContext*)data;
	ServerContext* serverCtx = socketCtx->serverCtx;

	if (socketCtx->isRelease == true)
		return 0;

	char buf[10];
	PackBuf wsabuf = { 10, buf };
	serverCtx->CreatePack_OffLine(&wsabuf);
	serverCtx->UnPack(socketCtx, (uint8_t*)buf);
	socketCtx->isRelease = true;
	serverCtx->_RemoveSocketContext(socketCtx);

	return 0;
}

void* ServerContext::_PostSendTask(void* data)
{
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx = socketCtx->serverCtx;

	// Ȼ��ʼͶ����һ��WSARecv����
	if (false == serverCtx->_PostSend(ioCtx))
	{
		//LOG4CPLUS_INFO(log.GetInst(), "Recv SocketCtx:"<<"["<<socketCtx<<"]"<<"ִ��ɾ��.");
		serverCtx->SendSocketAbnormal(socketCtx);
	}

	return 0;
}

void* ServerContext::_PostRecvTask(void* data)
{
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;

	if (socketCtx == 0)
		return 0;

	ServerContext* serverCtx = socketCtx->serverCtx;

#ifdef _IOCP
	socketCtx->ChangeToSplitState(ioCtx);
#endif

	// Ȼ��ʼͶ����һ��WSARecv����
	if (false == serverCtx->_PostRecv(ioCtx))
	{
#ifdef _EPOLL
		serverCtx->_AddToFreeList(ioCtx);
#endif
		//LOG4CPLUS_INFO(log.GetInst(), "Recv SocketCtx:"<<"["<<socketCtx<<"]"<<"ִ��ɾ��.");
		serverCtx->SendSocketAbnormal(socketCtx);
	}

	return 0;
}

void* ServerContext::_PostRemoveSocketCtxTask(void* data)
{
	SocketContext* socketCtx = (SocketContext*)data;
	ServerContext* serverCtx = socketCtx->serverCtx;

	serverCtx->_RemoveSocketContext(socketCtx);

	return 0;
}

void* ServerContext::_CloseSocketTask(void* data)
{
	SocketContext* socketCtx = (SocketContext*)data;
	ServerContext* serverCtx = socketCtx->serverCtx;

	RELEASE_SOCKET(socketCtx->sock);
	return 0;
}


void* ServerContext::_ReleaseTask(void* data)
{
	free(data);
	return 0;
}

void* ServerContext::_ConnectRemoteServerTask(void* data)
{
	TaskData* taskTask = (TaskData*)data;
	ServerContext* serverCtx = taskTask->serverCtx;
	RemoteServerInfo* remoteServerInfo = (RemoteServerInfo*)taskTask->dataPtr;

	serverCtx->_ConnectRemoteServer(remoteServerInfo);
	return 0;
}

void* ServerContext::_HeartBeatTask_CheckChildren(void* data)
{
	ServerContext* serverCtx = (ServerContext*)data;

	if (serverCtx->isCheckChildrenHeartBeat)
	{
		serverCtx->_HeartBeat_CheckChildren();
		serverCtx->PostTimerTask(_HeartBeatTask_CheckChildren, 0, serverCtx, serverCtx->childLifeDelayTime);
	}

	return 0;
}

void* ServerContext::_HeartBeatTask_CheckServer(void* data)
{
	RemoteServerInfo* remoteServerInfo = (RemoteServerInfo*)data;
	SocketContext* socketCtx = remoteServerInfo->socketCtx;
	ServerContext* serverCtx;

	if (socketCtx == 0){
		delete remoteServerInfo;
		return 0;
	}

	serverCtx = socketCtx->serverCtx;

	if (serverCtx->isCheckServerHeartBeat)
	{
		serverCtx->_HeartBeat_CheckServer(remoteServerInfo);
		serverCtx->PostTimerTask(_HeartBeatTask_CheckServer, 0, remoteServerInfo, remoteServerInfo->lifeDelayTime);
	}

	return 0;
}


void* ServerContext::_SendHeartBeatTask(void* data)
{
	RemoteServerInfo* remoteServerInfo = (RemoteServerInfo*)data;
	SocketContext* socketCtx = remoteServerInfo->socketCtx;
	ServerContext* serverCtx;
	
	if (socketCtx == 0){
		delete remoteServerInfo;
		return 0;
	}

	serverCtx = socketCtx->serverCtx;

	if (serverCtx->isSendServerHeartBeat)
	{
		IoContext* ioCtx = socketCtx->GetNewIoContext();
		remoteServerInfo->CreateSendPack_HeartBeat(ioCtx);
		serverCtx->SendPack(ioCtx);

		serverCtx->PostTimerTask(_SendHeartBeatTask, 0, remoteServerInfo, remoteServerInfo->sendHeartBeatPackTime);
	}

	return 0;
}
