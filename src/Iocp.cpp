#include"ServerContext.h"

#ifdef _IOCP


//�������߳�:ΪIOCP�������Ĺ������߳�
//Ҳ����ÿ����ɶ˿��ϳ�����������ݰ����ͽ�֮ȡ�������д�����߳�
void* ServerContext::_WorkerThread(void* pParam)
{
	THREADPARAMS_WORKER* param = (THREADPARAMS_WORKER*)pParam;
	ServerContext* serverCtx = (ServerContext*)param->serverCtx;
	CmiWaitSem& waitSem = serverCtx->waitSem;
	//CmiLogger& log = serverCtx->log;
	int nThreadNo = (int)param->nThreadNo;

	////LOG4CPLUS_INFO(log.GetInst(), "�������߳�������ID:" << nThreadNo);

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


	// ѭ����������ֱ�����յ�Shutdown��ϢΪֹ
	while (WAITSEM != waitSem.WaitSem(0))
	{
		BOOL bReturn = GetQueuedCompletionStatus(
			serverCtx->iocp,
			&dwBytesTransfered,
			(PULONG_PTR)&socketCtx,
			&pOverlapped,
			INFINITE);

		// ����յ������˳���־����ֱ���˳�
		if (EXIT_CODE == (DWORD)socketCtx)
		{
			break;
		}
		// �ж��Ƿ�����˴���
		else if (!bReturn)
		{
			// ��ȡ����Ĳ���
			IoContext* ioCtx = CONTAINING_RECORD(pOverlapped, IoContext, overlapped);

			//LOG4CPLUS_ERROR(log.GetInst(), "GetQueuedCompletionStatus()����������! ���������ر�����SOCKET:"<<socketCtx);
			//LOG4CPLUS_ERROR(log.GetInst(), "�������������"<<socketCtx<<"�������ر���ɵ�.�������:" << WSAGetLastError());

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
			// ��ȡ����Ĳ���
			IoContext* ioCtx = CONTAINING_RECORD(pOverlapped, IoContext, overlapped);

	
			// �ж��Ƿ��пͻ��˶Ͽ���
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

	//LOG4CPLUS_TRACE(log.GetInst(), "�������߳�" <<nThreadNo<< "���˳�.");

	// �ͷ��̲߳���
	RELEASE(pParam);

	return 0;
}

// ��ʼ����ɶ˿�
bool ServerContext::_InitializeIOCP()
{
	// ������һ����ɶ˿�
	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	if (NULL == iocp)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "������ɶ˿�ʧ�ܣ��������:"<<WSAGetLastError());
		return false;
	}

	// ���ݱ����еĴ�����������������Ӧ���߳���
	nThreads = WORKER_THREADS_PER_PROCESSOR * GetNumProcessors();

	// Ϊ�������̳߳�ʼ�����
	workerThreads = new pthread_t[nThreads];

	// ���ݼ�����������������������߳�
	for (int i = 0; i < nThreads; i++)
	{
		THREADPARAMS_WORKER* pThreadParams = new THREADPARAMS_WORKER;
		pThreadParams->serverCtx = this;
		pThreadParams->nThreadNo = i + 1;

		pthread_create(&workerThreads[i], 0, _WorkerThread, (void*)pThreadParams);
	}

	return true;
}

//Ͷ��Connect����
bool ServerContext::_PostConnect(IoContext* ioCtx, SOCKADDR_IN& sockAddr)
{
	// ��ʼ������
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
			//LOG4CPLUS_ERROR(log.GetInst(), "Ͷ�� ConnectEx ����ʧ�ܣ��������:" << WSAGetLastError());
			return false;
		}
	}

	return true;
}

// Ͷ��Accept����
bool ServerContext::_PostAccept(IoContext* ioCtx)
{
	assert(INVALID_SOCKET != listenCtx->sock);

	// ׼������
	DWORD dwBytes = 0;
	ioCtx->postIoType = POST_IO_ACCEPT;
	PackBuf *p_wbuf = &ioCtx->packBuf;
	OVERLAPPED *p_ol = &ioCtx->overlapped;
	
	// Ϊ�Ժ�������Ŀͻ�����׼����Socket
	ioCtx->sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == ioCtx->sock)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "��������Accept��Socketʧ�ܣ��������:"<< WSAGetLastError());
		return false;
	}

	// Ͷ��AcceptEx
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
			//LOG4CPLUS_ERROR(log.GetInst(), "Ͷ�� AcceptEx ����ʧ�ܣ��������:" << WSAGetLastError());
			return false;
		}
	}


	return true;
}


