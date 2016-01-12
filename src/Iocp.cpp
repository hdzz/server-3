#include"ServerContext.h"

#ifdef _IOCP


//工作者线程:为IOCP请求服务的工作者线程
//也就是每当完成端口上出现了完成数据包，就将之取出来进行处理的线程
void* ServerContext::_WorkerThread(void* pParam)
{
	THREADPARAMS_WORKER* param = (THREADPARAMS_WORKER*)pParam;
	ServerContext* serverCtx = (ServerContext*)param->serverCtx;
	CmiWaitSem& waitSem = serverCtx->waitSem;
	//CmiLogger& log = serverCtx->log;
	int nThreadNo = (int)param->nThreadNo;

	////LOG4CPLUS_INFO(log.GetInst(), "工作者线程启动，ID:" << nThreadNo);

	OVERLAPPED      *pOverlapped = NULL;
	SocketContext   *socketCtx = NULL;
	DWORD            dwBytesTransfered = 0;


	task_func_cb  taskFunc[4]=
	{
		_DoConnectTask,
		_DoAcceptTask,
		_DoRecvTask,
		_DoSendTask
	};


	// 循环处理请求，直到接收到Shutdown信息为止
	while (WAITSEM != waitSem.WaitSem(0))
	{
		BOOL bReturn = GetQueuedCompletionStatus(
			serverCtx->iocp,
			&dwBytesTransfered,
			(PULONG_PTR)&socketCtx,
			&pOverlapped,
			INFINITE);

		// 如果收到的是退出标志，则直接退出
		if (EXIT_CODE == (DWORD)socketCtx)
		{
			break;
		}
		// 判断是否出现了错误
		else if (!bReturn)
		{
			// 读取传入的参数
			IoContext* ioCtx = CONTAINING_RECORD(pOverlapped, IoContext, overlapped);

			//LOG4CPLUS_ERROR(log.GetInst(), "GetQueuedCompletionStatus()非正常返回! 服务器将关闭问题SOCKET:"<<socketCtx);
			//LOG4CPLUS_ERROR(log.GetInst(), "这个可能是由于"<<socketCtx<<"非正常关闭造成的.错误代码:" << WSAGetLastError());

			if(ioCtx->postIoType == POST_IO_ACCEPT)
				serverCtx->_PostTask(_ReAccpetTask, socketCtx, ioCtx);
			else
			{
				if (serverCtx->useMultithreading)
					serverCtx->_OfflineSocket(socketCtx);
				else
					serverCtx->_PostTask(_OfflineSocketTask, socketCtx, ioCtx);
			}
			continue;
		}
		else 
		{
			// 读取传入的参数
			IoContext* ioCtx = CONTAINING_RECORD(pOverlapped, IoContext, overlapped);

	
			// 判断是否有客户端断开了
			if(dwBytesTransfered == 0 && 
				ioCtx->postIoType != POST_IO_ACCEPT)
			{	
				if (serverCtx->useMultithreading)
					serverCtx->_OfflineSocket(socketCtx);
				else
					serverCtx->_PostTask(_OfflineSocketTask,socketCtx, ioCtx);
			}
			else
			{
				ioCtx->transferedBytes = dwBytesTransfered;

				if (serverCtx->useMultithreading)
				{
					switch (ioCtx->postIoType)
					{
					case POST_IO_RECV:
						serverCtx->_DoRecv(ioCtx);	
						break;

					default:
						serverCtx->_PostTask(taskFunc[ioCtx->postIoType], socketCtx, ioCtx);
						break;
					}
				}
				else
				{
					serverCtx->_PostTask(taskFunc[ioCtx->postIoType], socketCtx, ioCtx);
				}
			}					
		}

	}

	//LOG4CPLUS_TRACE(log.GetInst(), "工作者线程" <<nThreadNo<< "号退出.");

	// 释放线程参数
	RELEASE(pParam);

	return 0;
}

