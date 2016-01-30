#ifndef _CHATCLIENT_H_
#define _CHATCLIENT_H_

#include"ServerContext.h"
#include"BaseType.h"
#include <fstream>
#include <iostream>
using namespace std;

class ChatClient :public ServerContext
{
public:
	ChatClient(int _id)
	{
		id = _id;
		recvSize = 0;
		useListen = false;
	//	isSendServerHeartBeat = true;
	}

	~ChatClient(void){}

	// 启动服务器
	virtual bool Start();

	//	停止服务器
	//virtual void Stop();

	void SetDownFile(char* _remoteFilePath);
	void StartDownLoad();


	void UnPack(Event ev, const SocketContext* socketCtx, const uint8_t* pack);

	void ProcessEvPackRecv(const SocketContext* socketCtx, const uint8_t* pack);
	void UnPack_ServerFileInfo(const SocketContext* socketCtx, const uint8_t* pack, int len);
	void UnPack_ServerFileBlock(const SocketContext* socketCtx, const uint8_t* pack, int len);
	void SendPack_RequestDownFile();
	void SendPack_BeginDownFile(uint64_t beg, uint64_t end);


private:

	virtual IoContext* CreateIoContext_PostConnectPack(SocketContext* socketCtx)
	{
		char* msg = "ok!";
		int len = strlen(msg);
		IoContext* ioCtx = socketCtx->GetNewIoContext(GetPackHeadLength() + len*sizeof(char));

		uint8_t* pack = CreatePackAddr(ioCtx, PACK_SEND_MSG, len*sizeof(char));
		memcpy(pack, msg, len*sizeof(char));
		return ioCtx;
	}


	static IoContext* CreateSendPack_HeartBeat(SocketContext* socketCtx);
	uint8_t* CreatePackAddr(IoContext* ioCtx, PackType packType, int packLen);
	static void* SendHeartBeatTask(void* data);


private:
	RemoteServerInfo gameServerInfo;

	const SocketContext* socketCtx;
	std::string localFilePath;
	std::string remoteFilePath;
	ofstream file;
	uint64_t size;
	uint64_t tellg_begin;
	uint64_t tellg_end;
	uint64_t reqRangeBeg;
	uint64_t reqRangeEnd;
	int id;
	uint64_t recvSize;
	uint64_t fileSize;
};

#endif