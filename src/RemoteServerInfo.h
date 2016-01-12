#ifndef _REMOTESERVERINFO_H_
#define _REMOTESERVERINFO_H_

#include"ServerBaseTypes.h"

//�����ӵ�Զ�̷�����
class RemoteServerInfo
{
public:
	RemoteServerInfo();
	~RemoteServerInfo();

	void Copy(RemoteServerInfo& obj);
	
	SocketContext* GetSocketContext()
	{
		return socketCtx;
	}


	std::string                  remoteServerIP;             // Զ�̷�����IP��ַ
	int                          remoteServerPort;           // Զ�̷������Ķ˿�
	int                          localConnectPort;           // ����Զ�̷������Ķ˿�
	CreateSendPack               CreateSendPack_HeartBeat;
	int                          lifeDelayTime;              // ÿ�μ�鵽������������״̬��ʱ����
	int                          sendHeartBeatPackTime;      // ÿ�η������������     
	bool                         isCheckHeartBeat;           // �Ƿ�������
	                       
private:
	SocketContext*               socketCtx;                  // ����Զ�̷�������Context��Ϣ(�˱����洢��Զ�̷�������������Ϣ)
	void*                        node;

	friend class ServerContext;
	
};

#endif