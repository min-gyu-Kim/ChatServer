#include "pch.h"
#include "CLanServer.h"
#define _PROFILE
#include "Profile.h"

#include <process.h>

#include "Packet.h"
#include "Log.h"

unsigned WINAPI CLanServer::AcceptThread(LPVOID arg)
{
	CLanServer* pServer = (CLanServer*)arg;

	SOCKADDR_IN clientAddr;
	int addrSize = sizeof(clientAddr);
	SOCKET clientSocket;
	DWORD waitRet;
	while (1)
	{
		waitRet = WaitForSingleObject(pServer->_hExitEvent, 0);
		if (waitRet != WAIT_TIMEOUT)
		{
			break;
		}

		clientSocket = accept(pServer->_listenSocket, (sockaddr*)&clientAddr, &addrSize);
		if (clientSocket == INVALID_SOCKET)
		{
			continue;
		}

		unsigned int index;

		if (false == pServer->_indexStack.Pop(index))
		{
			continue;
		}
		Session* pSession = &pServer->_sessionArray[index];

		pSession->_clientAddr = clientAddr;
		pSession->_clientSocket = clientSocket;
		pSession->_sessionID = ((UINT64)index << 48) | pServer->_sessionID++;
		pSession->_isAlive = true;
		pSession->_isRelease = false;

		if (!pServer->OnConnectionRequest(clientAddr.sin_addr.S_un.S_addr, clientAddr.sin_port))
		{
			pServer->ReleaseSession(pSession);
			continue;
		}
		
		pSession->_ioCount = 1;
		pSession->_sendFlag = false;
		pSession->_recvBuf.ClearBuffer();
		pSession->_sendPacketCnt = 0;		

		ZeroMemory(&pSession->_recvOverlapped, sizeof(WSAOVERLAPPED));
		ZeroMemory(&pSession->_sendOverlapped, sizeof(WSAOVERLAPPED));

		CreateIoCompletionPort((HANDLE)pSession->_clientSocket, pServer->_hIocp, (ULONG_PTR)pSession, 0);

		DWORD transferred;
		WSABUF wsabuf;
		int wsarecvRet;
		int lastError;
		DWORD flag = 0;
		wsabuf.buf = pSession->_recvBuf.GetRearBufferPtr();
		wsabuf.len = pSession->_recvBuf.DirectEnqueueSize();
		
		//recvpost 대체
		wsarecvRet = WSARecv(pSession->_clientSocket, &wsabuf, 1, &transferred, &flag, &pSession->_recvOverlapped, nullptr);
		if (wsarecvRet == SOCKET_ERROR)
		{
			lastError = WSAGetLastError();
			if (lastError != WSA_IO_PENDING)
			{
				if (lastError != WSAENOTSOCK && lastError != WSAECONNABORTED && lastError != WSAESHUTDOWN)
				{
					wprintf(L"Accept Thread wsarecvError! code : %d\n", lastError);
				}
				if(InterlockedDecrement16((SHORT*)&pSession->_ioCount) == 0)
					pServer->ReleaseSession(pSession);
				continue;
			}
		}

		pServer->OnClientJoin(pSession->_sessionID);
	}
	return 0;
}

