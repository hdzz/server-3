#ifndef _IOCONTEXT_H_
#define _IOCONTEXT_H_

#include"ServerBaseTypes.h"

struct IoContext
{
public:
#ifdef _IOCP
	OVERLAPPED      overlapped;                               // ÿһ���ص�����������ص��ṹ(���ÿһ��Socket��ÿһ����������Ҫ��һ��)
	int             transferedBytes;                          // ������ֽ���
	PostIoType      postIoType;                               // ��ʶ�������������(��Ӧ�����ö��)
#endif

	PackBuf         packBuf;                                   // WSA���͵Ļ����������ڸ��ص�������������
	char*           buf;                                       // �����PackBuf�������ַ��Ļ�����	
	int             maxBufSize;
	SOCKET          sock;
	SocketContext*  socketCtx;

public:
	// ��ʼ��
	IoContext(int _maxBufSize = MAX_BUFFER_LEN);
	// �ͷŵ�Socket
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

	// ���û���������
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