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
	gameServerInfo.localConnectPort = 5565;
	gameServerInfo.isCheckHeartBeat = true;
	gameServerInfo.sendHeartBeatPackTime = 1000;
	gameServerInfo.CreateSendPack_HeartBeat = CreateSendPack_HeartBeat;

	ConnectRemoteServer(&gameServerInfo);
	return true;
}


void ChatClient::UnPack(Event ev, const SocketContext* socketCtx, const uint8_t* pack)
{
	switch (ev)
	{
	case EV_SOCKET_CONNECTED:
		ChatClient::socketCtx = socketCtx;
		SetDownFile("c://x.mp4");
		//SetDownFile("G://download//cn_windows_10_multiple_editions_x64_dvd_6848463.iso");
		StartDownLoad();
		break;

	case EV_SOCKET_OFFLINE:
		printf("offline!!\n");
		break;

	case EV_PACK_RECV:
		ProcessEvPackRecv(socketCtx, pack);
		break;
	}
}

void ChatClient::ProcessEvPackRecv(const SocketContext* socketCtx, const uint8_t* pack)
{
	PackHeader* header = (PackHeader*)pack;
	uint8_t* datapack = (uint8_t*)pack + sizeof(PackHeader);
	int len = header->len;

	switch (header->type)
	{
	case PACK_SERVER_FILE_INFO:
		UnPack_ServerFileInfo(socketCtx, datapack, len);
		break;

	case PACK_SERVER_FILE_BLOCK:
		UnPack_ServerFileBlock(socketCtx, datapack, len);
		break;
	}
}

void ChatClient::UnPack_ServerFileInfo(const SocketContext* socketCtx, const uint8_t* pack, int len)
{
	fileSize = *(uint64_t*)pack;

	if (size < fileSize)
	{
		SendPack_BeginDownFile(size, fileSize);
	}
}


void ChatClient::UnPack_ServerFileBlock(const SocketContext* socketCtx, const uint8_t* pack, int len)
{
	char* block = (char*)(pack + (sizeof(uint64_t)*2));
	recvSize += len - sizeof(uint64_t)* 2;
	file.write(block, len - sizeof(uint64_t)* 2);
  
	if (recvSize >= fileSize)
		file.close();
}


void ChatClient::SetDownFile(char* _remoteFilePath)
{
	remoteFilePath = _remoteFilePath;

	char buf[100] = {0};
	sprintf(buf, "c:\\x%d.mp4", id);
	localFilePath = buf;
}

void ChatClient::StartDownLoad()
{
	file.open(localFilePath, ios_base::binary | ios::app);
	tellg_begin = file.tellp();
	file.seekp(0, ios_base::end);
	tellg_end = file.tellp();
	size = tellg_end - tellg_begin;

	SendPack_RequestDownFile();
}



void ChatClient::SendPack_RequestDownFile()
{
	int packSize = remoteFilePath.size();

	IoContext* ioCtx = CreateIoContext(packSize + GetPackHeadLength());
	ioCtx->sock = socketCtx->sock;
	ioCtx->socketCtx = (SocketContext*)socketCtx;
	ioCtx->serverCtx = socketCtx->serverCtx;

	uint8_t* pack = CreatePackAddr(ioCtx, PACK_CLIENT_REQUEST_DOWNFILE, packSize);
	memcpy(pack, remoteFilePath.c_str(), remoteFilePath.size());
	SendPack(ioCtx);
}


void ChatClient::SendPack_BeginDownFile(uint64_t beg, uint64_t end)
{
	int packSize = sizeof(uint64_t) * 2;

	IoContext* ioCtx = CreateIoContext(packSize + GetPackHeadLength());
	ioCtx->sock = socketCtx->sock;
	ioCtx->socketCtx = (SocketContext*)socketCtx;
	ioCtx->serverCtx = socketCtx->serverCtx;

	uint8_t* pack = CreatePackAddr(ioCtx, PACK_CLIENT_BEGIN_DOWNFILE, packSize);

	uint64_t* pbeg = (uint64_t*)pack;
	*pbeg = beg;

	uint64_t* pend = (uint64_t*)(pack + sizeof(uint64_t));
	*pend = end;

	SendPack(ioCtx);
}


IoContext* ChatClient::CreateSendPack_HeartBeat(SocketContext* socketCtx)
{
	ChatClient* chatClient = (ChatClient*)(socketCtx->serverCtx);
	IoContext* ioCtx = socketCtx->GetNewIoContext(chatClient->GetPackHeadLength());
	chatClient->CreatePackAddr(ioCtx, PACK_HEARTBEAT, 0);
	return ioCtx;
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