unsigned __stdcall CLanServer::WorkerThread(LPVOID arg)
{
	CLanServer* pServer = (CLanServer*)arg;

	DWORD transferred;
	OVERLAPPED* pOverlapped;
	Session* pSession;

	int lastError;

	while (true)
	{
		transferred = 0;
		pOverlapped = nullptr;
		pSession = nullptr;

		GetQueuedCompletionStatus(pServer->_hIocp, &transferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);

		if (transferred == 0 &&
			pOverlapped == nullptr &&
			pSession == nullptr)
		{
			PostQueuedCompletionStatus(pServer->_hIocp, 0, 0, nullptr);
			break;
		}

		if (transferred == 0)
		{
			closesocket(pSession->_clientSocket);
			pSession->_clientSocket = INVALID_SOCKET;
			DWORD ioCount = InterlockedDecrement16((SHORT*)&pSession->_ioCount);
			if (ioCount == 0)
			{
				pServer->ReleaseSession(pSession);
				continue;
			}
			
			continue;
		}

		if (pOverlapped == &pSession->_recvOverlapped)
		{
			pSession->_recvBuf.MoveRear(transferred);

			Packet* pPacket;
			unsigned short header;
			int dqSize;
			while (1)
			{
				pPacket = Packet::Alloc();
				
				if (pSession->_recvBuf.Peek((char*)&header, sizeof(unsigned short)) != sizeof(unsigned short))
				{
					break;
				}

				if (pSession->_recvBuf.GetUseSize() < header + sizeof(header))
				{
					break;
				}
				pSession->_recvBuf.MoveFront(sizeof(header));
				dqSize = pSession->_recvBuf.Dequeue(pPacket->GetBufferPtr(), header);
				pPacket->MoveWritePos(dqSize);

				pServer->OnRecv(pSession->_sessionID, pPacket);
			}
			pServer->recvPost(pSession);
		}
		else if (pOverlapped == &pSession->_sendOverlapped)
		{
			Packet* pPacket;
			for (int i = 0; i < pSession->_sendPacketCnt; i++)
			{
				pPacket = pSession->_sendingPackets[i];
				pPacket->SubRef();
			}
			//wprintf(L"%d send\n", pSession->_sendPacketCnt);
			pSession->_sendPacketCnt = 0;

			InterlockedExchange8((char*)&pSession->_sendFlag, false);

			pServer->sendPost(pSession);
		}

		DWORD ioCount = InterlockedDecrement16((SHORT*)&pSession->_ioCount);
		if (ioCount == 0)
		{
			pServer->ReleaseSession(pSession);
			continue;
		}
	}
	return 0;
}

CLanServer::CLanServer()
{
	Log::Initialize();
	Log::SET_LOGLEVEL(Log::eLogLevel::LL_DEBUG);
	Log::SET_DIRECTORY(L"LOG");
}

bool CLanServer::Start(wchar_t* ipAddress, unsigned short port, unsigned int numWorkerThread, unsigned int numRunningThread, bool nagleOpt, unsigned int maxConnection, unsigned int bufferSize)
{
	IN_ADDR addr;
	int retVal = InetPton(AF_INET, ipAddress, &addr);
	if (retVal != 1)
	{
		OnError(WSAGetLastError(), L"Check ipAddress");
		return false;
	}

	return Start(addr.S_un.S_addr, port, numWorkerThread, numRunningThread, nagleOpt, maxConnection, bufferSize);
}

bool CLanServer::Start(unsigned int ipAddress, unsigned short port, unsigned int numWorkerThread, unsigned int numRunningThread, bool nagleOpt, unsigned int maxConnection, unsigned int bufferSize)
{
	if (initializeNetwork(ipAddress, port, nagleOpt) == false)
	{
		return false;
	}

	_maxSession = maxConnection;
	_sessionArray = new Session[maxConnection];

	for (short idx = maxConnection - 1; idx >= 0; --idx)
	{
		_indexStack.Push(idx);
	}

	_ipAddress = ipAddress;
	_port = port;

	_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, numRunningThread);
	if (_hIocp == INVALID_HANDLE_VALUE)
	{
		closesocket(_listenSocket);
		WSACleanup();
		return false;
	}

	_numWorkerThread = numWorkerThread;
	_hWorkerThreads = new HANDLE[_numWorkerThread];
	_dwWorkcerThreadsID = new DWORD[_numWorkerThread];

	_hExitEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (_hExitEvent == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	_hAcceptThread = (HANDLE)_beginthreadex(nullptr, 0, &CLanServer::AcceptThread, this, 0, (unsigned int*)&_dwAcceptThreadID);
	if (_hAcceptThread == (HANDLE)-1)
	{
		return false;
	}

	for (unsigned int idx = 0; idx < _numWorkerThread; ++idx)
	{
		_hWorkerThreads[idx] = (HANDLE)_beginthreadex(nullptr, 0, &CLanServer::WorkerThread, this, 0, (unsigned int*)&_dwWorkcerThreadsID[idx]);
		if (_hWorkerThreads[idx] == (HANDLE)-1)
		{
			return false;
		}
	}

    return false;
}

