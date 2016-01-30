#ifndef _SOCKETCONTEXT_H_
#define _SOCKETCONTEXT_H_

#include"ServerBaseTypes.h"
#include"ExtractPack.h"
#include"IoContext.h"


class SocketContext
{
public:
	// 初始化
	SocketContext(ServerContext* _serverCtx);

	// 释放资源
	virtual ~SocketContext();

	void UpdataTimeStamp(uint64_t _timeStamp)
	{
		timeStamp = _timeStamp;
	}

	void UpdataTimeStamp()
	{
		timeStamp = GetTickCount64();
	}

	IoContext* GetNewIoContext(int packSize = MAX_BUFFER_LEN);

	RemoteServerInfo* GetRemoteServerInfo()
	{
		return remoteServerInfo;
	}

public:
	SOCKET             sock;
	SocketType         socketType;
	SOCKADDR_IN        sockAddr;
	uint64_t           timeStamp;
	PackBuf            unPackCache;

	RemoteServerInfo  *remoteServerInfo;
	ServerContext     *serverCtx;
	int                holdCount;

private:
#ifdef _IOCP
	IoCtxList         sendList;
#endif

	friend class ServerContext;
};

#endif
