#include"ServerContext.h"

// 与远程服务器连接上后，进行处理
void* ServerContext::_DoConnectTask(void* data)
{
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx = socketCtx->serverCtx;

	socketCtx->socketType = CONNECTED_SERVER_SOCKET;
	socketCtx->UpdataTimeStamp();

	serverCtx->_AddToRemoteServerInfoList(socketCtx->remoteServerInfo);

	if (ioCtx->packBuf.len != MAX_BUFFER_LEN){
		ReleaseIoContext(ioCtx);
		ioCtx = CreateIoContext();
		ioCtx->sock = socketCtx->sock;
		ioCtx->socketCtx = socketCtx;
		ioCtx->serverCtx = serverCtx;
	}

	if (false == serverCtx->_PostRecv(ioCtx))
	{
		ReleaseIoContext(ioCtx);
		serverCtx->SendSocketAbnormal(socketCtx);
	}
	else
	{
		serverCtx->_ConnectedPack(socketCtx);
	}
	return 0;
}

void* ServerContext::_DoAcceptTask(void* data)
{
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx = socketCtx->serverCtx;

	// 把这个有效的客户端信息，加入到ClientCtxList中去
	serverCtx->_AddToClientCtxList(socketCtx);

	if (false == serverCtx->_PostRecv(ioCtx))
	{
		ReleaseIoContext(ioCtx);
		serverCtx->PostSocketAbnormal(socketCtx);
	}

	return 0;
}

void* ServerContext::_PostSendTask(void* data)
{
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx = ioCtx->serverCtx;

	if (false == serverCtx->_PostSend(ioCtx))
	{
		ReleaseIoContext(ioCtx);
		serverCtx->SendSocketAbnormal(socketCtx);
	}
	return 0;
}

void* ServerContext::_PostRecvTask(void* data)
{
    IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx = ioCtx->serverCtx;

	 //spd::get("file_logger")->info() << "_PostRecvTask";

	if (false == serverCtx->_PostRecv(ioCtx))
	{
		ReleaseIoContext(ioCtx);
		serverCtx->SendSocketAbnormal(socketCtx);
	}

	return 0;
}


void* ServerContext::_PostErrorTask(void* data)
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

		if (false == serverCtx->_PostSend(ioCtx))
		{
			ReleaseIoContext(ioCtx);
			serverCtx->SendSocketAbnormal(socketCtx);
			return 0;
		}

		serverCtx->PostTimerTask(_SendHeartBeatTask, 0, remoteServerInfo, remoteServerInfo->sendHeartBeatPackTime);
	}

	return 0;
}

void* ServerContext::_ReleaseTask(void* data)
{
	free(data);
	return 0;
}
