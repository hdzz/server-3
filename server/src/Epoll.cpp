#include"ServerContext.h"

#ifdef _EPOLL

void* ServerContext::_WorkerThread(void* pParam)
{
	THREADPARAMS_WORKER* param = (THREADPARAMS_WORKER*)pParam;
	ServerContext* serverCtx = (ServerContext*)param->serverCtx;
	int nThreadNo = (int)param->nThreadNo;
	EpollToIocp* epoll = serverCtx->epoll;
	SocketContext* socketCtx;
	IoContext* ioCtx;
    int dwBytesTransfered = 0;

	ExtractPack* extractPack = new ExtractPack(serverCtx);
    pthread_setspecific (serverCtx->key, extractPack);

	while(serverCtx->exitWorkThreads != 1)
	{
		bool bReturn = epoll->GetQueuedCompletionStatus(
			dwBytesTransfered,
			(void**)&socketCtx,
			(void**)&ioCtx);

		// 如果收到的是退出标志，则直接退出
		if (0 == (int)socketCtx)
		{
			break;
		}
		// 判断是否出现了错误
		else if (!bReturn)
		{
			//LOG4CPLUS_ERROR(log.GetInst(), "GetQueuedCompletionStatus()非正常返回! 服务器将关闭问题SOCKET:"<<socketCtx);
			//LOG4CPLUS_ERROR(log.GetInst(), "这个可能是由于"<<socketCtx<<"非正常关闭造成的.错误代码:" << WSAGetLastError());
			ReleaseIoContext(ioCtx);
			serverCtx->_PostTask(_PostErrorTask, socketCtx);
			continue;
		}
		else
		{

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
				//spd::get("file_logger")->info() << "dwBytesTransfered"<<dwBytesTransfered ;

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

	//LOG4CPLUS_TRACE(log.GetInst(), "工作者线程" <<nThreadNo<< "号退出.");
	delete extractPack;

	// 释放线程参数
	RELEASE(pParam);

	return 0;
}

// 初始化完成端口
bool ServerContext::_InitializeEPOLL()
{
     // 建立第一个完成端口
	 epoll = new EpollToIocp();

	// 根据本机中的处理器数量，建立对应的线程数
	//nThreads = WORKER_THREADS_PER_PROCESSOR * GetNumProcessors();
	nThreads = 10;

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
	ioCtx->postIoType = POST_IO_CONNECT;
	PackBuf *p_wbuf = &ioCtx->packBuf;

	int  ret = epoll->WSAConnect(ioCtx->sock,  (struct sockaddr *) &sockAddr, sizeof(sockAddr), p_wbuf , ioCtx);
    return true;
}

// 投递接受请求
bool ServerContext::_PostAccept(IoContext* ioCtx)
{
    // 准备参数
	ioCtx->postIoType = POST_IO_ACCEPT;
	PackBuf *p_wbuf = &ioCtx->packBuf;

	int ret = epoll->WSAAccept(ioCtx->sock, p_wbuf,  1, ioCtx);
	return true;
}

// 投递发送数据请求
bool ServerContext::_PostRecv(IoContext* ioCtx)
{
	PackBuf *p_wbuf = &(ioCtx->packBuf);

	ResetBuffer(ioCtx);
	ioCtx->postIoType = POST_IO_RECV;

	// 初始化完成后，投递WSARecv请求
	int nBytesRecv = epoll->WSARecv(ioCtx->sock, p_wbuf, 1,  ioCtx);
	return true;
}

// 投递发送数据请求
bool ServerContext::_PostSend(IoContext* ioCtx)
{
	// 初始化变量
	PackBuf *p_wbuf = &(ioCtx->packBuf);
	ioCtx->postIoType = POST_IO_SEND;

	// 初始化完成后，投递WSARecv请求
	int nBytesRecv = epoll->WSASend(ioCtx->sock, p_wbuf, 1, ioCtx);
	return true;
}

bool ServerContext::_InitPostAccept(SocketContext *listenCtx)
{
	// 为AcceptEx 准备参数，然后投递AcceptEx I/O请求
	IoContext* ioCtx;
	for (int i = 0; i < MAX_POST_ACCEPT + 10; i++)
	{
		// 新建一个IO_CONTEXT
		ioCtx = listenCtx->GetNewIoContext();
		if (false == _PostAccept(ioCtx))
		{
			ReleaseIoContext(ioCtx);
		}
	}

	//LOG4CPLUS_INFO(log.GetInst(), "投递"<< MAX_POST_ACCEPT << "个AcceptEx请求完毕.");
	return true;
}

#endif
