
#include"ChatClient.h"

#pragma comment(lib,"ws2_32.lib")

int main()
{
	ChatClient::LoadSocketLib();
	ChatClient* chatClient[13];


	
	for (int i = 0; i < 10; i++)
	{
		chatClient[i] = new ChatClient(i);
		chatClient[i]->Start();
	}


	/*
	
	Sleep(1000);

	for (int i = 0; i < 20; i++)
	{
		chatClient[i] = new ChatClient(i, port++);
		chatClient[i]->Start();
	}


	Sleep(1000);

	for (int i = 0; i < 10; i++)
	{
		chatClient[i] = new ChatClient(i, port++);
		chatClient[i]->Start();
	}


	Sleep(1000);

	for (int i = 0; i < 20; i++)
	{
		chatClient[i] = new ChatClient(i, port++);
		chatClient[i]->Start();
	}
	Sleep(2000);

	for (int i = 0; i < 20; i++)
	{
		chatClient[i] = new ChatClient(i, port++);
		chatClient[i]->Start();
	}

	Sleep(2000);

	for (int i = 0; i < 30; i++)
	{
		chatClient[i] = new ChatClient(i, port++);
		chatClient[i]->Start();
	}
	*/
	
	Sleep(1000000);

	return 0;
}