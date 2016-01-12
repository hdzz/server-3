#ifndef _SOCKETCONTEXT_H_
#define _SOCKETCONTEXT_H_

#include"ServerBaseTypes.h"
#include"ExtractPack.h"
#include"IoContext.h"

//SocketContext被创建后,这个类最好只在ServerContext的TaskThread线程中使用
//这样可以保证SocketContext不会出现同步问题
//当然SocketContext返回所携带的数据可以拷贝后转移到其它线程操作
class SocketContext
{
public:
	// 初始化
	SocketContext(ServerContext* _serverCtx);

	// 释放资源
	virtual ~SocketContext();

	// 获取一个新的IoContext
	IoContext* GetNewIoContext(int packContentSize = MAX_BUFFER_LEN);

	// 从vec表中移除一个指定的IoContext
	void RemoveIoContext(IoContext* ioCtx, bool isDel = true);

	//把ioCtx添加到空闲表中
	void AddToFreeList(IoContext* ioCtx);
	void ChangeToFreeState(IoContext* ioCtx);
	void ChangeToSplitState(IoContext* ioCtx);
	void ChangeToFreeList();

	//把ioCtx添加到使用表中
	void AddToList(IoContext* ioCtx);
	

	void  SetPackContext(uint8_t* _cachebuf, int _cachelen)
	{
		extractPack.SetContext(serverCtx, _cachebuf, _cachelen);
	}

	uint8_t* NextPack(){ return extractPack.next(); }
	uint8_t* CurPack(){ return extractPack.cur(); }

	IoCtxList& getIoCtxList(){ return ioctxList; }

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
		timeStamp = GetSysTickCount64();
	}

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
	IoCtxList          ioctxList;
	ExtractPack        extractPack;
	void*              extensionData;
	DataReleaseFunc    extDataRelease;
	RemoteServerInfo  *remoteServerInfo;
	ServerContext     *serverCtx;
	int                holdCount;
	
private:
	void              *node;
	bool               isRelease;

#ifdef _EPOLL
	__uint32_t         events;
#endif

	friend class ServerContext;
};

#endif