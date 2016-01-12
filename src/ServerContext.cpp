#include"ServerContext.h"
#include"RemoteServerInfo.h"
#include<stdlib.h>


// ��ʼ��WinSock 2.2
bool ServerContext::LoadSocketLib()
{
#ifdef _IOCP
	WORD wVersionRequested;
	WSADATA wsaData;    // ��ṹ�����ڽ���Wjndows Socket�Ľṹ��Ϣ��
	int nResult;

	wVersionRequested = MAKEWORD(2, 2);   // ����2.2�汾��WinSock��
	nResult = WSAStartup(wVersionRequested, &wsaData);

	if (NO_ERROR != nResult)
	{
		return false;          // ����ֵΪ���ʱ���Ǳ�ʾ�ɹ�����WSAStartup
	}

#endif

	return true;
}

ServerContext::ServerContext(TaskProcesser* _taskProcesser)
: nThreads(0)
, listenCtx(NULL)
, workerThreads(NULL)
, useMultithreading(false)
, useListen(true)
, useWaitConnectRemoteServer(false)
, useDefTaskProcesser(false)
, isCheckChildrenHeartBeat(false)
, isCheckServerHeartBeat(false)
, isStop(true)
, childLifeDelayTime(11000)             //10s

#ifdef _IOCP
,iocp(NULL)
,localListenPort(DEFAULT_PORT)
,lpfnAcceptEx(NULL)
,lpfnGetAcceptExSockAddrs(NULL)
,lpfnConnectEx(NULL)
#endif

#ifdef _EPOLL
,events(NULL)
,quitSocketCtx(NULL)
#endif
{
	setTaskProcesser(_taskProcesser);

	//tstring logfile = L"server.log";
	//log.OutputFile(logfile);
	//log.OutputConsole();

	localListenPort = 6666;
}


ServerContext::~ServerContext(void)
{
	// ȷ����Դ�����ͷ�
	Stop();
	_DeInitializeByClose();
}


//	����������
bool ServerContext::Start()
{

	// ��ʼ�����������Ϣ
	if (false == _InitializeLocalInfo())
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "��ʼ��LocalInfoʧ��!");
		_DeInitializeByStop();
		return false;
	}
	else
	{
		//LOG4CPLUS_INFO(log.GetInst(), "LocalInfo��ʼ�����.");
	}

#ifdef _IOCP
	// ��ʼ��IOCP
	if (false == _InitializeIOCP())
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "��ʼ��IOCPʧ��!");
		_DeInitializeByStop();
		return false;
	}
	else
	{
		//LOG4CPLUS_INFO(log.GetInst(), "IOCP��ʼ�����.");
	}
#endif

#ifdef _EPOLL
	// ��ʼ��EPOLL
	if (false == _InitializeEPOLL())
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "��ʼ��EPOLLʧ��!");
		_DeInitialize();
		return false;
	}
	else
	{
		//LOG4CPLUS_INFO(log.GetInst(), "EPOLL��ʼ�����.");
	}
#endif


	// ���������еķ���������ͻ���
	if(useListen)
	{
		if (false == _ListenRemoteChildren())
		{
			//LOG4CPLUS_ERROR(log.GetInst(), "��������ʧ��!");
			_DeInitializeByStop();
			return false;
		}
		else
		{
			//LOG4CPLUS_INFO(log.GetInst(), "������ʼ����.");
		}
	}

	isStop = false;

	//LOG4CPLUS_INFO(log.GetInst(), "ϵͳ׼������....");
	return true;
}


bool ServerContext::_InitializeLocalInfo()
{
	taskProcesser->Start();
	CreateLocalIP();
	return true;
}


//	��ʼ����ϵͳ�˳���Ϣ���˳���ɶ˿ں��߳���Դ
void ServerContext::Stop()
{
	if (isStop)
		return;

	void *status;
	waitSem.SetSem();

#ifdef _IOCP
	for (int i = 0; i < nThreads; i++){
		// ֪ͨ���е���ɶ˿ڲ����˳�
		PostQueuedCompletionStatus(iocp, 0, (DWORD)EXIT_CODE, NULL);
	}
#endif

#ifdef _EPOLL
	char s[] = "quit";
	for (int i = 0; i < nThreads; i++)
	{
		write(fd[1], s, sizeof(s));
	}
#endif

	for (int i = 0; i < nThreads; i++)
	{
		if (pthread_kill(workerThreads[i], 0) != ESRCH)
			pthread_join(workerThreads[i], &status);
	}

	taskProcesser->Stop();

	_ClearVecList(clientCtxList);
	_ClearVecList(remoteServerInfoList);
	
	// �ͷ�������Դ
	_DeInitializeByStop();

	//LOG4CPLUS_INFO(log.GetInst(), "ֹͣ����");
}


