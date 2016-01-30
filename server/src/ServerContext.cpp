#include"ServerContext.h"
#include"RemoteServerInfo.h"
#include<stdlib.h>


// 初始化WinSock 2.2
bool ServerContext::LoadSocketLib()
{
#ifdef _IOCP
	WORD wVersionRequested;
	WSADATA wsaData;    // 这结构是用于接收Wjndows Socket的结构信息的
	int nResult;

	wVersionRequested = MAKEWORD(2, 2);   // 请求2.2版本的WinSock库
	nResult = WSAStartup(wVersionRequested, &wsaData);

	if (NO_ERROR != nResult)
	{
		return false;          // 返回值为零的时候是表示成功申请WSAStartup
	}

	LoadIocpExFuncs();
#endif

	return true;
}

ServerContext::ServerContext(TaskProcesser* _taskProcesser)
: nThreads(0)
, listenCtx(NULL)
, taskProcesser(0)
, workerThreads(NULL)
, useListen(true)
, useDefTaskProcesser(false)
, isCheckChildrenHeartBeat(false)
, isCheckServerHeartBeat(false)
, isSendServerHeartBeat(false)
, isStop(true)
, childLifeDelayTime(11000)             //10s

#ifdef _IOCP
,iocp(NULL)
,localListenPort(DEFAULT_PORT)
,exitWorkThreads(0)
#endif
{

#ifdef _IOCP
	key = TlsAlloc();
#endif

#ifdef _EPOLL
	pthread_key_create(&key, 0);
#endif

	setTaskProcesser(_taskProcesser);
	localListenPort = 6666;
}


ServerContext::~ServerContext(void)
{
	// 确保资源彻底释放
	Stop();
	_DeInitializeByClose();
}


//	启动服务器
bool ServerContext::Start()
{

	// 初始化本机相关信息
	if (false == _InitializeLocalInfo())
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "初始化LocalInfo失败!");
		_DeInitializeByStop();
		return false;
	}
	else
	{
		//LOG4CPLUS_INFO(log.GetInst(), "LocalInfo初始化完毕.");
	}

#ifdef _IOCP
	// 初始化IOCP
	if (false == _InitializeIOCP())
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "初始化IOCP失败!");
		_DeInitializeByStop();
		return false;
	}
	else
	{
		//LOG4CPLUS_INFO(log.GetInst(), "IOCP初始化完毕.");
	}
#endif

#ifdef _EPOLL
	// 初始化EPOLL
	if (false == _InitializeEPOLL())
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "初始化EPOLL失败!");
		_DeInitializeByStop();
		return false;
	}
	else
	{
		//LOG4CPLUS_INFO(log.GetInst(), "EPOLL初始化完毕.");
	}
#endif


	// 监听监听中的服务器，或客户端
	if(useListen)
	{
		if (false == _ListenRemoteChildren())
		{
			//LOG4CPLUS_ERROR(log.GetInst(), "本机监听失败!");
			_DeInitializeByStop();
			return false;
		}
		else
		{
			//LOG4CPLUS_INFO(log.GetInst(), "本机开始监听.");
		}
	}

	isStop = false;

	//LOG4CPLUS_INFO(log.GetInst(), "系统准备就绪....");
	return true;
}


bool ServerContext::_InitializeLocalInfo()
{
	taskProcesser->Start();
	CreateLocalIP();
	return true;
}


//	开始发送系统退出消息，退出完成端口和线程资源
void ServerContext::Stop()
{
	if (isStop)
		return;



#ifdef _IOCP

	InterlockedExchange(&exitWorkThreads, TRUE);

	for (int i = 0; i < nThreads; i++){
		// 通知所有的完成端口操作退出
		PostQueuedCompletionStatus(iocp, 0, (DWORD)EXIT_CODE, NULL);
	}
#endif

#ifdef _EPOLL
	void *status;

	  __sync_add_and_fetch(&exitWorkThreads,1);

      epoll->PostQueuedCompletionStatus();

	  for (int i = 0; i < nThreads; i++)
	  {
		  if (pthread_kill(workerThreads[i], 0) != ESRCH)
		    	pthread_join(workerThreads[i], &status);
	  }
#endif

	taskProcesser->Stop();

	// 释放其他资源
	_DeInitializeByStop();

	//LOG4CPLUS_INFO(log.GetInst(), "停止监听");
}


