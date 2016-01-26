#ifndef _IOCONTEXT_H_
#define _IOCONTEXT_H_

#include"ServerBaseTypes.h"


typedef struct _IoContext
{

#ifdef _IOCP
	OVERLAPPED      overlapped;                               // 每一个重叠网络操作的重叠结构(针对每一个Socket的每一个操作，都要有一个)
#endif

	int             transferedBytes;                          // 传输的字节数
	PostIoType      postIoType;                               // 标识网络操作的类型(对应上面的枚举)
	PackBuf         packBuf;                                   // WSA类型的缓冲区，用于给重叠操作传参数的
	char*           buf;                                       // 这个是PackBuf里具体存字符的缓冲区
	int             maxBufSize;
	SOCKET          sock;
	SocketContext*  socketCtx;
	ServerContext* serverCtx;

}IoContext;

IoContext* CreateIoContext(int _maxBufSize = MAX_BUFFER_LEN);
void ResetBuffer(IoContext* ioCtx);
void ReleaseIoContext(IoContext* ioCtx);

#endif
