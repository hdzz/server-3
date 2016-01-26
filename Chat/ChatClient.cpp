#include "ChatClient.h"
#include<map>
#include <iostream>
using namespace std;

///////////////////////////////////////////////////////////////////
bool ChatClient::Start()
{

	if (!ServerContext::Start())
		return false;
	
	gameServerInfo.remoteServerIP.append("192.168.1.103");
	gameServerInfo.remoteServerPort = 6666;
	gameServerInfo.localConnectPort = port;
	gameServerInfo.isCheckHeartBeat = true;
	gameServerInfo.sendHeartBeatPackTime = 1000;
//	gameServerInfo.CreateSendPack_HeartBeat = CreateSendPack_HeartBeat;

	ConnectRemoteServer(&gameServerInfo);

	//workerThread = CreateThread(NULL, 0, _work, (void*)this, 0, NULL);

	//pthread_create(&workerThread, 0, _work, (void*)this);

	return true;
}


void ChatClient::UnPack(SocketContext* socketCtx, uint8_t* pack)
{
	PackHeader* header = (PackHeader*)pack;
	uint8_t* datapack = pack + sizeof(PackHeader);
	int len = header->len;
	static char* msg = "hellsd f!!!";
	static char* msgx = "secode my ok!!!";

	switch (header->type)
	{
	case PACK_SOCKET_OFFLINE:
		UnPack_SocketOffline(socketCtx, 0, 0);
		break;

	case PACK_SOCKET_ACCEPTED:
		break;

	case PACK_SEND_MSG:
	{
						  Sleep(10);
						  Unpack_Send_Msg(datapack, len);
						  SendPack_Msg(socketCtx, msg, strlen(msg));

						  /*
						  SendPack_Msg(socketCtx, msgx, strlen(msgx));
						  SendPack_Msg(socketCtx, msgx, strlen(msgx));

						  char* msgx4 = "secode my ok4!!!";
						  SendPack_Msg(socketCtx, msgx4, strlen(msgx4));
						  SendPack_Msg(socketCtx, msgx4, strlen(msgx4));
						  SendPack_Msg(socketCtx, msgx4, strlen(msgx4));


						  char* msgx3 = "secode my ok3!!!";
						  SendPack_Msg(socketCtx, msgx3, strlen(msgx3));
						*/
						 
	}
		break;

	case PACK_RESPONSE_SEND_MSG:
		break;

	case PACK_GAME_PLAY:
		break;
	}
}

void ChatClient::UnPack_SocketOffline(SocketContext* socketCtx, uint8_t* pack, int len)
{
	printf("client:server [%d] offline!! \n", (int)socketCtx);
}



void ChatClient::Unpack_Send_Msg(uint8_t* pack, int len)
{
	printf("client %d:%s\n",index++, pack);
}


void ChatClient::SendPack_Msg(SocketContext* socketCtx, char* msg, int len)
{

	IoContext* ioCtx = CreateIoContext();//new IoContext();
	ioCtx->socketCtx = socketCtx;
	ioCtx->sock = socketCtx->sock;
	ioCtx->serverCtx = socketCtx->serverCtx;

	//IoContext* ioCtx = socketCtx->GetNewIoContext();

	/*
	uint8_t* pack = CreatePackAddr(ioCtx, PACK_SEND_MSG, len*sizeof(char));
	memcpy(pack, msg, len*sizeof(char));

	if (false == _PostSend(ioCtx))
	{
		_AddToFreeList(ioCtx);
		SendSocketAbnormal(socketCtx);
	}
	*/
	

	uint8_t* pack = CreatePackAddr(ioCtx, PACK_SEND_MSG, len*sizeof(char));
	memcpy(pack, msg, len*sizeof(char));

	/*
	if (false == _PostSend(ioCtx))
	{
		ReleaseIoContext(ioCtx);
		SendSocketAbnormal(socketCtx);
	}
	*/

	PostSendPack(ioCtx);
}



void ChatClient::CreateSendPack_HeartBeat(IoContext* ioCtx)
{
	ChatClient* chatClient = (ChatClient*)(ioCtx->socketCtx->serverCtx);
	chatClient->CreatePackAddr(ioCtx, PACK_HEARTBEAT, 0);
}

uint8_t* ChatClient::CreatePackAddr(IoContext* ioCtx, PackType packType, int packLen)
{
	uint8_t* pack = (uint8_t*)(ioCtx->buf);
	PackHeader* packHead = (PackHeader*)pack;
	packHead->type = packType;
	packHead->len = packLen;
	ioCtx->packBuf.len = packHead->len + sizeof(PackHeader);
	return pack + sizeof(PackHeader);
}
