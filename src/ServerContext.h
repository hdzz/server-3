#ifndef _SERVERCONTEXT_H_
#define _SERVERCONTEXT_H_

#include"ServerBaseTypes.h"
#include"SocketContext.h"
#include"RemoteServerInfo.h"
#include"TaskProcesser.h"

// ServerContext类
class ServerContext
{
public:
	ServerContext(TaskProcesser* _taskProcesser = 0);
	virtual ~ServerContext(void);

public:

	// 加载Socket库
	static bool LoadSocketLib();

	// 卸载Socket库
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


	//等待连接远程服务器
	bool WaitConnectRemoteServer(RemoteServerInfo* remoteServerInfo, long delayMillisecond = -1);


	//连接远程服务器
	bool ConnectRemoteServer(RemoteServerInfo* remoteServerInfo)
	{
		RemoteServerInfo* cRemoteServerInfo = new RemoteServerInfo();
		cRemoteServerInfo->Copy(*remoteServerInfo);
		return _ConnectRemoteServer(cRemoteServerInfo);
	}

	//投递连接远程服务器任务
	bool PostConnectRemoteServerTask(RemoteServerInfo* remoteServerInfo)
	{
		RemoteServerInfo* cRemoteServerInfo = new RemoteServerInfo();
		cRemoteServerInfo->Copy(*remoteServerInfo);
		return _PostTask(_ConnectRemoteServerTask, cRemoteServerInfo);
	}

	//发送包(当代码逻辑在任务处理器中时，可以直接使用此函数来发包)
	bool SendPack(IoContext* ioCtx)
	{		
		if (false == _PostSend(ioCtx))	
		{	
			SendSocketAbnormal(ioCtx->socketCtx);	
			return false;	
		}	
		return true;	
	}

	//投递任务包到线性任务器中(当代码逻辑不在任务处理器中时，必须投递任务包到任务处理器中再处理)
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

	//关闭socket
	bool CloseSocket(SocketContext* socketCtx)
	{
		return _PostTaskData(_CloseSocketTask, 0, socketCtx);
	}

	//发送socket异常
	bool PostSocketAbnormal(SocketContext* socketCtx);
	
	bool SendSocketAbnormal(SocketContext* socketCtx)
	{
		return PostSocketAbnormal(socketCtx);
	}

	//子类继承ServerContext后,以下功能按需覆盖
	//
	// 启动服务器
	virtual bool Start();

	//	停止服务器
	virtual void Stop();

	//从对方socket那边接收到的数据包在这个函数中处理，系统自动调用此函数解包
	virtual void UnPack(SocketContext* socketCtx, uint8_t* pack)
	{
	}

	//自定义对方socket离线后的包内容，供系统UnPack调用
	virtual bool CreatePack_OffLine(PackBuf* pack)
	{
		return true;
	}

	//自定义连接上对方socket后的包内容，供系统UnPack调用
	virtual bool CreatePack_Connected(PackBuf* pack)
	{
		return true;
	}

	//自定义连接上对方socket后的包内容，供系统UnPack调用
	virtual bool CreatePack_NotPassRecv(PackBuf* pack)
	{
		return true;
	}

	//自定义接受到对方socket后的包内容，供系统UnPack调用
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
	

	// 设置监听端口
	void SetPort(const int& _nPort) { localListenPort = _nPort; }


	// 获得本机的IP地址
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

	//初始化本机相关信息
	bool _InitializeLocalInfo();

	//监听远程服务器或客户端
	bool _ListenRemoteChildren();

	// 最后释放资源
	void _DeInitializeByStop();
	void _DeInitializeByClose();

	// 将客户端的相关信息存储到数组中
	void _AddToClientCtxList(SocketContext *socketCtx);

	//移除socket(当代码逻辑在任务处理器中时，可以直接使用此函数来移除socket)
	bool _RemoveSocketContext(SocketContext *socketCtx);

	int _GetFixSizeIndex(int orgSize, int& fixSize);

	

	// 清空信息
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

	// 投递发送数据请求
	bool _PostSend(IoContext* ioCtx);

	void _AddToRemoteServerInfoList(RemoteServerInfo* remoteServerInfo);
	void _AddToFreeList(IoContext *ioCtx);
	void _AddToFreeList(SocketContext *socketCtx);

	void _DoRecv(IoContext* ioCtx);
	void _OfflineSocket(SocketContext* socketCtx);

#ifdef _IOCP

	// 初始化IOCP
	bool _InitializeIOCP();
	bool _AssociateWithIOCP(SocketContext *socketCtx);
	bool _InitPostAccept(SocketContext *listenCtx);

	// 投递Connect请求
	bool _PostConnect(IoContext* ioCtx,SOCKADDR_IN& sockAddr);

	// 投递Accept请求
	bool _PostAccept(IoContext* ioCtx);

	// 投递接收数据请求
	bool _PostRecv(IoContext* ioCtx);

	static void* _ReAccpetTask(void* data);

	bool _LoadWsafunc_AccpetEx(SOCKET tmpsock);
	bool _LoadWsafunc_ConnectEx(SOCKET tmpsock);

#endif


#ifdef _EPOLL
	//初始化Epoll
	bool _InitializeEPOLL();
	bool _AddWithEpoll(IoContext* ioCtx, __uint32_t events);
	bool _ModWithEpoll(IoContext* ioCtx, __uint32_t events);
	bool _DelWithEpoll(SocketContext* socketCtx);
	static int _SetNonBlocking(SOCKET sockfd);
	void* _DoPostSendTask(void* data);

	bool _PostAccept(SocketContext* socketCtx);

	// 投递接收数据请求
	bool _PostRecv(SocketContext* socketCtx);

	bool _SendPack(IoContext* ioCtx);

#endif

	// 线程函数，为IOCP或epoll请求服务的工作者线程
	static void* _WorkerThread(void* param);


	// 与远程服务器连接上后处理
	static void* _DoConnectTask(void* data);

	// 在有客户端连入的时候处理
	static void* _DoAcceptTask(void* data);

	// 在有接收的数据到达的时候处理
	static void* _DoRecvTask(void* data);

	//发送完成后处理
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
	int                          childLifeDelayTime;         // 每次检查所有客户端心跳状态的时间间隔
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
	int		                     nThreads;                   // 生成的线程数量
	std::string                  localIP;                    // 本机IP地址
	int                          localListenPort;            // 本机监听端口
    pthread_t*                   workerThreads;              // 工作者线程组指针
	SocketContext*               listenCtx;                  // 用于监听的Socket的Context信息
	
	SocketCtxList                clientCtxList;              // 客户端Socket的Context信息
	RemoteServerInfoList         remoteServerInfoList;       // 远程服务器信息表

#ifdef _IOCP
	HANDLE                       iocp;                       // 完成端口的句柄
	LPFN_CONNECTEX               lpfnConnectEx;
	LPFN_ACCEPTEX                lpfnAcceptEx;               // AcceptEx 和 GetAcceptExSockaddrs 的函数指针，用于调用这两个扩展函数
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