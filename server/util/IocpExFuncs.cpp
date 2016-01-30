
#ifdef _WIN32
#include"IocpExFuncs.h"

LPFN_CONNECTEX               lpfnConnectEx;
LPFN_DISCONNECTEX            lpfnDisConnectEx;
LPFN_ACCEPTEX                lpfnAcceptEx;               // AcceptEx �� GetAcceptExSockaddrs �ĺ���ָ�룬���ڵ�����������չ����
LPFN_GETACCEPTEXSOCKADDRS    lpfnGetAcceptExSockAddrs;


bool LoadIocpExFuncs()
{
	SOCKET tmpsock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (false == LoadWsafunc_AccpetEx(tmpsock))
	{
		return false;
	}

	if (false == LoadWsafunc_ConnectEx(tmpsock))
	{
		return false;
	}

	closesocket(tmpsock);
	return true;
}

//���������ò������غ���ָ��
bool LoadWsafunc_AccpetEx(SOCKET tmpsock)
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

bool LoadWsafunc_ConnectEx(SOCKET tmpsock)
{
	DWORD dwBytes = 0;

	// AcceptEx �� GetAcceptExSockaddrs ��GUID�����ڵ�������ָ��
	GUID GuidConnectEx = WSAID_CONNECTEX;
	GUID GuidDisconnectEx = WSAID_DISCONNECTEX;

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

	if (!lpfnDisConnectEx && WSAIoctl(
		tmpsock,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidDisconnectEx,
		sizeof(GuidDisconnectEx),
		&lpfnDisConnectEx,
		sizeof(lpfnDisConnectEx),
		&dwBytes,
		NULL,
		NULL))
	{
		return false;
	}

	return true;
}

#endif