// Ͷ�ݽ�����������
bool ServerContext::_PostRecv(IoContext* ioCtx)
{
	// ��ʼ������
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	//PackBuf p_wbuf ;
	PackBuf *p_wbuf = &(ioCtx->packBuf);
	OVERLAPPED *p_ol = &ioCtx->overlapped;

	ioCtx->ResetBuffer();
	ioCtx->postIoType = POST_IO_RECV;
	ioCtx->socketCtx->AddToList(ioCtx);

	// ��ʼ����ɺ�Ͷ��WSARecv����
	int nBytesRecv = WSARecv(ioCtx->sock, p_wbuf, 1, &dwBytes, &dwFlags, p_ol, NULL);

	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "Ͷ��WSARecvʧ��!");
		return false;
	}

	return true;
}


// Ͷ�ݷ�����������
bool ServerContext::_PostSend(IoContext* ioCtx)
{
	// ��ʼ������
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	PackBuf *p_wbuf = &(ioCtx->packBuf);
	OVERLAPPED *p_ol = &ioCtx->overlapped;

	ioCtx->postIoType = POST_IO_SEND;
	ioCtx->socketCtx->AddToList(ioCtx);

	// ��ʼ����ɺ�Ͷ��WSARecv����
	int nBytesRecv = WSASend(ioCtx->sock, p_wbuf, 1, &dwBytes, dwFlags, p_ol, NULL);

	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "Ͷ��WSASendʧ��!");
		return false;
	}

	return true;
}

// �����(Socket)�󶨵���ɶ˿���
bool ServerContext::_AssociateWithIOCP(SocketContext *socketCtx)
{
	// �����ںͿͻ���ͨ�ŵ�SOCKET�󶨵���ɶ˿���
	HANDLE hTemp = CreateIoCompletionPort((HANDLE)socketCtx->sock, iocp, (DWORD)socketCtx, 0);

	if (NULL == hTemp)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "ִ��CreateIoCompletionPort()���ִ���.������룺"<< GetLastError());
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

	// ΪAcceptEx ׼��������Ȼ��Ͷ��AcceptEx I/O����
	IoContext* ioCtx;
	for (int i = 0; i < MAX_POST_ACCEPT; i++)
	{
		// �½�һ��IO_CONTEXT
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

	//LOG4CPLUS_INFO(log.GetInst(), "Ͷ��"<< MAX_POST_ACCEPT << "��AcceptEx�������.");
	return true;
}

//���������ò������غ���ָ��
bool ServerContext::_LoadWsafunc_AccpetEx(SOCKET tmpsock)
{
	DWORD dwBytes = 0;

	// AcceptEx �� GetAcceptExSockaddrs ��GUID�����ڵ�������ָ��
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;

	// ʹ��AcceptEx��������Ϊ���������WinSock2�淶֮���΢�������ṩ����չ����
	// ������Ҫ�����ȡһ�º�����ָ�룬
	// ��ȡAcceptEx����ָ��

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
		//LOG4CPLUS_ERROR(log.GetInst(), "WSAIoctl δ�ܻ�ȡAcceptEx����ָ�롣�������: " << WSAGetLastError());
		return false;
	}

	// ��ȡGetAcceptExSockAddrs����ָ�룬Ҳ��ͬ��
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
		//LOG4CPLUS_ERROR(log.GetInst(), "WSAIoctl δ�ܻ�ȡGuidGetAcceptExSockAddrs����ָ�롣�������:" << WSAGetLastError());
		return false;
	}

	return true;
}

bool ServerContext::_LoadWsafunc_ConnectEx(SOCKET tmpsock)
{
	DWORD dwBytes = 0;

	// AcceptEx �� GetAcceptExSockaddrs ��GUID�����ڵ�������ָ��
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
		//LOG4CPLUS_ERROR(log.GetInst(), "WSAIoctl δ�ܻ�ȡConnectEx����ָ�롣�������:" << WSAGetLastError());
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

	// Ȼ��ʼͶ����һ��WSARecv����
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