// 初始化完成端口
bool ServerContext::_InitializeIOCP()
{
	// 建立第一个完成端口
	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	if (NULL == iocp)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "建立完成端口失败！错误代码:"<<WSAGetLastError());
		return false;
	}

	// 根据本机中的处理器数量，建立对应的线程数
	nThreads = WORKER_THREADS_PER_PROCESSOR * GetNumProcessors();

	// 为工作者线程初始化句柄
	workerThreads = new pthread_t[nThreads];

	// 根据计算出来的数量建立工作者线程
	for (int i = 0; i < nThreads; i++)
	{
		THREADPARAMS_WORKER* pThreadParams = new THREADPARAMS_WORKER;
		pThreadParams->serverCtx = this;
		pThreadParams->nThreadNo = i + 1;

		pthread_create(&workerThreads[i], 0, _WorkerThread, (void*)pThreadParams);
	}

	return true;
}

//投递Connect请求
bool ServerContext::_PostConnect(IoContext* ioCtx, SOCKADDR_IN& sockAddr)
{
	// 初始化变量
	DWORD dwBytes = 0;
	OVERLAPPED *p_ol = &ioCtx->overlapped;
	PackBuf *p_wbuf = &ioCtx->packBuf;

	ioCtx->postIoType = POST_IO_CONNECT;

	int rc = lpfnConnectEx(
		ioCtx->sock,
		(sockaddr*)&(sockAddr),
		sizeof(sockAddr),
		p_wbuf->buf,
		p_wbuf->len,
		&dwBytes,
		p_ol);

	if (rc == FALSE)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			//LOG4CPLUS_ERROR(log.GetInst(), "投递 ConnectEx 请求失败，错误代码:" << WSAGetLastError());
			return false;
		}
	}

	return true;
}

// 投递Accept请求
bool ServerContext::_PostAccept(IoContext* ioCtx)
{
	assert(INVALID_SOCKET != listenCtx->sock);

	// 准备参数
	DWORD dwBytes = 0;
	ioCtx->postIoType = POST_IO_ACCEPT;
	PackBuf *p_wbuf = &ioCtx->packBuf;
	OVERLAPPED *p_ol = &ioCtx->overlapped;
	
	// 为以后新连入的客户端先准备好Socket
	ioCtx->sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == ioCtx->sock)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "创建用于Accept的Socket失败！错误代码:"<< WSAGetLastError());
		return false;
	}

	// 投递AcceptEx
	if (FALSE == lpfnAcceptEx(
		listenCtx->sock,
		ioCtx->sock,
		p_wbuf->buf,
		0,                              //p_wbuf->len - ((sizeof(SOCKADDR_IN)+16) * 2)
		sizeof(SOCKADDR_IN)+16,
		sizeof(SOCKADDR_IN)+16,
		&dwBytes,
		p_ol))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			//LOG4CPLUS_ERROR(log.GetInst(), "投递 AcceptEx 请求失败，错误代码:" << WSAGetLastError());
			return false;
		}
	}


	return true;
}


// 投递接收数据请求
bool ServerContext::_PostRecv(IoContext* ioCtx)
{
	// 初始化变量
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	//PackBuf p_wbuf ;
	PackBuf *p_wbuf = &(ioCtx->packBuf);
	OVERLAPPED *p_ol = &ioCtx->overlapped;

	ioCtx->ResetBuffer();
	ioCtx->postIoType = POST_IO_RECV;
	ioCtx->socketCtx->AddToList(ioCtx);

	// 初始化完成后，投递WSARecv请求
	int nBytesRecv = WSARecv(ioCtx->sock, p_wbuf, 1, &dwBytes, &dwFlags, p_ol, NULL);

	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "投递WSARecv失败!");
		return false;
	}

	return true;
}


// 投递发送数据请求
bool ServerContext::_PostSend(IoContext* ioCtx)
{
	// 初始化变量
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	PackBuf *p_wbuf = &(ioCtx->packBuf);
	OVERLAPPED *p_ol = &ioCtx->overlapped;

	ioCtx->postIoType = POST_IO_SEND;
	ioCtx->socketCtx->AddToList(ioCtx);

	// 初始化完成后，投递WSARecv请求
	int nBytesRecv = WSASend(ioCtx->sock, p_wbuf, 1, &dwBytes, dwFlags, p_ol, NULL);

	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "投递WSASend失败!");
		return false;
	}

	return true;
}

// 将句柄(Socket)绑定到完成端口中
bool ServerContext::_AssociateWithIOCP(SocketContext *socketCtx)
{
	// 将用于和客户端通信的SOCKET绑定到完成端口中
	HANDLE hTemp = CreateIoCompletionPort((HANDLE)socketCtx->sock, iocp, (DWORD)socketCtx, 0);

	if (NULL == hTemp)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "执行CreateIoCompletionPort()出现错误.错误代码："<< GetLastError());
		return false;
	}

	return true;
}

