#include "EasyTcpServer.hpp"
#include <thread>
bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			bool g_bRun =false;
			printf("退出cmdThread线程\n");
			break;
		}
		/*else if (0 == strcmp(cmdBuf, "login"))
		{
		Login login;
		strcpy(login.userName, "lyd");
		strcpy(login.PassWord, "lyd");
		client->SendData(&login);
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
		Logout logout;
		strcpy(logout.userName, "lyd");
		client->SendData(&logout);
		}*/
		else {
			printf("不支持的命令.\n");
		}
	}
}

class MyServer  : public EasyTcpServer
{
public:
	virtual void OnNetJoin(ClientSocket* pClient)
	{
		EasyTcpServer::OnNetJoin(pClient);
	}
	//cellServer 4 线程触发 不安全
	virtual void OnNetLeave(ClientSocket* pClient)
	{
		EasyTcpServer::OnNetLeave(pClient);
	}
	virtual void OnNetMsg(CellServer* pCellServer,ClientSocket* pClient, DataHeader* header)
	{
		EasyTcpServer::OnNetMsg(pCellServer,pClient, header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			//printf("收到客户端<Socket=%d>请求:CMD_LOGIN 数据长度：%d,userName=%s PassWord= %s\n", cSock, login->dataLength, login->userName, login->PassWord);
			//忽略判断用户密码是否正确的过程
			LoginResult* ret = new LoginResult();
			//pClient->SendData(&ret);
			//send(cSock, (char *)&ret, sizeof(LoginResult), 0);
			
			pCellServer->addSendTask(pClient, ret);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout *logout = (Logout*)header;
			//printf("收到客户端<Socket=%d>请求:CMD_LOGOUT 数据长度：%d,userName=%s \n", cSock, logout->dataLength, logout->userName);
			//忽略判断用户密码是否正确的过程
			//LogoutResult ret;
			//send(cSock, (char *)&ret, sizeof(ret), 0);
			//SendData(cSock, &ret);
		}
		break;
		default:
		{
			printf("<Socket=%d>收到未定义消息，数据长度：%d \n", pClient->sockfd(), header->dataLength);
			//DataHeader ret;
			//SendData(cSock, &ret);
		}
		break;
		}
	}

	virtual void OnNetRecv(ClientSocket* pClient)
	{
		_recvCount++;
		//printf("client<%d> leave\n", pClient->sockfd());
	}
private:

};


int main()
{

	MyServer server;
	server.InitSocket();
	server.Bind(nullptr, 4551);
	server.Listen(5);
	server.Start(4);

	//启动线程
	std::thread t1(cmdThread);
	t1.detach();
	while (g_bRun)
	{
		server.OnRun();
	}
	server.Close();
	printf("已退出，任务结束。\n");
	getchar();
	return 0;
}