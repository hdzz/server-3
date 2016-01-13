#ifndef _IOCONTEXT_H_
#define _IOCONTEXT_H_

#include"ServerBaseTypes.h"

struct IoContext
{
public:
#ifdef _IOCP
	OVERLAPPED      overlapped;                               // 每一个重叠网络操作的重叠结构(针对每一个Socket的每一个操作，都要有一个)
	int             transferedBytes;                          // 传输的字节数
	PostIoType      postIoType;                               // 标识网络操作的类型(对应上面的枚举)
#endif

	PackBuf         packBuf;                                   // WSA类型的缓冲区，用于给重叠操作传参数的
	char*           buf;                                       // 这个是PackBuf里具体存字符的缓冲区	
	int             maxBufSize;
	SOCKET          sock;
	SocketContext*  socketCtx;

public:
	// 初始化
	IoContext(int _maxBufSize = MAX_BUFFER_LEN);
	// 释放掉Socket
	~IoContext();

	void CopyPack(char* msgBuf, int msgLen)
	{
		if (msgBuf && msgLen > 0){
			memcpy(buf, msgBuf, msgLen);
			packBuf.len = msgLen;
		}
	}

	void Clear()
	{

	}

	// 重置缓冲区内容
	void ResetBuffer()
	{
		memset(buf, 0, maxBufSize);
	}

	void SetNode(void* nd)
	{
		node = nd;
	}

private:
	void*           node;
	uint64_t        timeStamp;

	friend class SocketContext;
	friend class ServerContext;

};

#endif