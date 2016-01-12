#ifndef _SOCKETCONTEXT_H_
#define _SOCKETCONTEXT_H_

#include"ServerBaseTypes.h"
#include"ExtractPack.h"
#include"IoContext.h"

//SocketContext��������,��������ֻ��ServerContext��TaskThread�߳���ʹ��
//�������Ա�֤SocketContext�������ͬ������
//��ȻSocketContext������Я�������ݿ��Կ�����ת�Ƶ������̲߳���
class SocketContext
{
public:
	// ��ʼ��
	SocketContext(ServerContext* _serverCtx);

	// �ͷ���Դ
	virtual ~SocketContext();

	// ��ȡһ���µ�IoContext
	IoContext* GetNewIoContext(int packContentSize = MAX_BUFFER_LEN);

	// ��vec�����Ƴ�һ��ָ����IoContext
	void RemoveIoContext(IoContext* ioCtx, bool isDel = true);

	//��ioCtx��ӵ����б���
	void AddToFreeList(IoContext* ioCtx);
	void ChangeToFreeState(IoContext* ioCtx);
	void ChangeToSplitState(IoContext* ioCtx);
	void ChangeToFreeList();

	//��ioCtx��ӵ�ʹ�ñ���
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