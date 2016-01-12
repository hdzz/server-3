#include"RemoteServerInfo.h"
#include"SocketContext.h"

RemoteServerInfo::RemoteServerInfo()
:remoteServerPort(0)
, localConnectPort(0)
, socketCtx(0)
, lifeDelayTime(11000)
, sendHeartBeatPackTime(10000)
, isCheckHeartBeat(false)
{
}

RemoteServerInfo::~RemoteServerInfo()
{
	RELEASE(socketCtx);
}

void RemoteServerInfo::Copy(RemoteServerInfo& obj)
{
	remoteServerIP = obj.remoteServerIP;
	remoteServerPort = obj.remoteServerPort;
	localConnectPort = obj.localConnectPort;
	CreateSendPack_HeartBeat = obj.CreateSendPack_HeartBeat;
	lifeDelayTime = obj.lifeDelayTime;
	isCheckHeartBeat = obj.isCheckHeartBeat;
}