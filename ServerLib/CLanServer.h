#pragma once

#include <unordered_map>
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "RingBuffer.h"
#include "Queue.h"
#include "Stack.h"

typedef UINT64 SessionID;

class Packet;

class CLanServer
{
private:
	struct Session
	{
		Session() : _recvBuf(), _sendBuf(), _isAlive(false) { }
		Session(unsigned int bufferSize) : _recvBuf(bufferSize) {}

		SOCKET			_clientSocket;
		SOCKADDR_IN		_clientAddr;
		SessionID		_sessionID;

		RingBuffer		_recvBuf;
		Queue<Packet*>	_sendBuf;

		Packet*			_sendingPackets[500];

		WSAOVERLAPPED		_recvOverlapped;
		WSAOVERLAPPED		_sendOverlapped;

		int				_sendPacketCnt;

		unsigned short	_isRelease;
		unsigned short	_ioCount;

		BOOL			_sendFlag;
		bool			_isAlive;
	};

public:
	CLanServer();
	virtual ~CLanServer() {}

	bool Start(wchar_t* ipAddress, unsigned short port, unsigned int numWorkerThread, unsigned int numRunningThread, bool nagleOpt = false, unsigned int maxConnection = 5000, unsigned int bufferSize = 10000);
	bool Start(unsigned int ipAddress, unsigned short port, unsigned int numWorkerThread, unsigned int numRunningThread, bool nagleOpt = false, unsigned int maxConnection = 5000, unsigned int bufferSize = 10000);
	void Stop();
	int GetSessionCount();

	bool Disconnect(SessionID identity);
	bool SendPacket(SessionID identity, Packet* pPacket);

	virtual bool OnConnectionRequest(unsigned int ipAddress, unsigned short port) = 0;
	virtual void OnClientJoin(SessionID identity) = 0;
	virtual void OnClientLeave(SessionID identity) = 0;

	virtual void OnRecv(SessionID identity, Packet* pPacket) = 0;
	virtual void OnError(int errorCode, const wchar_t* errorMsg) = 0;

private:
	static unsigned WINAPI AcceptThread(LPVOID);
	static unsigned WINAPI WorkerThread(LPVOID);

private:
	bool initializeNetwork(unsigned int ipAddress, unsigned short port, bool nagleOpt);
	void cleanup();

	void recvPost(Session* pSession);
	void sendPost(Session* pSession);

	void ReleaseSession(Session* pSession);

private:
	HANDLE	_hIocp;
	HANDLE	_hExitEvent;
	SOCKET	_listenSocket;
		
	Session* _sessionArray;
	unsigned int _maxSession;	
	Stack<unsigned int> _indexStack;

	unsigned int _numWorkerThread;
	HANDLE	_hAcceptThread;
	DWORD	_dwAcceptThreadID;
	HANDLE*	_hWorkerThreads;
	DWORD*	_dwWorkcerThreadsID;

	unsigned int _ipAddress;
	unsigned short _port;
	SessionID _sessionID;
};