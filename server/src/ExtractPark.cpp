#include"ExtractPack.h"
#include"ServerContext.h"

void ExtractPack::SetCurtPack(SocketContext* _curtPackSocketCtx, uint8_t* _curtPack, int _curtPackLen)
{
	curtPackSocketCtx = _curtPackSocketCtx;
	curtPack = _curtPack;
	curtPackLen = _curtPackLen;
}

int ExtractPack::Extract()
{
	int calcPackLen;
	int packHeadLen = serverCtx->GetPackHeadLength();
	PackBuf* cachePackBuf = &(curtPackSocketCtx->unPackCache);

	while (1)
	{


		if (cachePackBuf->buf == 0)
		{
			if (curtPackLen >= packHeadLen)
			{
				calcPackLen = serverCtx->GetPackLength(curtPack);
			}
			else
			{
				cachePackBuf->buf = (char*)malloc(MAX_BUFFER_LEN);

				cachePackBuf->len = curtPackLen;
				memcpy(cachePackBuf->buf, curtPack, cachePackBuf->len);
				return 1;
			}

			if (calcPackLen == curtPackLen)
			{
				serverCtx->UnPack(curtPackSocketCtx, curtPack);
				return 0;
			}
			else if (calcPackLen < curtPackLen)
			{
				memcpy(buf, curtPack, calcPackLen);
				buf[calcPackLen] = '\0';
				serverCtx->UnPack(curtPackSocketCtx, buf);

				curtPack += calcPackLen;
				curtPackLen -= calcPackLen;
			}
			else
			{
				cachePackBuf->buf = (char*)malloc(calcPackLen);

				cachePackBuf->len = curtPackLen;
				memcpy(cachePackBuf->buf, curtPack, cachePackBuf->len);
				return 1;
			}
		}
		else
		{
			if (cachePackBuf->len < packHeadLen)
			{
				int leavePackHeadLen = packHeadLen - cachePackBuf->len;

				if (curtPackLen < leavePackHeadLen)
				{
					memcpy(cachePackBuf->buf + cachePackBuf->len, curtPack, curtPackLen);
					cachePackBuf->len += curtPackLen;
					return 1;
				}
				else
				{
					memcpy(cachePackBuf->buf + cachePackBuf->len, curtPack, leavePackHeadLen);
					cachePackBuf->len += leavePackHeadLen;

					calcPackLen = serverCtx->GetPackLength((uint8_t*)(cachePackBuf->buf));
					calcPackLen -= packHeadLen - leavePackHeadLen;

					if (calcPackLen <= curtPackLen)
					{
						memcpy(cachePackBuf->buf + cachePackBuf->len, curtPack + leavePackHeadLen, calcPackLen - leavePackHeadLen);
						cachePackBuf->len += calcPackLen - leavePackHeadLen;
						serverCtx->UnPack(curtPackSocketCtx, (uint8_t*)(cachePackBuf->buf));

						free(cachePackBuf->buf);
						cachePackBuf->buf = 0;
						cachePackBuf->len = 0;

						if (calcPackLen == curtPackLen)
							return 0;

						curtPack += calcPackLen;
						curtPackLen -= calcPackLen;
					}
					else
					{
						memcpy(cachePackBuf->buf + cachePackBuf->len, curtPack + leavePackHeadLen, curtPackLen - leavePackHeadLen);
						cachePackBuf->len += curtPackLen - leavePackHeadLen;
						return 1;
					}
				}
			}
			else
			{
				calcPackLen = serverCtx->GetPackLength((uint8_t*)(cachePackBuf->buf));
				calcPackLen -= cachePackBuf->len;

				if (calcPackLen <= curtPackLen)
				{
					memcpy(cachePackBuf->buf + cachePackBuf->len, curtPack, calcPackLen);
					cachePackBuf->len += calcPackLen;
					serverCtx->UnPack(curtPackSocketCtx, (uint8_t*)(cachePackBuf->buf));

					free(cachePackBuf->buf);
					cachePackBuf->buf = 0;
					cachePackBuf->len = 0;

					if (calcPackLen == curtPackLen)
						return 0;

					curtPack += calcPackLen;
					curtPackLen -= calcPackLen;
				}
				else
				{
					memcpy(cachePackBuf->buf + cachePackBuf->len, curtPack, curtPackLen);
					cachePackBuf->len += curtPackLen;
					return 1;
				}
			}
		}
	}

	return 0;
}