//	释放掉与stop相关的资源
void ServerContext::_DeInitializeByStop()
{

#ifdef _IOCP
	// 关闭IOCP句柄
	RELEASE_HANDLE(iocp);
#endif

#ifdef _EPOLL
	delete epoll;
#endif

	// 关闭监听Socket
	RELEASE(listenCtx);
}

//释放掉close时的资源
void ServerContext::_DeInitializeByClose()
{
	if(useDefTaskProcesser)
		RELEASE(taskProcesser);
}

// 监听监听中的服务器，或客户端
bool ServerContext::_ListenRemoteChildren()
{
	bool ret;

	// 生成用于监听的Socket的信息
	listenCtx = new SocketContext(this);
	listenCtx->socketType = LISTEN_SOCKET;

	// 需要使用重叠IO，必须得使用WSASocket来建立Socket，才可以支持重叠IO操作
	listenCtx->sock = _Socket();

	if (INVALID_SOCKET == listenCtx->sock)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "初始化Socket失败，错误代码:"<<_GetLastError());
		return false;
	}


    printf("listenctx=%u, listen_sock = %u\n",listenCtx, listenCtx->sock);
	// 将Listen Socket绑定至iocp或epoll中
	ret = _AssociateSocketCtx(listenCtx);

	if (false == ret)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "绑定iocp失败！错误代码:" <<_GetLastError());
		RELEASE_SOCKET(listenCtx->sock);
		return false;
	}

	// 生成本机绑定地址信息
	SOCKADDR_IN localAddress;

	while(1){
		 if (false == CreateAddressInfo(localIP.c_str(), localListenPort, localAddress))
		 {
			 //LOG4CPLUS_ERROR(log.GetInst(), "无效的本机监听地址["<<localIP.c_str()<<":"<<localListenPort<<"]");
			 return false;
		 }
		 else{
		 //LOG4CPLUS_ERROR(log.GetInst(), "本机监听地址["<<localIP.c_str()<<":"<<localListenPort<<"]");
		 }

		 // 绑定地址和端口
		 if (SOCKET_ERROR == bind(listenCtx->sock, (struct sockaddr *) &localAddress, sizeof(localAddress)))
		 {
			 srand(time(NULL));
			 localListenPort = rand() % 65535;
			 //LOG4CPLUS_ERROR(log.GetInst(), "监听时bind()函数执行错误.");
			 continue;
		 }
		 break;
	}

	 // 开始进行监听
	if (SOCKET_ERROR == listen(listenCtx->sock, SOMAXCONN))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "Listen()函数执行出现错误.");
		RELEASE_SOCKET(listenCtx->sock);
		return false;
	}

	if(false == _InitPostAccept(listenCtx))
		return false;

	if (isCheckChildrenHeartBeat)
	{
		PostTimerTask(_HeartBeatTask_CheckChildren, 0, this, childLifeDelayTime - 1000);
	}

	return true;
}


bool ServerContext::_ConnectRemoteServer(RemoteServerInfo* remoteServerInfo)
{
	// 生成用于连接服务器的Socket的信息
	uint64_t timeStamp = GetTickCount64();
	SocketContext* connectSocketCtx = remoteServerInfo->socketCtx = new SocketContext(this);
	connectSocketCtx->sock = _Socket();
	connectSocketCtx->remoteServerInfo = remoteServerInfo;

	if (INVALID_SOCKET == connectSocketCtx->sock)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "初始化Socket失败，错误代码:" << _GetLastError());
		return false;
	}

	// 生成远程服务器地址信息
	SOCKADDR_IN serverAddress;

	if (false == CreateAddressInfo(remoteServerInfo->remoteServerIP.c_str(), remoteServerInfo->remoteServerPort, serverAddress))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "无效的远程服务器地址["<<remoteServerInfo.remoteServerIP.c_str()<<":"<<remoteServerInfo.remoteServerPort<<"]");
		return false;
	}

	connectSocketCtx->sockAddr = serverAddress;
	connectSocketCtx->socketType = CONNECT_SERVER_SOCKET;
	memcpy(&(connectSocketCtx->sockAddr), &serverAddress, sizeof(SOCKADDR_IN));

	// 生成本机绑定地址信息
	SOCKADDR_IN localAddress;
	while (1){
		if (false == CreateAddressInfo(localIP.c_str(), remoteServerInfo->localConnectPort, localAddress))
		{
			SendSocketAbnormal(connectSocketCtx);
			//LOG4CPLUS_ERROR(log.GetInst(), "无效的本机地址["<<localIP.c_str()<<":"<<remoteServerInfo.localConnectPort<<"]");
			return false;
		}


		// 绑定地址和端口
		if (SOCKET_ERROR == bind(connectSocketCtx->sock, (SOCKADDR *)&localAddress, sizeof(localAddress)))
		{
			srand(time(NULL));
			remoteServerInfo->localConnectPort = rand() % 65535;
			//LOG4CPLUS_ERROR(log.GetInst(), "连接服务器时bind()函数执行错误.");
			continue;
		}

		break;
	}

	if (false == _AssociateSocketCtx(connectSocketCtx))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "完成端口绑定 connect Socket失败！错误代码:" <<_GetLastError());
		SendSocketAbnormal(connectSocketCtx);
		return false;
	}

	connectSocketCtx->UpdataTimeStamp(timeStamp);

	IoContext* newIoCtx = CreateIoContext_PostConnectPack(connectSocketCtx);

	// 开始连接服务器
	if (false == _PostConnect(newIoCtx,connectSocketCtx->sockAddr))
	{
		ReleaseIoContext(newIoCtx);
		PostSocketAbnormal(connectSocketCtx);
		return false;
	}

	return true;
}

