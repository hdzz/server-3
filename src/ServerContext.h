#ifndef _SERVERCONTEXT_H_
#define _SERVERCONTEXT_H_

#include"ServerBaseTypes.h"
#include"SocketContext.h"
#include"RemoteServerInfo.h"
#include"TaskProcesser.h"

// ServerContext��
class ServerContext
{
public:
	ServerContext(TaskProcesser* _taskProcesser = 0);
	virtual ~ServerContext(void);

public:

	// ����Socket��
	static bool LoadSocketLib();

	// ж��Socket��
	static void UnloadSocketLib()
	{
#ifdef _IOCP
		WSACleanup();
#endif
	}

	void UseMultithreading(bool use)
	{
		useMultithreading = use;
	}

	void setTaskProcesser(TaskProcesser* _taskProcesser);
	
	void setCheckChildrenHeartBeat(bool bCheck)
	{
		isCheckChildrenHeartBeat = bCheck;
	}

	void setCheckServerHeartBeat(bool bCheck)
	{
		isCheckServerHeartBeat = bCheck;
	}


	//�ȴ�����Զ�̷�����
	bool WaitConnectRemoteServer(RemoteServerInfo* remoteServerInfo, long delayMillisecond = -1);


	//����Զ�̷�����
	bool ConnectRemoteServer(RemoteServerInfo* remoteServerInfo)
	{
		RemoteServerInfo* cRemoteServerInfo = new RemoteServerInfo();
		cRemoteServerInfo->Copy(*remoteServerInfo);
		return _ConnectRemoteServer(cRemoteServerInfo);
	}

	//Ͷ������Զ�̷���������
	bool PostConnectRemoteServerTask(RemoteServerInfo* remoteServerInfo)
	{
		RemoteServerInfo* cRemoteServerInfo = new RemoteServerInfo();
		cRemoteServerInfo->Copy(*remoteServerInfo);
		return _PostTask(_ConnectRemoteServerTask, cRemoteServerInfo);
	}

	//���Ͱ�(�������߼�������������ʱ������ֱ��ʹ�ô˺���������)
	bool SendPack(IoContext* ioCtx)
	{		
		if (false == _PostSend(ioCtx))	
		{	
			SendSocketAbnormal(ioCtx->socketCtx);	
			return false;	
		}	
		return true;	
	}

	//Ͷ���������������������(�������߼���������������ʱ������Ͷ��������������������ٴ���)
	bool ServerContext::PostSendPack(IoContext* ioCtx, int delay = 0)
	{
		return _PostTaskData(_PostSendTask, 0, ioCtx, delay);
	}

	bool ServerContext::PostRecvPack(IoContext* ioCtx, int delay = 0)
	{
		return _PostTaskData(_PostRecvTask, 0, ioCtx, delay);
	}

	bool ServerContext::PostPackTask(task_func_cb taskfunc)
	{
		return _PostTaskData(taskfunc, 0, this);
	}

	bool ServerContext::PostPackTask(task_func_cb taskfunc, task_func_cb delfunc, void* data)
	{
		return _PostTaskData(taskfunc, delfunc, data);
	}

	bool ServerContext::PostTimerTask(task_func_cb taskfunc, task_func_cb delfunc, void* data, int delay)
	{
		return _PostTaskData(taskfunc, delfunc, data, delay);
	}

	bool ServerContext::PostRemoveSocketCtx(SocketContext* socketCtx, int delay = 0)
	{
		return _PostTaskData(_PostRemoveSocketCtxTask, 0, socketCtx, delay);
	}

	//�ر�socket
	bool CloseSocket(SocketContext* socketCtx)
	{
		return _PostTaskData(_CloseSocketTask, 0, socketCtx);
	}

	//����socket�쳣
	bool PostSocketAbnormal(SocketContext* socketCtx);
	
	bool SendSocketAbnormal(SocketContext* socketCtx)
	{
		return PostSocketAbnormal(socketCtx);
	}

	//����̳�ServerContext��,���¹��ܰ��踲��
	//
	// ����������
	virtual bool Start();

	//	ֹͣ������
	virtual void Stop();

	//�ӶԷ�socket�Ǳ߽��յ������ݰ�����������д���ϵͳ�Զ����ô˺������
	virtual void UnPack(SocketContext* socketCtx, uint8_t* pack)
	{
	}