//	�ͷŵ���stop��ص���Դ
void ServerContext::_DeInitializeByStop()
{
	// �������б���Ϣ
	_ClearVecList(clientCtxList);
	_ClearVecList(remoteServerInfoList);


#ifdef _IOCP
	// �ر�IOCP���
	RELEASE_HANDLE(iocp);
#endif

#ifdef _EPOLL
	gfree(events);
	close(fd[0]);
	close(fd[1]);
	quitSocketCtx->sock = 0;
	RELEASE(quitSocketCtx);
#endif

	// �رռ���Socket
	RELEASE(listenCtx);
}

//�ͷŵ�closeʱ����Դ
void ServerContext::_DeInitializeByClose()
{
	// �������б���Ϣ
	for (int i = 0; i < 8; i++)
		_ClearVecList(ioctxFreeList[i]);

	_ClearVecList(socketctxFreeList);

	if(useDefTaskProcesser)
		RELEASE(taskProcesser);
}

template <class T>
void ServerContext::_ClearVecList(CmiVector<T>& vecList)
{
	vecList.reset();
	while (vecList.next()){
		delete *(vecList.cur());
		vecList.del();
	}
}



// ���������еķ���������ͻ���
bool ServerContext::_ListenRemoteChildren()
{
	bool ret;

	// �������ڼ�����Socket����Ϣ
	listenCtx = new SocketContext(this);
	listenCtx->socketType = LISTEN_SOCKET;

	// ��Ҫʹ���ص�IO�������ʹ��WSASocket������Socket���ſ���֧���ص�IO����
	listenCtx->sock = _Socket();

	if (INVALID_SOCKET == listenCtx->sock)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "��ʼ��Socketʧ�ܣ��������:"<<_GetLastError());
		return false;
	}


	// ��Listen Socket����iocp��epoll��
#ifdef _IOCP
	ret = _AssociateWithIOCP(listenCtx);

	if (false == ret)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "��iocpʧ�ܣ��������:" <<_GetLastError());
		RELEASE_SOCKET(listenCtx->sock);
		return false;
	}	
#endif

	// ���ɱ����󶨵�ַ��Ϣ
	SOCKADDR_IN localAddress;

	while(1){
		 if (false == CreateAddressInfo(localIP.c_str(), localListenPort, localAddress))
		 {
			 //LOG4CPLUS_ERROR(log.GetInst(), "��Ч�ı���������ַ["<<localIP.c_str()<<":"<<localListenPort<<"]");
			 return false;
		 }
		 else{
		 //LOG4CPLUS_ERROR(log.GetInst(), "����������ַ["<<localIP.c_str()<<":"<<localListenPort<<"]");
		 }

		 // �󶨵�ַ�Ͷ˿�
		 if (SOCKET_ERROR == bind(listenCtx->sock, (struct sockaddr *) &localAddress, sizeof(localAddress)))
		 {
			 srand(time(NULL));
			 localListenPort = rand() % 65535;
			 //LOG4CPLUS_ERROR(log.GetInst(), "����ʱbind()����ִ�д���.");
			 continue;
		 }
		 break;
	}

	// ��ʼ���м���
	if (SOCKET_ERROR == listen(listenCtx->sock, SOMAXCONN))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "Listen()����ִ�г��ִ���.");
		return false;
	}

#ifdef _EPOLL
	IoContext* ioCtx = listenCtx->GetNewIoContext(1);
	ret = _AddWithEpoll(ioCtx, EPOLLIN | EPOLLET | EPOLLONESHOT);
	if (false == ret)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "�� epollʧ�ܣ��������:" <<_GetLastError());
		_AddToFreeList(ioCtx);
		RELEASE_SOCKET(listenCtx->sock);
		return false;
	}
#endif

#ifdef _IOCP
	if(false == _InitPostAccept(listenCtx))
		return false;
#endif

	if (isCheckChildrenHeartBeat)
	{
		PostTimerTask(_HeartBeatTask_CheckChildren, 0, this, childLifeDelayTime);
	}

	return true;
}


