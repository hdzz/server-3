#include"SocketContext.h"
#include"ServerContext.h"

SocketContext::SocketContext(ServerContext* _serverCtx)
:sock(INVALID_SOCKET)
, socketType(PENDDING_SOCKET)
, verifType(NO_VERIF)
, timeStamp(0)
, node(0)
, remoteServerInfo(0)
, extensionData(0)
, extDataRelease(0)
, serverCtx(_serverCtx)
, holdCount(0)
, isRelease(false)

#ifdef _EPOLL
, events(0)
#endif
{
	memset(&sockAddr, 0, sizeof(sockAddr));
}

// �ͷ���Դ
SocketContext:: ~SocketContext()
{
	if (sock != INVALID_SOCKET){
		RELEASE_SOCKET(sock);
		sock = INVALID_SOCKET;
	}

	ChangeToFreeList();

	node = 0;

	if (extDataRelease)
		extDataRelease(extensionData);
}


void SocketContext::ChangeToFreeList()
{
	ioctxList.reset();
	while (ioctxList.next())
	{
		IoContext* ioCtx = *(ioctxList.cur());
		ioCtx->sock = INVALID_SOCKET;
		ioCtx->socketCtx = 0;
		AddToFreeList(ioCtx);
		ioctxList.del();
	}
}

//��ioCtx��ӵ����б���
void SocketContext::AddToFreeList(IoContext* ioCtx)
{
	serverCtx->_AddToFreeList(ioCtx);
}

void SocketContext::ChangeToFreeState(IoContext* ioCtx)
{
	RemoveIoContext(ioCtx, false);
	AddToFreeList(ioCtx);
}

void SocketContext::ChangeToSplitState(IoContext* ioCtx)
{
	if (ioCtx)
		ioctxList.delete_node(ioCtx->node);
}

//��ioCtx��ӵ�ʹ�ñ���
void SocketContext::AddToList(IoContext* ioCtx)
{
	if (ioCtx){
		ioctxList.push_back(ioCtx);
		ioCtx->SetNode(ioctxList.get_last_node());
	}
}

// ��ȡһ���µ�IoContext
IoContext* SocketContext::GetNewIoContext(int packContentSize)
{
	IoContext* p;

	p = serverCtx->GetNewIoContext(packContentSize);
	p->socketCtx = this;
	p->sock = sock;
	p->node = 0;

	return p;
}


// ��vec�����Ƴ�һ��ָ����IoContext
void SocketContext::RemoveIoContext(IoContext* ioCtx, bool isDel)
{
	if (ioCtx){
		ioctxList.delete_node(ioCtx->node);
		ioCtx->node = 0;
		ioCtx->sock = INVALID_SOCKET;
		ioCtx->socketCtx = 0;
		
		if (isDel)
			delete ioCtx;
	}
}
