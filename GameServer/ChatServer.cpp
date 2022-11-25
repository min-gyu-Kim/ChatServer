#include "ChatServer.h"
#include <Packet.h>
#include "Player.h"
#include <CrashDump.h>
#include <process.h>

ChatServer::ChatServer()
{
	InitializeSRWLock(&_mapLock);
	_messageQEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	_roomCounter = 0;

	_updateThread = (HANDLE)_beginthreadex(nullptr, 0, UpdateThread, this, 0, (unsigned int*)&_threadID);

	Room* pRoom = _roomPool.Alloc();
	pRoom->roomID = 0;
	wcscpy_s(pRoom->roomName, L"Hello~");
	_roomMap.insert(std::make_pair(0, pRoom));
	_roomCounter++;
}

bool ChatServer::OnConnectionRequest(unsigned int ipAddress, unsigned short port)
{
    return true;
}

void ChatServer::OnClientJoin(SessionID identity)
{
	AcquireSRWLockExclusive(&_mapLock);
	_playerMap.insert(std::make_pair(identity, _playerPool.Alloc()));
	ReleaseSRWLockExclusive(&_mapLock);
}

void ChatServer::OnClientLeave(SessionID identity)
{
	AcquireSRWLockExclusive(&_mapLock);
	_playerMap.erase(identity);
	ReleaseSRWLockExclusive(&_mapLock);
}

void ChatServer::OnRecv(SessionID identity, Packet* pPacket)
{
	_messageQueue.Push(Message{ identity, pPacket });
	SetEvent(_messageQEvent);
}

void ChatServer::OnError(int errorCode, const wchar_t* errorMsg)
{
}

unsigned ChatServer::UpdateThread(LPVOID arg)
{
	ChatServer* server = (ChatServer*)arg;
	server->updateThread();
	return 0;
}

void ChatServer::updateThread()
{
	Message message;
	while (true)
	{
		int result = WaitForSingleObject(_messageQEvent, INFINITE);
		if (WAIT_OBJECT_0 != result)
			CrashDump::Crash();

		while (_messageQueue.Size() > 0)
		{
			if (!_messageQueue.Pop(message))
				break;

			if (!processMessage(message))
			{				
				CrashDump::Crash();
			}
		}
	}
}

bool ChatServer::processMessage(const Message& message)
{
	PACKET_HEADER header;
	*message.packet >> header.code >> header.type;

	if (header.code != 0xff)
	{
		printf("error CODE!\n");
	}

	switch (header.type)
	{
	case CS_REQ_ROOM_LIST:
		sendRoomList(message.sessionID);
		break;

	case CS_REQ_CREATE_ROOM:
		createRoom(message.sessionID, message.packet);
		break;

	case CS_REQ_ENTER_ROOM:
		enterRoom(message.sessionID, message.packet);
		break;

	default:
		printf("there isn't message type\n");
		return false;
	}

	message.packet->SubRef();

	return true;
}

void ChatServer::sendRoomList(SessionID sessionID)
{
	Packet* pPacket = Packet::Alloc();

	pPacket->MoveWritePos(4);
	PACKET_HEADER* header = (PACKET_HEADER*)(pPacket->GetBufferPtr() + 2);

	*pPacket << (BYTE)_roomMap.size();
	for (auto room : _roomMap)
	{
		Room* pRoom = room.second;
		unsigned short len = wcslen(pRoom->roomName) * sizeof(wchar_t);
		*pPacket << pRoom->roomID << len;
		pPacket->PutData((char*)pRoom->roomName, len);
	}

	header->code = 0xff;
	header->type = SC_RES_ROOM_LIST;

	SendPacket(sessionID, pPacket);
}

void ChatServer::createRoom(SessionID sessionID, Packet* pPacket)
{
	Room* pRoom = _roomPool.Alloc();

	pRoom->sessions.clear();
	unsigned char roomNameSize;
	pRoom->roomID = _roomCounter++;

	*pPacket >> roomNameSize;
	pPacket->GetData((char*)pRoom->roomName, roomNameSize);

	_roomMap.insert(std::make_pair(pRoom->roomID, pRoom));
}

/* 
*		session count : 2
*		session IDs	  : N
*/
void ChatServer::enterRoom(SessionID sessionID, Packet* pPacket)
{
	DWORD roomID;
	*pPacket >> roomID;

	Room* pRoom = _roomMap.find(roomID)->second;
	pRoom->sessions.push_back(sessionID);

	Packet* packet = Packet::Alloc();
	packet->MoveWritePos(4);
	PACKET_HEADER* header = (PACKET_HEADER*)(packet->GetBufferPtr() + 2);

	*packet << (unsigned short)pRoom->sessions.size();
	for (auto sessionID : pRoom->sessions)
	{
		*packet << sessionID;
	}

	header->code = 0xff;
	header->type = SC_RES_ENTER_ROOM;
	
	SendPacket(sessionID, packet);
}
