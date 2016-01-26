#ifndef _REMOTESERVERINFO_H_
#define _REMOTESERVERINFO_H_

#include"ServerBaseTypes.h"

//已连接的远程服务器
class RemoteServerInfo
{
public:
	RemoteServerInfo();
	~RemoteServerInfo();

	void Copy(RemoteServerInfo& obj);
	
	SocketContext* GetSocketContext()
	{
		return socketCtx;
	}


	std::string                  remoteServerIP;             // 远程服务器IP地址
	int                          remoteServerPort;           // 远程服务器的端口
	int                          localConnectPort;           // 连接远程服务器的端口
	CreateSendPack               CreateSendPack_HeartBeat;
	int                          lifeDelayTime;              // 每次检查到服务器端心跳状态的时间间隔
	int                          sendHeartBeatPackTime;      // 每次发送心跳包间隔     
	bool                         isCheckHeartBeat;           // 是否检测心跳
	SocketContext*               socketCtx;                  // 连接远程服务器的Context信息(此变量存储与远程服务器的连接信息)
	                       
private:
	void*                        node;

	friend class ServerContext;
	
};

#endif