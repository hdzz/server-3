#include"ServerBaseTypes.h"
#include"SocketContext.h"
#include"ServerContext.h"


SocketContext::SocketContext(ServerContext* _serverCtx)
:sock(INVALID_SOCKET)
, socketType(PENDDING_SOCKET)
, timeStamp(0)
, remoteServerInfo(0)
, serverCtx(_serverCtx)
, holdCount(0)
{
	unPackCache.buf = 0;
	unPackCache.len = 0;

	memset(&sockAddr, 0, sizeof(sockAddr));
}

// 释放资源
SocketContext:: ~SocketContext()
{
	delete remoteServerInfo;

	if (sock != INVALID_SOCKET){
		RELEASE_SOCKET(sock);
		sock = INVALID_SOCKET;
	}
}


IoContext* SocketContext::GetNewIoContext(int packSize)
{
	IoContext* ioCtx = CreateIoContext(packSize);
	ioCtx->sock = sock;
	ioCtx->socketCtx = this;
	ioCtx->serverCtx = serverCtx;
	return ioCtx;
}