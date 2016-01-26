#ifndef _EXTRACTPACK_H_
#define _EXTRACTPACK_H_

#include"ServerBaseTypes.h"

class ExtractPack
{
public:
	ExtractPack(ServerContext* _serverCtx)
	:buf(0)
	{
		buf = (uint8_t*)malloc(MAX_BUFFER_LEN);
		serverCtx = _serverCtx;
	}

	~ExtractPack()
	{
		free(buf);
	}

	int Extract();
	void SetCurtPack(SocketContext* _curtPackSocketCtx, uint8_t* _curtPack, int _curtPackLen);


private:
	uint8_t        *buf;
	SocketContext* curtPackSocketCtx;
	uint8_t*       curtPack;
	int            curtPackLen;
	ServerContext* serverCtx;
};

#endif