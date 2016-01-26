#ifndef _CHATCLIENT_H_
#define _CHATCLIENT_H_

#include"ServerContext.h"
#include"BaseType.h"


class ChatClient :public ServerContext
{
public:
	ChatClient(int _index, int _port, TaskProcesser* _taskProcesser = 0)
		:ServerContext(_taskProcesser)
	{
		useListen = false;
		//useWaitConnectRemoteServer = true;
		//useMultithreading = true;
	//	isSendServerHeartBeat = true;
		port = _port;
		index = _index;
	}

	~ChatClient(void){}

	// 启动服务器
	virtual bool Start();

	//	停止服务器
	//virtual void Stop();

	void UnPack(SocketContext* socketCtx, uint8_t* pack);


	void UnPack_SocketOffline(SocketContext* socketCtx, uint8_t* pack, int len);
	void SendPack_Msg(SocketContext* socketCtx, char* msg, int len);
	void Unpack_Send_Msg(uint8_t* pack, int len);


private:

	virtual bool CreatePack_OffLine(PackBuf* pack)
	{
		PackHeader* packHeader = (PackHeader*)(pack->buf);
		packHeader->type = PACK_SOCKET_OFFLINE;
		packHeader->len = 0;
		return true;
	}

	virtual bool CreatePack_Connected(PackBuf* pack)
	{
		PackHeader* packHeader = (PackHeader*)(pack->buf);
		packHeader->type = PACK_SOCKET_CONNECTED;
		packHeader->len = 0;
		return true;
	}

	virtual IoContext* CreateIoContext_PostConnectPack(SocketContext* socketCtx)
	{
		char* msg = "ok!";
		int len = strlen(msg);
		IoContext* ioCtx = socketCtx->GetNewIoContext(GetPackHeadLength() + len*sizeof(char));

		uint8_t* pack = CreatePackAddr(ioCtx, PACK_SEND_MSG, len*sizeof(char));
		memcpy(pack, msg, len*sizeof(char));
		return ioCtx;
	}



	static void CreateSendPack_HeartBeat(IoContext* ioCtx);
	



	uint8_t* CreatePackAddr(IoContext* ioCtx, PackType packType, int packLen);

	static void* SendHeartBeatTask(void* data);
	//static void* _work(void* data);
	static DWORD WINAPI _work(LPVOID data);

private:
	RemoteServerInfo gameServerInfo;
	int port;
	int index;
	//pthread_t                  workerThread;
	HANDLE workerThread;
};

#endif