bool ServerContext::SendPack(IoContext* ioCtx)
{
#ifdef _IOCP
	_PostTask(_PostSendTask, ioCtx);
#endif

#ifdef _EPOLL
	if (false == _PostSend(ioCtx))
	{
		ReleaseIoContext(ioCtx);
		PostSocketAbnormal(ioCtx->socketCtx);
		return false;
	}
#endif
	return true;
}

bool ServerContext::PostSocketAbnormal(SocketContext* socketCtx)
{
	TaskData* data = (TaskData*)malloc(sizeof(TaskData));
	data->serverCtx = this;
	data->sock = socketCtx->sock;

	_PostTaskData(_PostErrorTask, _ReleaseTask, data, 1000);
	return true;
}

// 生成地址信息
bool ServerContext::CreateAddressInfo(const char* name, u_short hostshort, SOCKADDR_IN& localAddress)
{
	// 生成本机绑定地址信息
	HOSTENT *local;

	local = gethostbyname(name);
	if (local == NULL)
		return false;

	memset((char *)&localAddress, 0, sizeof(localAddress));
	localAddress.sin_family = AF_INET;
	memcpy((char *)&localAddress.sin_addr.s_addr, (char *)local->h_addr, local->h_length);
	localAddress.sin_port = htons(hostshort);

	return true;

}

void ServerContext::setTaskProcesser(TaskProcesser* _taskProcesser)
{
	if (_taskProcesser == 0)
	{
		if (taskProcesser && !useDefTaskProcesser)
		{
			RELEASE(taskProcesser);
			taskProcesser = new CommonTaskProcesser;
			useDefTaskProcesser = true;
		}
		else if (taskProcesser == 0)
		{
			taskProcesser = new CommonTaskProcesser;
			useDefTaskProcesser = true;
		}

		return;
	}

	if (taskProcesser && useDefTaskProcesser){
		RELEASE(taskProcesser);
		useDefTaskProcesser = false;
	}
	taskProcesser = _taskProcesser;
}

SOCKET ServerContext:: _Socket()
{
#ifdef _IOCP
	SOCKET sock;
	// 需要使用重叠IO，必须得使用WSASocket来建立Socket，才可以支持重叠IO操作
	// 为以后新连入的客户端先准备好Socket
	if (!reusedSocketList.isempty()){

		sock = *reusedSocketList.begin();
		reusedSocketList.delete_node(reusedSocketList.get_first_node());
	}
	else{
		sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	}

	return sock;
#endif

#ifdef _EPOLL
	SOCKET sock = socket(PF_INET, SOCK_STREAM, 0);

	 if (fcntl(sock, F_SETFL, fcntl(sock, F_GETFD, 0)|O_NONBLOCK) == -1)
	{
        return -1;
    }

	return sock;
#endif
}

int ServerContext::_GetLastError()
{
#ifdef _IOCP
	return WSAGetLastError();
#endif

#ifdef _EPOLL
	return errno;
#endif
}