void CLanServer::Stop()
{
	cleanup();

	bool isClean = false;
	while (!isClean)
	{
		isClean = _indexStack.Size() == this->_maxSession;
		Sleep(1);
	}

	delete[] _sessionArray;
	PostQueuedCompletionStatus(_hIocp, 0, 0, nullptr);

	WaitForMultipleObjects(_numWorkerThread, _hWorkerThreads, TRUE, INFINITE);
	CloseHandle(_hIocp);

	unsigned int index;
	while (_indexStack.Pop(index) != false)
	{
		
	}
	
	WSACleanup();
}

int CLanServer::GetSessionCount()
{
	return _maxSession - _indexStack.Size();
}

bool CLanServer::Disconnect(SessionID identity)
{
	Session* pSession = nullptr;
	pSession = &_sessionArray[identity >> 48];

	if (pSession->_isRelease == true)
	{
		return false;
	}

	if (pSession->_sessionID != identity)
	{
		return false;
	}

	shutdown(pSession->_clientSocket, SD_BOTH);
	CancelIoEx((HANDLE)pSession->_clientSocket, nullptr);

    return true;
}

bool CLanServer::SendPacket(SessionID identity, Packet* pPacket)
{
	Session* pSession = nullptr;
	pSession = &_sessionArray[identity >> 48];

	InterlockedIncrement16((SHORT*)&pSession->_ioCount);
	
	if (pSession->_isRelease == true)
	{
		if (InterlockedDecrement16((SHORT*)&pSession->_ioCount) == 0)
		{
			ReleaseSession(pSession);
		}
		return false;
	}

	if (pSession->_sessionID != identity)
	{		
		if (InterlockedDecrement16((SHORT*)&pSession->_ioCount) == 0)
		{
			ReleaseSession(pSession);
		}
		return false;
	}

	unsigned short packetSize = pPacket->GetDataSize() - sizeof(unsigned short);
	memcpy(pPacket->GetBufferPtr(), &packetSize, sizeof(unsigned short));

	pPacket->AddRef();
	pSession->_sendBuf.Push(pPacket);
	sendPost(pSession);

	if (InterlockedDecrement16((SHORT*)&pSession->_ioCount) == 0)
	{
		ReleaseSession(pSession);
	}

    return true;
}

bool CLanServer::initializeNetwork(unsigned int ipAddress, unsigned short port, bool nagleOpt)
{
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSocket == INVALID_SOCKET)
	{
		return false;
	}

	linger ling;
	ling.l_onoff = nagleOpt;
	ling.l_linger = 0;
	setsockopt(_listenSocket, SO_LINGER, SOL_SOCKET, (const char*)&ling, sizeof(ling));

	// 송신버퍼 0
	int sendSize = 0;
	setsockopt(_listenSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendSize, sizeof(sendSize));

	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(SOCKADDR_IN));
	serverAddr.sin_addr.s_addr = ipAddress;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_family = AF_INET;
	if (bind(_listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) != 0)
	{
		OnError(WSAGetLastError(), L"Binding error");
		return false;
	}

	if (listen(_listenSocket, SOMAXCONN_HINT(65535)) != 0)
	{
		OnError(WSAGetLastError(), L"Listen error");
		return false;
	}

	return true;
}

void CLanServer::cleanup()
{
	CloseHandle(_hAcceptThread);

	for (unsigned int idx = 0; idx < _numWorkerThread; ++idx)
	{
		CloseHandle(_hWorkerThreads[idx]);
	}

	closesocket(_listenSocket);
	_listenSocket = INVALID_SOCKET;
	SetEvent(_hExitEvent);

	for (unsigned int idx = 0; idx < _maxSession; ++idx)
	{
		if (_sessionArray[idx]._isAlive)
		{
			closesocket(_sessionArray[idx]._clientSocket);
		}
	}

	delete[] _dwWorkcerThreadsID;
	delete[] _hWorkerThreads;
}

