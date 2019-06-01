#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifdef _WIN32
	#define FD_SETSIZE 2506
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <WinSock2.h>
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <unistd.h>  //unix std
	#include <arpa/inet.h>
	#include <string.h>

	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif

#include <stdio.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>

#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"
#include "CELLTask.hpp"

//��������С��Ԫ��С
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240*5
#define SEND_BUFF_SIZE RECV_BUFF_SIZE
#endif 

//�ͻ�����������
class ClientSocket
{
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, RECV_BUFF_SIZE);
		_lastPos = 0;
		memset(_szSendBuf, 0, SEND_BUFF_SIZE);
		_lastSendPos = 0;
	}

	SOCKET sockfd()
	{
		return _sockfd;
	}

	char* msgBuf()
	{
		return _szMsgBuf;
	}

	int getLastPos()
	{
		return _lastPos;
	}
	void setLastPos(int pos)
	{
		_lastPos = pos;
	}
	//��������
	int SendData(DataHeader* header)
	{
		int ret = SOCKET_ERROR;
		//Ҫ���͵����ݳ���
		int nSendLen = header->dataLength;
		//Ҫ���͵�����
		const char* pSendData = (const char*)header;
		while (true)
		{
			if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
			{
				//����ɿ��������ݳ���
				int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
				//��������
				memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
				pSendData += nCopyLen;
				nSendLen -= nSendLen;
				ret = send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
				_lastSendPos = 0;
				//���ʹ���
				if (SOCKET_ERROR == ret)
					return ret;
			}
			else {
				memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
				_lastSendPos += nSendLen;
				break;
			}
		}
		return ret;
	}
private:
	SOCKET _sockfd; //socket fd_set file desc set
	//�ڶ������� ��Ϣ������
	char _szMsgBuf[RECV_BUFF_SIZE];
	//��Ϣ������β��λ��
	int _lastPos;

	//�ڶ������� ���ͻ�����
	char _szSendBuf[RECV_BUFF_SIZE];
	//��Ϣ������β��λ��
	int _lastSendPos;
};
class CellServer;
//�����¼��ӿ�
class INetEvent
{
public:
	//�ͻ��˼����¼�
	virtual void OnNetJoin(ClientSocket* pClient) = 0;
	//�ͻ����뿪�¼�
	virtual void OnNetLeave(ClientSocket* pClient) = 0;
	//�ͻ�����Ϣ�¼�
	virtual void OnNetMsg(CellServer* CellServer, ClientSocket* pClient, DataHeader* header) = 0;
	//recv�¼�
	virtual void OnNetRecv(ClientSocket* pClient) = 0;
private:

};
//������Ϣ��������
class CellSendMsg2ClientTask : public CellTask
{
	ClientSocket* _pClient;
	DataHeader* _pHeader;
public:
	CellSendMsg2ClientTask(ClientSocket* pClient, DataHeader* header)
	{
		_pClient = pClient;
		_pHeader = header;
	}

	void doTask()
	{
		_pClient->SendData(_pHeader);
		delete _pHeader;
	}
};
//������Ϣ��������
class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET)
	{
		_sock = sock;
		_pThread = nullptr;
		_pNetEvent = nullptr;
	}

	~CellServer()
	{
		delete _pThread;
		Close();
		_sock = INVALID_SOCKET;
	}

	void setEventObj(INetEvent* event)
	{
		_pNetEvent = event;
	}

	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}
	//���ݿͻ�socket fd_set
	fd_set _fdRead_bak;
	//�ͻ��б��Ƿ��б仯
	bool _clients_change;
	SOCKET _maxSock;
	void OnRun()
	{
		_clients_change = true;
		while (isRun())
		{
			if (_clientsBuff.size() > 0)
			{//�ӻ��������ȡ���ͻ�����
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients[pClient->sockfd()] = pClient;
				}
				_clientsBuff.clear();
				_clients_change = true;
			}

			//���û����Ҫ����Ŀͻ��ˣ�������
			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

			//�������׽��� BSD socket
			fd_set fdRead;//��������socket�� ����
						  //������
			FD_ZERO(&fdRead);
			if (_clients_change)
			{
				_clients_change = false;
				//����������socket�����뼯��
				_maxSock = _clients.begin()->second->sockfd();
				for (auto iter : _clients)
				{
					FD_SET(iter.second->sockfd(), &fdRead);
					if (_maxSock < iter.second->sockfd())
					{
						_maxSock = iter.second->sockfd();
					}
				}
				memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
			}
			else {
				memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
			}

			///nfds ��һ������ֵ ��ָfd_set����������������(socket)�ķ�Χ������������
			///���������ļ����������ֵ+1 ��Windows�������������д0
			int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
			if (ret < 0)
			{
				printf("select���������\n");
				Close();
				return ;
			}
			else if (ret == 0)
			{
				continue;
			}

#ifdef _WIN32
			for (int n = 0; n < fdRead.fd_count; n++)
			{
				auto iter = _clients.find(fdRead.fd_array[n]);
				if (iter != _clients.end())
				{
					if (-1 == RecvData(iter->second))
					{
						if (_pNetEvent)
							_pNetEvent->OnNetLeave(iter->second);
						_clients_change = true;
						_clients.erase(iter->first);
					}
				}
				else {
					printf("error. if (iter != _clients.end())\n");
				}

			}
#else
			std::vector<ClientSocket*> temp;
			for (auto iter : _clients)
			{
				if (FD_ISSET(iter.second->sockfd(), &fdRead))
				{
					if (-1 == RecvData(iter.second))
					{
						if (_pNetEvent)
							_pNetEvent->OnNetLeave(iter.second);
						_clients_change = false;
						temp.push_back(iter.second);
					}
				}
			}
			for (auto pClient : temp)
			{
				_clients.erase(pClient->sockfd());
				delete pClient;
			}
#endif
		}
	}

	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (auto iter : _clients)
			{
				closesocket(iter.second->sockfd());
				delete iter.second;
			}
			//�ر��׽���closesocket
			closesocket(_sock);
