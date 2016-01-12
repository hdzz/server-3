#ifndef _EXTRACTPACK_H_
#define _EXTRACTPACK_H_

#include"ServerBaseTypes.h"

class ExtractPack
{
public:
	ExtractPack()
	:buf(0)
	, pos(0)
	, left(0)
	, len(0)
	, bufAllocLen(0)
	{
		buf = (uint8_t*)malloc(MAX_BUFFER_LEN);
		bufAllocLen = MAX_BUFFER_LEN;
	}

	~ExtractPack()
	{
		free(buf);
	}

	uint8_t* next();
	uint8_t* cur(){ return out; }

	void SetContext(
		ServerContext* _serverCtx,
		uint8_t* _cachebuf,
		int _cachelen)
	{
		serverCtx = _serverCtx;
		cachePack = _cachebuf;
		cacheLen = _cachelen;
	}


private:
	uint8_t        *out;

	uint8_t        *buf;
	int            pos;
	int            left;
	int            len;
	int            bufAllocLen;

	uint8_t*       cachePack;
	int            cacheLen;
	ServerContext* serverCtx;
};

#endif