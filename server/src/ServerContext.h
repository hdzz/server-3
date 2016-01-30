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

	void setTaskProcesser(TaskProcesser* _taskProcesser);

	void setCheckChildrenHeartBeat(bool bCheck)
	{
		isCheckChildrenHeartBeat = bCheck;
	}

	void setCheckServerHeartBeat(bool bCheck)
	{
		isCheckServerHeartBeat = bCheck;
	}

	//连接远程服务器任务
	bool ConnectRemoteServer(RemoteServerInfo* remoteServerInfo)
	{
		RemoteServerInfo* cRemoteServerInfo = new RemoteServerInfo();
		cRemoteServerInfo->Copy(*remoteServerInfo);
		return _PostTask(_ConnectRemoteServerTask, cRemoteServerInfo);
	}

	//直接发送包
	bool SendPack(IoContext* ioCtx);

	//投递任务包到线性任务器中
	bool PostSendPack(IoContext* ioCtx, int delay = 0)
	{
		return _PostTaskData(_PostSendTask, 0, ioCtx, delay);
	}

	bool PostPackTask(task_func_cb taskfunc)
	{
		return _PostTaskData(taskfunc, 0, this);
	}

	bool PostPackTask(task_func_cb taskfunc, task_func_cb delfunc, void* data)
	{
		return _PostTaskData(taskfunc, delfunc, data);
	}

	bool PostTimerTask(task_func_cb taskfunc, task_func_cb delfunc, void* data, int delay)
	{
		return _PostTaskData(taskfunc, delfunc, data, delay);
	}

	bool PostRemoveSocketCtx(SocketContext* socketCtx, int delay = 0)
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
	virtual void UnPack(Event ev, const SocketContext* socketCtx, const uint8_t* pack)
	{
	}


	//自定义连接上对方socket后发送的内容包
	virtual IoContext* CreateIoContext_PostConnectPack(SocketContext* socketCtx)
	{
		return 0;
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

	bool CreateAddressInfo(const char* name, u_short hostshort, SOCKADDR_IN& localAddress);


public:
	static bool LoadWsafunc_AccpetEx(SOCKET tmpsock);
	static bool LoadWsafunc_ConnectEx(SOCKET tmpsock);
	
protected:

//private:

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
	void _RemoveSocketContext(SocketContext *socketCtx);

	SOCKET _Socket();


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


	void _CreateCheckRemoteServerTimer(SocketContext* socketCtx);

	void _DoAccept(IoContext* ioCtx);
	void _DoRecv(IoContext* ioCtx);
	void _DoSend(IoContext* ioCtx);
	

	// 投递Connect请求
	bool _PostConnect(IoContext* ioCtx,SOCKADDR_IN& sockAddr);

	// 投递Accept请求
	bool _PostAccept(IoContext* ioCtx);

		// 投递接收数据请求
	bool _PostRecv(IoContext* ioCtx);

		// 投递发送数据请求
	bool _PostSend(IoContext* ioCtx);

    bool _AssociateSocketCtx(SocketContext *socketCtx);
    bool _InitPostAccept(SocketContext *listenCtx);

     bool _PostRecvPack(IoContext* ioCtx, int delay = 0)
	{
		return _PostTaskData(_PostRecvTask, 0, ioCtx, delay);
	}

#ifdef _IOCP

	// 初始化IOCP
	bool _InitializeIOCP();
	
	// 线程函数，为IOCP请求服务的工作者线程
	static DWORD WINAPI _WorkerThread(LPVOID param);
	static void* _DoSendTask(void* data);
	static void* _PostAcceptTask(void* data);
	static void* _PostInitAcceptTask(void* data);
#endif


#ifdef _EPOLL
	//初始化Epoll
	bool _InitializeEPOLL();


	// 线程函数，为epoll请求服务的工作者线程
	static void* _WorkerThread(void* param);
#endif


	// 与远程服务器连接上后处理
	static void* _DoConnectTask(void* data);

	// 在有客户端连入的时候处理
	static void* _DoAcceptTask(void* data);

	static void* _PostRecvTask(void* data);
	static void* _PostSendTask(void* data);
	static void* _PostErrorTask(void* data);
	static void* _PostRemoveSocketCtxTask(void* data);

	static void* _CloseSocketTask(void* data);
	static void* _ConnectRemoteServerTask(void* data);
	static void* _HeartBeatTask_CheckChildren(void* data);
	static void* _HeartBeatTask_CheckServer(void* data);
	static void* _SendHeartBeatTask(void* data);

	static void* _ReleaseTask(void* data);


protected:
	bool                         useListen;
	bool                         useDefTaskProcesser;

	bool                         isCheckChildrenHeartBeat;
	int                          childLifeDelayTime;         // 每次检查所有客户端心跳状态的时间间隔
	bool                         isCheckServerHeartBeat;
	bool                         isSendServerHeartBeat;
	bool                         isStop;

private:
	map<SOCKET, SocketContext*>  sockmap;
	TaskProcesser*               taskProcesser;

	int                          serverType;
	int		                     nThreads;                   // 生成的线程数量
	std::string                  localIP;                    // 本机IP地址
	int                          localListenPort;            // 本机监听端口
	SocketContext*               listenCtx;                  // 用于监听的Socket的Context信息
	volatile long                exitWorkThreads;

#ifdef _IOCP
	HANDLE                       iocp;                       // 完成端口的句柄
	DWORD                        key;                        // 工作者线程组指针
	HANDLE*                      workerThreads;
	CmiVector<SOCKET>            reusedSocketList;           // 可重用的socket列表 
#endif

#ifdef _EPOLL
    EpollToIocp*         epoll;
    pthread_key_t        key;
	pthread_t*           workerThreads;              // 工作者线程组指针
#endif

	friend class ExtractPack;
	friend class SocketContext;

};
#endif