#else
			for (auto iter : _clients)
			{
				close(iter.second->sockfd());
				delete iter.second;
			}
			//�ر��׽���closesocket
			close(_sock);
#endif
			_clients.clear();
		}
	}
	//������
	int RecvData(ClientSocket *pClient)
	{
		//5 ���տͻ�������
		char* szRecv = pClient->msgBuf() + pClient->getLastPos();
		int nLen = (int)recv(pClient->sockfd(), szRecv, (RECV_BUFF_SIZE) - pClient->getLastPos(), 0);
		_pNetEvent->OnNetRecv(pClient);
		//DataHeader *header = (DataHeader *)_szRecv;
		if (nLen <= 0)
		{
			//printf("�ͻ���<Socket=%d>���˳������������\n", pClient->sockfd());
			return -1;
		}
		//����ȡ�������ݿ�������Ϣ������
		//memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		//��Ϣ������������β��λ�ú���
		pClient->setLastPos(pClient->getLastPos() + nLen);
		//�ж���Ϣ�����������ݳ��ȴ�����ϢͷDataHeader����
		while (pClient->getLastPos() >= sizeof(DataHeader))
		{
			//��ʱ�Ϳ���֪����ǰ��Ϣ��ĳ���
			DataHeader *header = (DataHeader *)pClient->msgBuf();
			//�ж���Ϣ�����������ݳ��ȴ�����Ϣ����
			if (pClient->getLastPos() >= header->dataLength)
			{
				//��Ϣ������ʣ��δ������Ϣ���������ݵĳ���
				int nSize = pClient->getLastPos() - header->dataLength;
				//����������Ϣ
				OnNetMsg(pClient, header);
				//����Ϣ������ʣ��δ����δ��������
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				//����Ϣ������������β��λ��ǰ��
				pClient->setLastPos(nSize);
			}
			else {
				//��Ϣ������ʣ�����ݲ���һ��������Ϣ
				break;
			}
			return 0;
		}
	}

	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header)
	{
		_pNetEvent->OnNetMsg(this, pClient, header);
	}

	void addClient(ClientSocket* pClient)
	{
		_mutex.lock();
		_clientsBuff.push_back(pClient);
		_mutex.unlock();
	}

	void Start()
	{
		_pThread = new std::thread(std::mem_fn(&CellServer::OnRun), this);
		_taskServer.Start();
	}

	size_t getClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}

	void addSendTask(ClientSocket* pClient, DataHeader* header)
	{
		CellSendMsg2ClientTask* task = new CellSendMsg2ClientTask(pClient, header);
		_taskServer.addTask(task);
	}

private:
	SOCKET _sock;
	std::map<SOCKET, ClientSocket*> _clients;
	std::vector<ClientSocket*> _clientsBuff;
	//���������
	std::mutex _mutex;
	std::thread *_pThread;
	//�����¼�����
	INetEvent* _pNetEvent;
	//
	CellTaskServer _taskServer;

};

//new ���ڴ�
class EasyTcpServer : public  INetEvent
{
private:
	SOCKET _sock;

