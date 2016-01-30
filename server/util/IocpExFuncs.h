#ifndef _IOCPEXFUNCS_H_
#define _IOCPEXFUNCS_H_

#ifdef _WIN32
#include<windows.h>
#include<winsock2.h>
#include<MSWSock.h>

extern LPFN_CONNECTEX               lpfnConnectEx;
extern LPFN_DISCONNECTEX            lpfnDisConnectEx;
extern LPFN_ACCEPTEX                lpfnAcceptEx;               // AcceptEx �� GetAcceptExSockaddrs �ĺ���ָ�룬���ڵ�����������չ����
extern LPFN_GETACCEPTEXSOCKADDRS    lpfnGetAcceptExSockAddrs;

bool LoadIocpExFuncs();
bool LoadWsafunc_AccpetEx(SOCKET tmpsock);
bool LoadWsafunc_ConnectEx(SOCKET tmpsock);

#endif

#endif