bool ServerContext::_ConnectRemoteServer(RemoteServerInfo* remoteServerInfo)
{
	// �����������ӷ�������Socket����Ϣ
	uint64_t timeStamp = GetSysTickCount64();
	SocketContext* connectSocketCtx = remoteServerInfo->socketCtx = CreateConnectSocketCtx(timeStamp);
	connectSocketCtx->sock = _Socket();
	connectSocketCtx->remoteServerInfo = remoteServerInfo;

	if (INVALID_SOCKET == connectSocketCtx->sock)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "��ʼ��Socketʧ�ܣ��������:" << _GetLastError());
		return false;
	}

	// ����Զ�̷�������ַ��Ϣ
	SOCKADDR_IN serverAddress;

	if (false == CreateAddressInfo(remoteServerInfo->remoteServerIP.c_str(), remoteServerInfo->remoteServerPort, serverAddress))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "��Ч��Զ�̷�������ַ["<<remoteServerInfo.remoteServerIP.c_str()<<":"<<remoteServerInfo.remoteServerPort<<"]");
		return false;
	}

	connectSocketCtx->sockAddr = serverAddress;
	connectSocketCtx->socketType = CONNECT_SERVER_SOCKET;
	memcpy(&(connectSocketCtx->sockAddr), &serverAddress, sizeof(SOCKADDR_IN));

#ifdef _IOCP

	// ���ɱ����󶨵�ַ��Ϣ
	SOCKADDR_IN localAddress;
	while (1){
		if (false == CreateAddressInfo(localIP.c_str(), remoteServerInfo->localConnectPort, localAddress))
		{
			SendSocketAbnormal(connectSocketCtx);
			//LOG4CPLUS_ERROR(log.GetInst(), "��Ч�ı�����ַ["<<localIP.c_str()<<":"<<remoteServerInfo.localConnectPort<<"]");
			return false;
		}


		// �󶨵�ַ�Ͷ˿�
		if (SOCKET_ERROR == bind(connectSocketCtx->sock, (SOCKADDR *)&localAddress, sizeof(localAddress)))
		{
			srand(time(NULL));
			remoteServerInfo->localConnectPort = rand() % 65535;
			//LOG4CPLUS_ERROR(log.GetInst(), "���ӷ�����ʱbind()����ִ�д���.");
			continue;
		}

		break;
	}

	if (false == _AssociateWithIOCP(connectSocketCtx))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "��ɶ˿ڰ� connect Socketʧ�ܣ��������:" <<_GetLastError());
		SendSocketAbnormal(connectSocketCtx);
		return false;
	}

	if (false == _LoadWsafunc_ConnectEx(connectSocketCtx->sock))
	{
		SendSocketAbnormal(connectSocketCtx);
		return false;
	}

	connectSocketCtx->UpdataTimeStamp(timeStamp);

	IoContext* newIoCtx;	
	newIoCtx = connectSocketCtx->GetNewIoContext(1);
	newIoCtx->packBuf.len = 1;

	// ��ʼ���ӷ�����
	if (false == _PostConnect(newIoCtx,connectSocketCtx->sockAddr))
	{
		SendSocketAbnormal(connectSocketCtx);
		return false;
	}
#endif

	
#ifdef _EPOLL

	connectSocketCtx->UpdataTimeStamp(timeStamp);

	IoContext* newIoCtx;
	newIoCtx = connectSocketCtx->GetNewIoContext(1);
	newIoCtx->packBuf.len = 1;

	connectSocketCtx->AddToList(newIoCtx);

	if (false == _AddWithEpoll(newIoCtx, EPOLLOUT | EPOLLET | EPOLLONESHOT))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "�� connect Socketʧ�ܣ��������:" <<_GetLastError());
		SendSocketAbnormal(connectSocketCtx);
		return false;
	}

	int ret = connect(connectSocketCtx->sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
	
	if(ret < 0)
	{
		if( errno != EINPROGRESS ){
			SendSocketAbnormal(connectSocketCtx);
			//LOG4CPLUS_ERROR(log.GetInst(), "����Զ�̷�����ʧ��!" );
			return false;
		}
	}

#endif

	return true;
}


// ��ȡһ���µ�IoContext
IoContext* ServerContext::GetNewIoContext(int packContentSize)
{
	IoContext* p;

	if (useMultithreading)
		shardMemoryLock.Lock();

	int size = packContentSize + GetPackHeadLength();
	int fixSize;
	int i = _GetFixSizeIndex(size, fixSize);

	if (!ioctxFreeList[i].isempty()){
		p = *(ioctxFreeList[i].begin());
		p->packBuf.buf = p->buf;
		p->packBuf.len = p->maxBufSize;
		ioctxFreeList[i].delete_node(p->node);
		p->ResetBuffer();
	}
	else{
		p = new IoContext(fixSize);
	}

	if (useMultithreading)
		shardMemoryLock.UnLock();

	return p;
}