	//��Ϣ��������ڲ��ᴴ���߳�
	std::vector<CellServer*> _cellServers;
	//ÿ����Ϣ��ʱ
	CELLTimestamp _tTime;
protected:
	//SOCKET recv����
	std::atomic_int _recvCount;
	//�յ���Ϣ����
	std::atomic_int _msgCount; 
	//�ͻ��˼���
	std::atomic_int _clientCount;

public:

	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
		_recvCount = 0;
		_msgCount = 0;
		_clientCount = 0;
	}

	virtual ~EasyTcpServer()
	{
		Close();
	}
	//��ʼ��socket
	SOCKET InitSocket()
	{
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		//1
		if (INVALID_SOCKET != _sock)
		{
			printf("<socket=%d>�رվ�����...\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("���󣬽���socketʧ�ܡ�����\n");
		}
		else {
			printf("����socket=<%d>�ɹ�������\n", _sock);
		}
		return _sock;
	}
	//��IP�Ͷ˿ں�
	int Bind(const char* ip, unsigned short port)
	{
		/*if (INVALID_SOCKET == _sock)
		{
			InitSocket();
		}*/
		//2
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		if (ip) {
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip) {
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (ret == SOCKET_ERROR)
		{
			printf("���󣬰�����˿�<%d>ʧ��...\n", port);
		}
		else {
			printf("������˿�<%d>�ɹ�...\n", port);
		}
		return ret;
	}
	//�����˿ں�
	int Listen(int n)
	{
		//3
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			printf("socket=<%d>���󣬼�������˿�ʧ��...\n", (int)_sock);
		}
		else {
			printf("socket=<%d>��������˿ڳɹ�...\n", (int)_sock);
		}
		return ret;
	}
	//���ܿͻ�������
	int Accept()
	{
		//4
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET cSock = INVALID_SOCKET;
		//char msgBuf[] = "Hello, I'm Server.";
#ifdef _WIN32
		cSock = accept(_sock, (sockaddr *)&clientAddr, &nAddrLen);
#else
		cSock = accept(_sock, (sockaddr *)&clientAddr, (socklen_t *)&nAddrLen);
#endif 
		if (INVALID_SOCKET == cSock)
		{
			printf("socket=<%d>���󣬽��յ���Ч�ͻ���SOCKET...\n", (int)_sock);
		}
		else {
			//NewUserJoin userJoin;
			//SendDataToAll(&userJoin);	
			//���¿ͻ��˷�����ͻ��������ٵ�cellServer
			addClientToCellServer(new ClientSocket(cSock));
			//printf("socket=<%d>�¿ͻ���<%d>���룺socket = %d, IP = %s \n", (int)_sock, _clients.size(), (int)cSock, inet_ntoa(clientAddr.sin_addr));
		}
		return cSock;
	}
	
	void addClientToCellServer(ClientSocket* pClient)
	{
		//���ҿͻ��������ٵ�CellServer��Ϣ�������
		auto pMinServer = _cellServers[0];
		for (auto pCellServer  : _cellServers)
		{
			if (pMinServer->getClientCount() > pCellServer->getClientCount())
			{
				pMinServer = pCellServer;
			}
		}
		pMinServer->addClient(pClient);
		OnNetJoin(pClient);
	}
	void Start(int nCellServer)
	{
		for (int n = 0; n < nCellServer; n++)
		{
			auto ser = new CellServer(_sock);
			_cellServers.push_back(ser);
			//ע�������¼����ܶ���
			ser->setEventObj(this);
			//������Ϣ�����߳�
			ser->Start();
		}
	}
	//�ر�Socket
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			//8
			closesocket(_sock);
			WSACleanup();
#else
			close(_sock);
#endif
		}
	}
	//����������Ϣ
	bool OnRun()
	{
		if (isRun())
		{
			time4msg();
			//������ socket ������
			fd_set fdRead;
			//fd_set fdWrite;
			//fd_set fdExp;

			FD_ZERO(&fdRead);
			//FD_ZERO(&fdWrite);
			//FD_ZERO(&fdExp);

			FD_SET(_sock, &fdRead);
			//FD_SET(_sock, &fdWrite);
			//FD_SET(_sock, &fdExp);

			//nfds ��һ������ֵ����ָfd_set����������������(socket)�ķ�Χ������������
			//���������ļ����������ֵ+1 ��Windows�������������д0
			timeval t = { 0, 10 };
			int ret = select(_sock + 1, &fdRead, 0, 0, &t);
			if (ret < 0)
			{
				printf("Accept Select���������\n");
				Close();
				return false;
			}
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				Accept();
				return true;
			}
			return false;
		}
		return false;

	}
	//�Ƿ�����
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//���������ÿ���յ���������Ϣ
	virtual void time4msg()
	{
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			
			printf("thread<%d>,time<%lf>,socket<%d>,clients<%d>,recv<%d>,msg<%d>\n", _cellServers.size(), t1, _sock, (int)_clientCount, (int)(_recvCount / t1), (int)(_msgCount / t1));
			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
		
	}

	//ֻ�ᱻһ���̴߳���
	virtual void OnNetJoin(ClientSocket* pClient)
	{
		_clientCount++;
	}
	//cellServer 4 �̴߳��� ����ȫ
	virtual void OnNetLeave(ClientSocket* pClient)
	{
		_clientCount--;
	}
	virtual void OnNetMsg(CellServer* pCellServer,ClientSocket* pClient, DataHeader* header)
	{
		_msgCount++;
	}
};

#endif