	//�Զ���Է�socket���ߺ�İ����ݣ���ϵͳUnPack����
	virtual bool CreatePack_OffLine(PackBuf* pack)
	{
		return true;
	}

	//�Զ��������϶Է�socket��İ����ݣ���ϵͳUnPack����
	virtual bool CreatePack_Connected(PackBuf* pack)
	{
		return true;
	}

	//�Զ��������϶Է�socket��İ����ݣ���ϵͳUnPack����
	virtual bool CreatePack_NotPassRecv(PackBuf* pack)
	{
		return true;
	}

	//�Զ�����ܵ��Է�socket��İ����ݣ���ϵͳUnPack����
	virtual bool CreatePack_Accepted(PackBuf* pack)
	{
		return true;
	}

	virtual SocketContext* CreateClientSocketCtx(uint64_t timeStamp)
	{
		return GetNewSocketContext(timeStamp);
	}

	virtual SocketContext* CreateConnectSocketCtx(uint64_t timeStamp)
	{
		return GetNewSocketContext(timeStamp);
	}

	virtual int GetPackHeadLength()
	{
		return sizeof(uint16_t) * 2;
	}

	virtual uint16_t GetPackLength(uint8_t* pack)
	{
		uint16_t* len = (uint16_t*)(pack + sizeof(uint16_t));
		return (*len + GetPackHeadLength());
	}
	

	// ���ü����˿�
	void SetPort(const int& _nPort) { localListenPort = _nPort; }


	// ��ñ�����IP��ַ
	std::string& CreateLocalIP()
	{
		char ip[16] = { 0 };
		GetLocalIP(ip);
		localIP.assign(ip);
		return localIP;
	}

	SocketContext* GetNewSocketContext(uint64_t timeStamp);
	IoContext* GetNewIoContext(int packContentSize = MAX_BUFFER_LEN);


	bool CreateAddressInfo(const char* name, u_short hostshort, SOCKADDR_IN& localAddress);


protected:
	static void* _ReleaseTask(void* data);

private:

	SOCKET _Socket();
	int _GetLastError();

	//��ʼ�����������Ϣ
	bool _InitializeLocalInfo();

	//����Զ�̷�������ͻ���
	bool _ListenRemoteChildren();

	// ����ͷ���Դ
	void _DeInitializeByStop();
	void _DeInitializeByClose();

	// ���ͻ��˵������Ϣ�洢��������
	void _AddToClientCtxList(SocketContext *socketCtx);

	//�Ƴ�socket(�������߼�������������ʱ������ֱ��ʹ�ô˺������Ƴ�socket)
	bool _RemoveSocketContext(SocketContext *socketCtx);

	int _GetFixSizeIndex(int orgSize, int& fixSize);

	

	// �����Ϣ
	template <class T>
	void _ClearVecList(CmiVector<T>& vecList, bool isDel = true);

	void _HeartBeat_CheckChildren();
	void _HeartBeat_CheckServer(RemoteServerInfo* remoteServerInfo);

	bool _ConnectRemoteServer(RemoteServerInfo* remoteServerInfo);

	bool _PostTask(task_func_cb taskfunc, SocketContext* socketCtx, int delay = 0)
	{
		return _PostTaskData(taskfunc, 0, socketCtx, delay);
	}

	bool _PostTask(task_func_cb taskfunc, IoContext* ioCtx, int delay = 0)
	{
		return _PostTaskData(taskfunc, 0, ioCtx, delay);
	}

	bool _PostTask(task_func_cb taskfunc, RemoteServerInfo* remoteServerInfo);

	bool _PostTaskData(task_func_cb taskfunc, task_func_cb tk_release_func, void* taskData, int delay = 0);

	void _UnPack(SocketContext* socketCtx);

	void _ConnectedPack(SocketContext* socketCtx);
	void _NotPassRecvPack(SocketContext* socketCtx);
	void _AcceptedClientPack(SocketContext* socketCtx);

	// Ͷ�ݷ�����������
	bool _PostSend(IoContext* ioCtx);

	void _AddToRemoteServerInfoList(RemoteServerInfo* remoteServerInfo);
	void _AddToFreeList(IoContext *ioCtx);
	void _AddToFreeList(SocketContext *socketCtx);

	void _DoRecv(IoContext* ioCtx);
	void _OfflineSocket(SocketContext* socketCtx);

#ifdef _IOCP