void ServerContext::_CreateCheckRemoteServerTimer(SocketContext* socketCtx)
{
	RemoteServerInfo* remoteServerInfo = socketCtx->remoteServerInfo;

	if (isCheckServerHeartBeat && remoteServerInfo->isCheckHeartBeat)
	{
		TaskData* data = (TaskData*)malloc(sizeof(TaskData));
		data->serverCtx = this;
		data->sock = socketCtx->sock;

		PostTimerTask(_HeartBeatTask_CheckServer, _ReleaseTask, data, remoteServerInfo->lifeDelayTime - 100);
	}

	if (isSendServerHeartBeat && remoteServerInfo->sendHeartBeatPackTime)
	{
		TaskData* data = (TaskData*)malloc(sizeof(TaskData));
		data->serverCtx = this;
		data->sock = socketCtx->sock;

		PostTimerTask(_SendHeartBeatTask, _ReleaseTask, data, remoteServerInfo->sendHeartBeatPackTime);
	}
}


//移除某个特定的Context
void ServerContext::_RemoveSocketContext(SocketContext *socketCtx)
{
#ifdef _IOCP
	SOCKET sock = socketCtx->sock;
	reusedSocketList.push_back(sock);
#endif

	delete socketCtx;
}


void ServerContext::_HeartBeat_CheckChildren()
{
	SocketContext* socketCtx;
	uint64_t time = GetTickCount64();
	uint64_t tm;

	map<SOCKET, SocketContext*>::iterator it, next;
	for (it = sockmap.begin(); it != sockmap.end();)
	{
		socketCtx = it->second;
		tm = time - socketCtx->timeStamp;

		//客户端列表
		if (socketCtx->socketType == LISTEN_CLIENT_SOCKET)
		{
			if (tm > childLifeDelayTime)
			{
				//LOG4CPLUS_INFO(log.GetInst(), "客户端socketCtx超时无响应!");
				SendSocketAbnormal(socketCtx);
			}
		}
	}
}


void ServerContext::_HeartBeat_CheckServer(RemoteServerInfo* remoteServerInfo)
{
	SocketContext* socketCtx;
	uint64_t time = GetTickCount64();
	uint64_t tm;

	//远程服务器
	socketCtx = remoteServerInfo->socketCtx;
	tm = time - socketCtx->timeStamp;

	if (tm > remoteServerInfo->lifeDelayTime)
	{
		//LOG4CPLUS_INFO(log.GetInst(), "客户端socketCtx超时无响应!");
		SendSocketAbnormal(socketCtx);
	}
}



bool ServerContext::_PostTask(task_func_cb taskfunc, RemoteServerInfo* remoteServerInfo)
{
	TaskData* data = (TaskData*)malloc(sizeof(TaskData));
	data->serverCtx = this;
	data->dataPtr = remoteServerInfo;

	return _PostTaskData(taskfunc, _ReleaseTask, data);
}


bool ServerContext::_PostTaskData(task_func_cb taskfunc, task_func_cb tk_release_func, void* taskData, int delay)
{
	int err;

	err = taskProcesser->PostTask(taskfunc, tk_release_func, taskData, delay);

	if (0 != err){
		//LOG4CPLUS_ERROR(log.GetInst(), "任务投递失败" );
		return false;
	}

	return true;
}


// 将句柄(Socket)绑定到完成端口中
bool ServerContext::_AssociateSocketCtx(SocketContext *socketCtx)
{
#ifdef _IOCP
	// 将用于和客户端通信的SOCKET绑定到完成端口中
	HANDLE hTemp = CreateIoCompletionPort((HANDLE)socketCtx->sock, iocp, (DWORD)socketCtx, 0);

	if (NULL == hTemp)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "执行CreateIoCompletionPort()出现错误.错误代码："<< GetLastError());
		return false;
	}
#endif

#ifdef _EPOLL
	// 将用于和客户端通信的SOCKET绑定到Epoll中
    int ret = epoll->BindSocket(socketCtx->sock,  socketCtx);

    if(ret != 0)
    {
        return false;
    }

#endif

	return true;
}


