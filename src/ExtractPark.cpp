#include"ExtractPack.h"
#include"ServerContext.h"

uint8_t* ExtractPack::next()
{
	while (1)
	{
		if (len > 0)
		{
			if (left > cacheLen)
			{
				memcpy(buf + pos, cachePack, cacheLen);
				pos += len;
				len -= cacheLen;
				left -= cacheLen;
				cachePack = 0;
				cacheLen = 0;
				out = 0;
				return 0;
			}
			else
			{
				memcpy(buf + pos, cachePack, left);
				left = 0;
				pos = 0;
				cachePack += left;
				cacheLen -= left;
				len = 0;
				out = buf;
				return out;
			}
		}
		else
		{
			if (cacheLen >= serverCtx->GetPackHeadLength())
			{
				len = serverCtx->GetPackLength(cachePack);
			}
			else{
				out = 0;
				return 0;
			}

			if (len <= cacheLen)
			{
				out = cachePack;
				cachePack += len;
				cacheLen -= len;
				len = 0;
				return out;
			}
		}
	}
}