void CLanServer::recvPost(Session* pSession)
{
	WSABUF wsaBuf[2];
	int bufSize = 0;
	DWORD numRecieve;
	int lastError;
	PROFILE();
	if (pSession->_recvBuf.DirectEnqueueSize() == pSession->_recvBuf.GetFreeSize())
	{
		bufSize = 1;
		wsaBuf[0].buf = pSession->_recvBuf.GetRearBufferPtr();
		wsaBuf[0].len = pSession->_recvBuf.DirectEnqueueSize();
	}
	else
	{
		bufSize = 2;
		wsaBuf[0].buf = pSession->_recvBuf.GetRearBufferPtr();
		wsaBuf[0].len = pSession->_recvBuf.DirectEnqueueSize();
		wsaBuf[1].buf = pSession->_recvBuf.GetBufferPtr();
		wsaBuf[1].len = pSession->_recvBuf.GetFreeSize() - pSession->_recvBuf.DirectEnqueueSize();
	}

	ZeroMemory(&pSession->_recvOverlapped, sizeof(WSAOVERLAPPED));

	DWORD flag = 0;
	InterlockedIncrement16((SHORT*)&pSession->_ioCount);
	int recvRet = WSARecv(pSession->_clientSocket, wsaBuf, bufSize, &numRecieve, &flag, &pSession->_recvOverlapped, nullptr);
	if (recvRet == SOCKET_ERROR)
	{
		lastError = WSAGetLastError();
		if (lastError != WSA_IO_PENDING)
		{
			DWORD ioCnt = InterlockedDecrement16((SHORT*)&pSession->_ioCount);
			if (ioCnt == 0)
			{
				ReleaseSession(pSession);
			}
			if (lastError != WSAENOTSOCK && lastError != WSAECONNABORTED && lastError != WSAESHUTDOWN && lastError != WSAECONNRESET)
			{
				wprintf(L"wsarecv() error! code : %d\n", lastError);
			}
		}
	}
}

void CLanServer::sendPost(Session* pSession)
{
	WSABUF wsaBuf[200];
	Packet* packetPtr;
	int bufSize = 0;
	DWORD numRecieve;
	int lastError;
	int idx = 0;

	PROFILE();
	do
	{
		bool sendFlag = _InterlockedCompareExchange8((char*)&pSession->_sendFlag, true, false);
		//bool sendFlag = InterlockedExchange8((char*)&pSession->_sendFlag, true);
		if (sendFlag == true)
		{
			return;
		}

		int size = pSession->_sendBuf.Size();
		if (size == 0)
		{
			_InterlockedExchange8((char*)&pSession->_sendFlag, false);

			if (pSession->_sendBuf.Size() != 0)
				continue;

			return;
		}

		while (pSession->_sendBuf.Pop(packetPtr) != false && idx < 200)
		{
			wsaBuf[idx].buf = packetPtr->GetBufferPtr();
			wsaBuf[idx].len = packetPtr->GetDataSize();
			pSession->_sendingPackets[idx] = packetPtr;

			idx++;
		}
		pSession->_sendPacketCnt = idx;

		ZeroMemory(&pSession->_sendOverlapped, sizeof(WSAOVERLAPPED));

		DWORD numSend;
		DWORD ioCount = InterlockedIncrement16((SHORT*)&pSession->_ioCount);
		int sendRet = WSASend(pSession->_clientSocket, wsaBuf, pSession->_sendPacketCnt, &numSend, 0, &pSession->_sendOverlapped, nullptr);
		//printf("[%I64d] send %d\n", pSession->_sessionID & 0x0000ffffffffffff, numPacket);
		if (sendRet == SOCKET_ERROR)
		{
			lastError = WSAGetLastError();
			if (lastError != WSA_IO_PENDING)
			{
				DWORD ioCnt = InterlockedDecrement16((SHORT*)&pSession->_ioCount);
				if (ioCnt == 0)
				{
					ReleaseSession(pSession);
				}
				if (lastError != WSAENOTSOCK && lastError != WSAECONNABORTED && lastError != WSAESHUTDOWN && lastError != WSAECONNRESET)
				{
					wprintf(L"wsasend() error! code : %d\n", lastError);
				}
			}
		}
	} while (0);
}

void CLanServer::ReleaseSession(Session* pSession)
{
	long ioBlock = InterlockedCompareExchange((LONG*)&pSession->_isRelease, 1, 0);
	if (ioBlock != 0)
	{
		return;
	}

	closesocket(pSession->_clientSocket);

	pSession->_clientSocket = INVALID_SOCKET;

	Packet* pPacket;
	while (pSession->_sendBuf.Pop(pPacket) != false)
	{
		pPacket->SubRef();
	}

	OnClientLeave(pSession->_sessionID);
	pSession->_isAlive = false;
	_indexStack.Push(pSession->_sessionID >> 48);
}
