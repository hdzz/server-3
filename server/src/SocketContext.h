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

	void SetNode(void* nd)
	{
		node = nd;
	}

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

	void* GetExtensionData()
	{
		return  extensionData;
	}

	void SetExtensionData(void* data)
	{
		extensionData = data;
	}

	void SetExtDataReleaseFunc(DataReleaseFunc func)
	{
		extDataRelease = func;
	}

public:
	SOCKET             sock;
	SocketType         socketType;
	SocketVerifType    verifType;
	SOCKADDR_IN        sockAddr;
	uint64_t           timeStamp;
	PackBuf            unPackCache;

	void*              extensionData;
	DataReleaseFunc    extDataRelease;
	RemoteServerInfo  *remoteServerInfo;
	ServerContext     *serverCtx;
	int                holdCount;

private:
	void              *node;
	bool               isRelease;

	friend class ServerContext;
};

#endif