void ServerContext::_DoAccept(IoContext* ioCtx)
{
	SocketContext* socketCtx = ioCtx->socketCtx;

	SOCKET newSock;
	SocketType socketType = LISTEN_CLIENT_SOCKET;
	SocketContext* newSocketCtx;
	IoContext* newIoCtx;
	int err = 0;

#ifdef _IOCP
	SOCKADDR_IN* ClientAddr = NULL;
	SOCKADDR_IN* LocalAddr = NULL;
	int remoteLen = sizeof(SOCKADDR_IN), localLen = sizeof(SOCKADDR_IN);
	std::string* value = 0;

	//取得连入客户端的地址信息,通过 lpfnGetAcceptExSockAddrs
	//不但可以取得客户端和本地端的地址信息，还能顺便取出客户端发来的第一组数据
	lpfnGetAcceptExSockAddrs(
		ioCtx->packBuf.buf,
		0,
		sizeof(SOCKADDR_IN)+16,
		sizeof(SOCKADDR_IN)+16,
		(LPSOCKADDR*)&LocalAddr,
		&localLen,
		(LPSOCKADDR*)&ClientAddr,
		&remoteLen);


	err = setsockopt(
		ioCtx->sock,
		SOL_SOCKET,
		SO_UPDATE_ACCEPT_CONTEXT,
		(char *)&(socketCtx->sock),
		sizeof(socketCtx->sock));

	newSock = ioCtx->sock;
#endif

#ifdef _EPOLL
	newSock = *(int*)(ioCtx->buf);
#endif

	//LOG4CPLUS_INFO(log.GetInst(),"接受来自地址"<<inet_ntoa(ClientAddr->sin_addr)<<"的链接.");
	uint64_t timeStamp = GetTickCount64();
	newSocketCtx = new SocketContext(this);  //CreateClientSocketCtx(timeStamp);
	newSocketCtx->sock = newSock;
	newSocketCtx->socketType = socketType;
	newSocketCtx->UpdataTimeStamp(timeStamp);

#ifdef _IOCP
	memcpy(&(newSocketCtx->sockAddr), ClientAddr, sizeof(SOCKADDR_IN));
#endif
	//LOG4CPLUS_INFO(log.GetInst(),"SocketCtx:"<<"["<<newSocketCtx<<"]"<< "时间戳:"<<newSocketCtx->timeStamp<<"ms");


	// 参数设置完毕，将这个Socket和完成端口绑定(这也是一个关键步骤)
	if (_AssociateSocketCtx(newSocketCtx))
	{
		UnPack(EV_SOCKET_ACCEPTED, newSocketCtx, 0);

		newIoCtx = CreateIoContext();
		newIoCtx->sock = newSocketCtx->sock;
		newIoCtx->socketCtx = newSocketCtx;
		newIoCtx->serverCtx = this;

		_PostTask(_DoAcceptTask, newIoCtx);
	}
	else
	{
		PostSocketAbnormal(newSocketCtx);
		//LOG4CPLUS_ERROR(log.GetInst(), "绑定操作任务失败.重新投递Accept");
		return;
	}

#ifdef _IOCP
	 _PostTask(_PostAcceptTask, ioCtx);
#endif

#ifdef _EPOLL
	// 投递新的AcceptEx
	if (false == _PostAccept(ioCtx))
	{
		ReleaseIoContext(ioCtx);
		PostSocketAbnormal(socketCtx);
	}
#endif


}

//在有接收的数据到达的时候，进行处理
void ServerContext::_DoRecv(IoContext* ioCtx)
{
	SocketContext* socketCtx = ioCtx->socketCtx;
	ExtractPack* extractPack;

#ifdef _IOCP
	extractPack = (ExtractPack*)TlsGetValue(key);
#endif

#ifdef _EPOLL
	extractPack = (ExtractPack*)pthread_getspecific(key);
#endif

   //spd::get("file_logger")->info() << "_DoRecv() : ioCtx->transferedBytes:"<<ioCtx->transferedBytes ;
	socketCtx->UpdataTimeStamp();
	extractPack->SetCurtPack(socketCtx, (uint8_t*)ioCtx->buf, ioCtx->transferedBytes);
	extractPack->Extract();

	// 然后开始投递下一个WSARecv请求
	_PostRecvPack(ioCtx);

}

//已经发送完数据后调用处理
void ServerContext::_DoSend(IoContext* ioCtx)
{
#ifdef _IOCP
	_PostTask(_DoSendTask, ioCtx);
#endif 

#ifdef _EPOLL
	SocketContext* socketCtx = ioCtx->socketCtx;
	UnPack(EV_PACK_SEND, socketCtx, (uint8_t*)ioCtx->buf);
	ReleaseIoContext(ioCtx);
#endif
}