	// ��ʼ��IOCP
	bool _InitializeIOCP();
	bool _AssociateWithIOCP(SocketContext *socketCtx);
	bool _InitPostAccept(SocketContext *listenCtx);

	// Ͷ��Connect����
	bool _PostConnect(IoContext* ioCtx,SOCKADDR_IN& sockAddr);

	// Ͷ��Accept����
	bool _PostAccept(IoContext* ioCtx);

	// Ͷ�ݽ�����������
	bool _PostRecv(IoContext* ioCtx);

	static void* _ReAccpetTask(void* data);

	bool _LoadWsafunc_AccpetEx(SOCKET tmpsock);
	bool _LoadWsafunc_ConnectEx(SOCKET tmpsock);

#endif


#ifdef _EPOLL
	//��ʼ��Epoll
	bool _InitializeEPOLL();
	bool _AddWithEpoll(IoContext* ioCtx, __uint32_t events);
	bool _ModWithEpoll(IoContext* ioCtx, __uint32_t events);
	bool _DelWithEpoll(SocketContext* socketCtx);
	static int _SetNonBlocking(SOCKET sockfd);
	void* _DoPostSendTask(void* data);

	bool _PostAccept(SocketContext* socketCtx);

	// Ͷ�ݽ�����������
	bool _PostRecv(SocketContext* socketCtx);

	bool _SendPack(IoContext* ioCtx);

#endif

	// �̺߳�����ΪIOCP��epoll�������Ĺ������߳�
	static void* _WorkerThread(void* param);


	// ��Զ�̷����������Ϻ���
	static void* _DoConnectTask(void* data);

	// ���пͻ��������ʱ����
	static void* _DoAcceptTask(void* data);

	// ���н��յ����ݵ����ʱ����
	static void* _DoRecvTask(void* data);

	//������ɺ���
	static void* _DoSendTask(void* data);


	static void* _PostRecvTask(void* data);
	static void* _PostSendTask(void* data);
	static void* _PostRemoveSocketCtxTask(void* data);

	static void* _OfflineSocketTask(void* data);
	static void* _CloseSocketTask(void* data);
	static void* _ConnectRemoteServerTask(void* data);	
	static void* _HeartBeatTask_CheckChildren(void* data);
	static void* _HeartBeatTask_CheckServer(void* data);
	static void* _SendHeartBeatTask(void* data);
	
	
protected:
	bool                         useMultithreading;
	bool                         useListen;
	bool                         useWaitConnectRemoteServer;
	bool                         useDefTaskProcesser;

	bool                         isCheckChildrenHeartBeat;
	int                          childLifeDelayTime;         // ÿ�μ�����пͻ�������״̬��ʱ����
	bool                         isCheckServerHeartBeat;
	bool                         isSendServerHeartBeat;
	bool                         isStop;

private:
	TaskProcesser*               taskProcesser;       
	
	IoCtxList                    ioctxFreeList[8];
	SocketCtxList                socketctxFreeList;

	CmiThreadLock                shardMemoryLock;
	
	CmiWaitSem                   waitSem;
	CmiWaitSem                   waitConnectRemoteServerSem;
	int                          serverType;	
	int		                     nThreads;                   // ���ɵ��߳�����
	std::string                  localIP;                    // ����IP��ַ
	int                          localListenPort;            // ���������˿�
    pthread_t*                   workerThreads;              // �������߳���ָ��
	SocketContext*               listenCtx;                  // ���ڼ�����Socket��Context��Ϣ
	
	SocketCtxList                clientCtxList;              // �ͻ���Socket��Context��Ϣ
	RemoteServerInfoList         remoteServerInfoList;       // Զ�̷�������Ϣ��

#ifdef _IOCP
	HANDLE                       iocp;                       // ��ɶ˿ڵľ��
	LPFN_CONNECTEX               lpfnConnectEx;
	LPFN_ACCEPTEX                lpfnAcceptEx;               // AcceptEx �� GetAcceptExSockaddrs �ĺ���ָ�룬���ڵ�����������չ����
	LPFN_GETACCEPTEXSOCKADDRS    lpfnGetAcceptExSockAddrs;
#endif

#ifdef _EPOLL
	int                           epfd;
	struct epoll_event           *events;
	int                           fd[2];
	SocketContext*                quitSocketCtx;
#endif

	friend class SocketContext;

};
#endif