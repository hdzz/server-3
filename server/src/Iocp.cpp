#include"ServerContext.h"

#ifdef _IOCP


extern LPFN_CONNECTEX               lpfnConnectEx;
extern LPFN_DISCONNECTEX            lpfnDisConnectEx;
extern LPFN_ACCEPTEX                lpfnAcceptEx;               // AcceptEx 和 GetAcceptExSockaddrs 的函数指针，用于调用这两个扩展函数
extern LPFN_GETACCEPTEXSOCKADDRS    lpfnGetAcceptExSockAddrs;


//工作者线程:为IOCP请求服务的工作者线程
//也就是每当完成端口上出现了完成数据包，就将之取出来进行处理的线程
DWORD WINAPI ServerContext::_WorkerThread(LPVOID pParam)
{
	THREADPARAMS_WORKER* param = (THREADPARAMS_WORKER*)pParam;
	ServerContext* serverCtx = (ServerContext*)param->serverCtx;
	int nThreadNo = (int)param->nThreadNo;

	////LOG4CPLUS_INFO(log.GetInst(), "工作者线程启动，ID:" << nThreadNo);

	OVERLAPPED      *pOverlapped = NULL;
	SocketContext   *socketCtx = NULL;
	DWORD            dwBytesTransfered = 0;

	ExtractPack* extractPack = new ExtractPack(serverCtx);
	TlsSetValue(serverCtx->key, (LPVOID)extractPack);


	// 循环处理请求，直到接收到Shutdown信息为止
	while (serverCtx->exitWorkThreads != TRUE)
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
			ReleaseIoContext(ioCtx);
			serverCtx->PostSocketAbnormal(socketCtx);
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
				ReleaseIoContext(ioCtx);
			    serverCtx->_PostTask(_PostErrorTask, socketCtx);
			}
			else
			{
				ioCtx->transferedBytes = dwBytesTransfered;

				switch (ioCtx->postIoType)
				{
				case POST_IO_CONNECT:
					serverCtx->_PostTask(_DoConnectTask, ioCtx);
					break;

				case POST_IO_ACCEPT:
					serverCtx->_DoAccept(ioCtx);
					break;

				case POST_IO_RECV:
					serverCtx->_DoRecv(ioCtx);
					break;

				case POST_IO_SEND:
					serverCtx->_DoSend(ioCtx);
					break;
				}
			}
		}

	}


	delete extractPack;
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
	workerThreads = new HANDLE[nThreads];

	// 根据计算出来的数量建立工作者线程
	for (int i = 0; i < nThreads; i++)
	{
		THREADPARAMS_WORKER* pThreadParams = new THREADPARAMS_WORKER;
		pThreadParams->serverCtx = this;
		pThreadParams->nThreadNo = i + 1;

		workerThreads[i] = CreateThread(NULL, 0, _WorkerThread, (void*)pThreadParams, 0, NULL);
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

	ResetBuffer(ioCtx);//ioCtx->ResetBuffer();
	ioCtx->postIoType = POST_IO_RECV;

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

	// 初始化完成后，投递WSARecv请求
	int nBytesRecv = WSASend(ioCtx->sock, p_wbuf, 1, &dwBytes, dwFlags, p_ol, NULL);

	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "投递WSASend失败!");
		return false;
	}

	return true;
}


bool ServerContext::_InitPostAccept(SocketContext *listenCtx)
{
	return _PostTask(_PostInitAcceptTask, listenCtx);
}

#endif
