#include"ServerContext.h"


// 与远程服务器连接上后，进行处理
void* ServerContext::_DoConnectTask(void* data)
{
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx = socketCtx->serverCtx;

	socketCtx->socketType = CONNECTED_SERVER_SOCKET;
	socketCtx->UpdataTimeStamp();

	serverCtx->sockmap[ioCtx->sock] = socketCtx;
	serverCtx->_CreateCheckRemoteServerTimer(socketCtx);

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
		serverCtx->UnPack(EV_SOCKET_CONNECTED, socketCtx, 0);
	}
	return 0;
}

void* ServerContext::_DoAcceptTask(void* data)
{
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx = ioCtx->serverCtx;
	CmiVector<SOCKET>& reusedSocketList = serverCtx->reusedSocketList;

	// 把这个有效的客户端信息，加入到ClientCtxList中去
	serverCtx->sockmap[socketCtx->sock] = socketCtx;

	if (false == serverCtx->_PostRecv(ioCtx))
	{
		ReleaseIoContext(ioCtx);
		serverCtx->PostSocketAbnormal(socketCtx);
	}

	return 0;
}

#ifdef _IOCP
void* ServerContext::_DoSendTask(void* data)
{
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx = ioCtx->serverCtx;
	IoCtxList& sendList = socketCtx->sendList;

	serverCtx->UnPack(EV_PACK_SEND, socketCtx, (uint8_t*)ioCtx->buf);
	
	ReleaseIoContext(ioCtx);
	sendList.delete_node(sendList.get_first_node());

	
	IoContext** pIoCtx = sendList.begin();

	if (pIoCtx)
	{
		if (false == serverCtx->_PostSend(*pIoCtx))
		{
			serverCtx->SendSocketAbnormal(socketCtx);
		}
	}

	return 0;
}

void* ServerContext::_PostAcceptTask(void* data)
{
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx = ioCtx->serverCtx;

	// 投递新的AcceptEx
	ioCtx->sock = serverCtx->_Socket();
	if (false == serverCtx->_PostAccept(ioCtx))
	{
		ReleaseIoContext(ioCtx);
		serverCtx->PostSocketAbnormal(socketCtx);
	}

	return 0;
}

void* ServerContext::_PostInitAcceptTask(void* data)
{
	SocketContext* listenCtx = (SocketContext*)data;
	ServerContext* serverCtx = listenCtx->serverCtx;
	IoContext* ioCtx;

	// 为AcceptEx 准备参数，然后投递AcceptEx I/O请求
	for (int i = 0; i < MAX_POST_ACCEPT; i++)
	{
		// 新建一个IO_CONTEXT
		ioCtx = listenCtx->GetNewIoContext((sizeof(SOCKADDR_IN)+16) * 2);

		// 为以后新连入的客户端先准备好Socket
		ioCtx->sock = serverCtx->_Socket();
		if (false == serverCtx->_PostAccept(ioCtx))
		{
			ReleaseIoContext(ioCtx);
		}
	}

	return 0;
}
#endif



void* ServerContext::_PostSendTask(void* data)
{
	IoContext* ioCtx = (IoContext*)data;
	SocketContext* socketCtx = ioCtx->socketCtx;
	ServerContext* serverCtx = ioCtx->serverCtx;

#ifdef _IOCP
	IoCtxList& sendList = socketCtx->sendList;

	sendList.push_back(ioCtx);

	if (sendList.size() == 1)
	{
		if (false == serverCtx->_PostSend(ioCtx))
		{
			ReleaseIoContext(ioCtx);
			serverCtx->SendSocketAbnormal(socketCtx);
		}
	}
#endif 

#ifdef _EPOLL
	if (false == serverCtx->_PostSend(ioCtx))
	{
		ReleaseIoContext(ioCtx);
		serverCtx->SendSocketAbnormal(socketCtx);
	}
#endif

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
	TaskData* taskData = (TaskData*)data;
	ServerContext* serverCtx = taskData->serverCtx;
	SOCKET sock = taskData->sock;
	map<SOCKET, SocketContext*>& sockmap = serverCtx->sockmap;
	map<SOCKET, SocketContext*>::iterator item;
	SocketContext* socketCtx;

	item = sockmap.find(sock);
	if (item == sockmap.end()){
		return 0;  
	}
	socketCtx = item->second;

	serverCtx->UnPack(EV_SOCKET_OFFLINE, socketCtx, 0);

	sockmap.erase(item);
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
		serverCtx->PostTimerTask(_HeartBeatTask_CheckChildren, 0, serverCtx, serverCtx->childLifeDelayTime - 1000);
	}

	return 0;
}

void* ServerContext::_HeartBeatTask_CheckServer(void* data)
{
	TaskData* taskData = (TaskData*)data;
	ServerContext* serverCtx = taskData->serverCtx;
	SOCKET sock = taskData->sock;
	map<SOCKET, SocketContext*>& sockmap = serverCtx->sockmap;
	map<SOCKET, SocketContext*>::iterator item;
	SocketContext* socketCtx;
	RemoteServerInfo* remoteServerInfo;

	item = sockmap.find(sock);
	if (item == sockmap.end()){
		return 0;
	}
	socketCtx = item->second;
	remoteServerInfo = socketCtx->remoteServerInfo;


	if (serverCtx->isCheckServerHeartBeat)
	{
		serverCtx->_HeartBeat_CheckServer(remoteServerInfo);

		TaskData* data = (TaskData*)malloc(sizeof(TaskData));
		data->serverCtx = serverCtx;
		data->sock = sock;
		serverCtx->PostTimerTask(_HeartBeatTask_CheckServer, _ReleaseTask, data, remoteServerInfo->lifeDelayTime - 100);
	}

	return 0;
}


void* ServerContext::_SendHeartBeatTask(void* data)
{
	TaskData* taskData = (TaskData*)data;
	ServerContext* serverCtx = taskData->serverCtx;
	SOCKET sock = taskData->sock;
	map<SOCKET, SocketContext*>& sockmap = serverCtx->sockmap;
	map<SOCKET, SocketContext*>::iterator item;
	SocketContext* socketCtx;
	RemoteServerInfo* remoteServerInfo;

	item = sockmap.find(sock);
	if (item == sockmap.end()){
		return 0;
	}
	socketCtx = item->second;
	remoteServerInfo = socketCtx->remoteServerInfo;

	if (serverCtx->isSendServerHeartBeat)
	{
		IoContext* ioCtx = remoteServerInfo->CreateSendPack_HeartBeat(socketCtx);

		if (false == serverCtx->SendPack(ioCtx))
		{
			ReleaseIoContext(ioCtx);
			serverCtx->SendSocketAbnormal(socketCtx);
			return 0;
		}

		TaskData* data = (TaskData*)malloc(sizeof(TaskData));
		data->serverCtx = serverCtx;
		data->sock = sock;
		serverCtx->PostTimerTask(_SendHeartBeatTask, _ReleaseTask, data, remoteServerInfo->sendHeartBeatPackTime);
	}

	return 0;
}

void* ServerContext::_ReleaseTask(void* data)
{
	free(data);
	return 0;
}
