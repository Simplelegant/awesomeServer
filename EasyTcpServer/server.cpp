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
			printf("�˳�cmdThread�߳�\n");
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
			printf("��֧�ֵ�����.\n");
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
	//cellServer 4 �̴߳��� ����ȫ
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
			//printf("�յ��ͻ���<Socket=%d>����:CMD_LOGIN ���ݳ��ȣ�%d,userName=%s PassWord= %s\n", cSock, login->dataLength, login->userName, login->PassWord);
			//�����ж��û������Ƿ���ȷ�Ĺ���
			LoginResult* ret = new LoginResult();
			//pClient->SendData(&ret);
			//send(cSock, (char *)&ret, sizeof(LoginResult), 0);
			
			pCellServer->addSendTask(pClient, ret);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout *logout = (Logout*)header;
			//printf("�յ��ͻ���<Socket=%d>����:CMD_LOGOUT ���ݳ��ȣ�%d,userName=%s \n", cSock, logout->dataLength, logout->userName);
			//�����ж��û������Ƿ���ȷ�Ĺ���
			//LogoutResult ret;
			//send(cSock, (char *)&ret, sizeof(ret), 0);
			//SendData(cSock, &ret);
		}
		break;
		default:
		{
			printf("<Socket=%d>�յ�δ������Ϣ�����ݳ��ȣ�%d \n", pClient->sockfd(), header->dataLength);
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

	//�����߳�
	std::thread t1(cmdThread);
	t1.detach();
	while (g_bRun)
	{
		server.OnRun();
	}
	server.Close();
	printf("���˳������������\n");
	getchar();
	return 0;
}