// ��ȡһ���µ�SocketContext
SocketContext* ServerContext::GetNewSocketContext(uint64_t timeStamp)
{
	SocketContext* p = 0;
	int tm = 0;

	if (!socketctxFreeList.isempty())
	{
		socketctxFreeList.reset();
		while (socketctxFreeList.next())
		{
			p = *(socketctxFreeList.cur());
			tm = timeStamp - p->timeStamp;

			if (p->holdCount == 0 && tm > 1000){
				socketctxFreeList.delete_node(p->node);
				p->node = 0;
				p->isRelease = false;
				return p;
			}
		}
	}
	else
	{
		p = new SocketContext(this);
	}

	return p;
}


bool ServerContext::PostSocketAbnormal(SocketContext* socketCtx)
{
	_PostTaskData(_OfflineSocketTask, 0, socketCtx);
	return true;
}

bool ServerContext::WaitConnectRemoteServer(RemoteServerInfo* remoteServerInfo, long delayMillisecond)
{
	useWaitConnectRemoteServer = true;
	_ConnectRemoteServer(remoteServerInfo);

	if (WAITSEM == waitConnectRemoteServerSem.WaitSem(delayMillisecond))
		return true;
	return false;
}

// ���ɵ�ַ��Ϣ
bool ServerContext::CreateAddressInfo(const char* name, u_short hostshort, SOCKADDR_IN& localAddress)
{
	// ���ɱ����󶨵�ַ��Ϣ
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
	// ��Ҫʹ���ص�IO�������ʹ��WSASocket������Socket���ſ���֧���ص�IO����
	return WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
#endif

#ifdef _EPOLL
	SOCKET sock = socket(PF_INET, SOCK_STREAM, 0);
	_SetNonBlocking(sock);
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


//��socketCtx��ӵ����б���
void ServerContext::_AddToFreeList(SocketContext *socketCtx)
{
	if (socketCtx)
	{
		if (useMultithreading)
			shardMemoryLock.Lock();

		socketCtx->UpdataTimeStamp();
		socketctxFreeList.push_back(socketCtx);
		socketCtx->SetNode(socketctxFreeList.get_last_node());

		if (useMultithreading)
			shardMemoryLock.UnLock();
	}
}


//��IoCtx��ӵ����б���
void ServerContext::_AddToFreeList(IoContext *ioCtx)
{
	if (ioCtx)
	{
		if (useMultithreading)
			shardMemoryLock.Lock();

		int fixSize;
		int i = _GetFixSizeIndex(ioCtx->maxBufSize, fixSize);
		ioctxFreeList[i].push_back(ioCtx);
		ioCtx->SetNode(ioctxFreeList[i].get_last_node());

		if (useMultithreading)
			shardMemoryLock.UnLock();
	}
}

// ���ͻ��˵������Ϣ�洢��������
void ServerContext::_AddToClientCtxList(SocketContext *socketCtx)
{
	clientCtxList.push_back(socketCtx);
	socketCtx->SetNode(clientCtxList.get_last_node());
}

void ServerContext::_AddToRemoteServerInfoList(RemoteServerInfo* remoteServerInfo)
{
	remoteServerInfoList.push_back(remoteServerInfo);
	remoteServerInfo->node = remoteServerInfoList.get_last_node();
	remoteServerInfo->socketCtx->node = remoteServerInfo->node;

	if (isCheckServerHeartBeat && remoteServerInfo->isCheckHeartBeat)
	{	
		PostTimerTask(_HeartBeatTask_CheckServer, 0, remoteServerInfo, remoteServerInfo->lifeDelayTime);
		PostTimerTask(_SendHeartBeatTask, 0, remoteServerInfo, remoteServerInfo->sendHeartBeatPackTime);
	}
}


//�Ƴ�ĳ���ض���Context
bool ServerContext::_RemoveSocketContext(SocketContext *socketCtx)
{
	if (socketCtx->sock == INVALID_SOCKET)
		return true;

	RemoteServerInfo* remoteServerInfo;
	SOCKET sock = socketCtx->sock;
	socketCtx->sock = INVALID_SOCKET;
	RELEASE_SOCKET(sock);
	socketCtx->ChangeToFreeList();

	switch (socketCtx->socketType)
	{
	case LISTEN_CLIENT_SOCKET:
		clientCtxList.delete_node(socketCtx->node);
		_AddToFreeList(socketCtx);
		return true;

	case CONNECTED_SERVER_SOCKET:
		remoteServerInfo = *(remoteServerInfoList.node(socketCtx->node));
		remoteServerInfo->socketCtx = 0;
		remoteServerInfoList.delete_node(socketCtx->node);
		_AddToFreeList(socketCtx);
		return true;

	default:
		_AddToFreeList(socketCtx);
		return true;
	}

	return false;
}

int ServerContext::_GetFixSizeIndex(int orgSize, int& fixSize)
{
	if (orgSize <= 16){
		fixSize = 16;
		return 0;
	}
	else if (orgSize > 16 && orgSize <= 32){
		fixSize = 32;
		return 1;
	}
	else if (orgSize > 32 && orgSize <= 64){
		fixSize = 64;
		return 2;
	}
	else if (orgSize >64 && orgSize <= 128){
		fixSize = 128;
		return 3;
	}
	else if (orgSize >128 && orgSize <= 256){
		fixSize = 256;
		return 4;
	}
	else if (orgSize >256 && orgSize <= 512){
		fixSize = 512;
		return 5;
	}
	else if (orgSize >512 && orgSize <= 1024){
		fixSize = 1024;
		return 6;
	}

	fixSize = MAX_BUFFER_LEN;
	return 7;
}

void ServerContext::_HeartBeat_CheckChildren()
{
	SocketContext* socketCtx;
	uint64_t time = GetSysTickCount64();
	uint64_t tm;

	//�ͻ����б�
	clientCtxList.reset();
	while (clientCtxList.next())
	{
		socketCtx = *(clientCtxList.cur());
		tm = time - socketCtx->timeStamp;

		//LOG4CPLUS_INFO(log.GetInst(), "SocketCtx:" << "[" << socketCtx << "]" << "ʱ���:" << socketCtx->timeStamp << "ms");
		//LOG4CPLUS_INFO(log.GetInst(), "time" << "ʱ���:" << time << "ms");
		//LOG4CPLUS_INFO(log.GetInst(), "socket�������ʱ��:" << tm << "ms");

		if (tm > childLifeDelayTime)
		{
			//LOG4CPLUS_INFO(log.GetInst(), "�ͻ���socketCtx��ʱ����Ӧ!");
			SendSocketAbnormal(socketCtx);
		}
	}
}


void ServerContext::_HeartBeat_CheckServer(RemoteServerInfo* remoteServerInfo)
{
	SocketContext* socketCtx;
	uint64_t time = GetSysTickCount64();
	uint64_t tm;

	//Զ�̷�����
	socketCtx = remoteServerInfo->socketCtx;
	tm = time - socketCtx->timeStamp;

	//LOG4CPLUS_INFO(log.GetInst(), "SocketCtx:" << "[" << socketCtx << "]" << "ʱ���:" << socketCtx->timeStamp << "ms");
	//LOG4CPLUS_INFO(log.GetInst(), "time" << "ʱ���:" << time << "ms");
	//LOG4CPLUS_INFO(log.GetInst(), "socket�������ʱ��:" << tm << "ms");

	if (tm > remoteServerInfo->lifeDelayTime)
	{
		//LOG4CPLUS_INFO(log.GetInst(), "�ͻ���socketCtx��ʱ����Ӧ!");
		SendSocketAbnormal(socketCtx);
	}	
}


void ServerContext::_ConnectedPack(SocketContext* socketCtx)
{
	char buf[10];
	PackBuf wsabuf = { 10, buf };
	CreatePack_Connected(&wsabuf);
	UnPack(socketCtx, (uint8_t*)buf);
}

void ServerContext::_NotPassRecvPack(SocketContext* socketCtx)
{
	char buf[10];
	PackBuf wsabuf = { 10, buf };
	CreatePack_NotPassRecv(&wsabuf);
	UnPack(socketCtx, (uint8_t*)buf);
}

void ServerContext::_AcceptedClientPack(SocketContext* socketCtx)
{
	char buf[10];
	PackBuf wsabuf = { 10, buf };
	CreatePack_Accepted(&wsabuf);
	UnPack(socketCtx, (uint8_t*)buf);
}

void ServerContext::_UnPack(SocketContext* socketCtx)
{
	uint8_t* packHead;

	while (socketCtx->NextPack())
	{
		packHead = socketCtx->CurPack();
		UnPack(socketCtx, packHead);
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
		//LOG4CPLUS_ERROR(log.GetInst(), "����Ͷ��ʧ��" );
		return false;
	}

	return true;
}