bool ServerContext::_InitPostAccept(SocketContext *listenCtx)
{
	if (false == _LoadWsafunc_AccpetEx(listenCtx->sock))
	{
		return false;
	}

	// 为AcceptEx 准备参数，然后投递AcceptEx I/O请求
	IoContext* ioCtx;
	for (int i = 0; i < MAX_POST_ACCEPT; i++)
	{
		// 新建一个IO_CONTEXT
		ioCtx = listenCtx->GetNewIoContext(1);
		listenCtx->AddToList(ioCtx);
	}

	listenCtx->ioctxList.reset();
	while (listenCtx->ioctxList.next())
	{
		ioCtx = *(listenCtx->ioctxList.cur());
		if (false == _PostAccept(ioCtx))
		{
			return false;
		}
	}

	//LOG4CPLUS_INFO(log.GetInst(), "投递"<< MAX_POST_ACCEPT << "个AcceptEx请求完毕.");
	return true;
}

//本函数利用参数返回函数指针
bool ServerContext::_LoadWsafunc_AccpetEx(SOCKET tmpsock)
{
	DWORD dwBytes = 0;

	// AcceptEx 和 GetAcceptExSockaddrs 的GUID，用于导出函数指针
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;

	// 使用AcceptEx函数，因为这个是属于WinSock2规范之外的微软另外提供的扩展函数
	// 所以需要额外获取一下函数的指针，
	// 获取AcceptEx函数指针

	if (!lpfnAcceptEx && SOCKET_ERROR == WSAIoctl(
		tmpsock,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx,
		sizeof(GuidAcceptEx),
		&lpfnAcceptEx,
		sizeof(lpfnAcceptEx),
		&dwBytes,
		NULL,
		NULL))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "WSAIoctl 未能获取AcceptEx函数指针。错误代码: " << WSAGetLastError());
		return false;
	}

	// 获取GetAcceptExSockAddrs函数指针，也是同理
	if (!lpfnGetAcceptExSockAddrs && SOCKET_ERROR == WSAIoctl(
		tmpsock,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidGetAcceptExSockAddrs,
		sizeof(GuidGetAcceptExSockAddrs),
		&lpfnGetAcceptExSockAddrs,
		sizeof(lpfnGetAcceptExSockAddrs),
		&dwBytes,
		NULL,
		NULL))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "WSAIoctl 未能获取GuidGetAcceptExSockAddrs函数指针。错误代码:" << WSAGetLastError());
		return false;
	}

	return true;
}

bool ServerContext::_LoadWsafunc_ConnectEx(SOCKET tmpsock)
{
	DWORD dwBytes = 0;

	// AcceptEx 和 GetAcceptExSockaddrs 的GUID，用于导出函数指针
	GUID GuidConnectEx = WSAID_CONNECTEX;

	if (!lpfnConnectEx && SOCKET_ERROR == WSAIoctl(
		tmpsock,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidConnectEx,
		sizeof(GuidConnectEx),
		&lpfnConnectEx,
		sizeof(lpfnConnectEx),
		&dwBytes,
		NULL,
		NULL))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "WSAIoctl 未能获取ConnectEx函数指针。错误代码:" << WSAGetLastError());
		return false;
	}

	return true;
}


void ServerContext::_DoRecv(IoContext* ioCtx)
{
	SocketContext* socketCtx = ioCtx->socketCtx;

	socketCtx->UpdataTimeStamp();
	socketCtx->SetPackContext((uint8_t*)ioCtx->buf, ioCtx->transferedBytes);
	_UnPack(socketCtx);

	// 然后开始投递下一个WSARecv请求
	PostRecvPack(ioCtx);
}


void ServerContext::_OfflineSocket(SocketContext* socketCtx)
{
	if (socketCtx->isRelease == true)
		return;

	char buf[10];
	PackBuf wsabuf = { 10, buf };
	CreatePack_OffLine(&wsabuf);
	UnPack(socketCtx, (uint8_t*)buf);
	socketCtx->isRelease = true;

	PostRemoveSocketCtx(socketCtx);
}

#endif
