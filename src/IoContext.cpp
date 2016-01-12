#include"IoContext.h"

IoContext::IoContext(int _maxBufSize)
{
#ifdef _IOCP
	memset(&overlapped, 0, sizeof(overlapped));
	postIoType = POST_IO_NULL;
	transferedBytes = 0;
#endif

	maxBufSize = _maxBufSize;

	buf = (char*)malloc(maxBufSize);
	memset(buf, 0, maxBufSize);

	sock = INVALID_SOCKET;
	packBuf.buf = buf;
	packBuf.len = maxBufSize;

}

//  Õ∑≈µÙSocket
IoContext::~IoContext()
{
	if (sock != INVALID_SOCKET){
		RELEASE_SOCKET(sock);
		sock = INVALID_SOCKET;
	}
}