#include"IoContext.h"


IoContext* CreateIoContext(int _maxBufSize)
{

	IoContext* ioCtx = (IoContext*)malloc(sizeof(IoContext));
    memset(ioCtx, 0, sizeof(IoContext));

	ioCtx->sock = INVALID_SOCKET;
	ioCtx->postIoType = POST_IO_NULL;

	ioCtx->maxBufSize = _maxBufSize;
	ioCtx->buf = (char*)malloc(ioCtx->maxBufSize);
	memset(ioCtx->buf, 0, ioCtx->maxBufSize);

	ioCtx->packBuf.buf = ioCtx->buf;
	ioCtx->packBuf.len = ioCtx->maxBufSize;

	return ioCtx;
}


void ResetBuffer(IoContext* ioCtx)
{
	memset(ioCtx->buf, 0, ioCtx->maxBufSize);
}



// 释放掉Socket
void ReleaseIoContext(IoContext* ioCtx)
{
	free(ioCtx->buf);
	free(ioCtx);
}
