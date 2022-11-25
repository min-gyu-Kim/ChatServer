#pragma once
//#include <ServerPch.h>
#include <CLanServer.h>
#include <unordered_map>
#include <MemoryPoolTLS.h>

#include "Player.h"

struct PACKET_HEADER
{
	unsigned char		code;
	unsigned char		type;
};

enum : unsigned char
{
	CS_REQ_ROOM_LIST = 0,
	SC_RES_ROOM_LIST = 1,
	CS_REQ_CREATE_ROOM = 2,
	CS_REQ_ENTER_ROOM = 3,
	SC_RES_ENTER_ROOM = 4
};

class ChatServer : public CLanServer
{
	struct Message
	{
		SessionID	sessionID;
		Packet*		packet;
	};

	struct Room
	{
		DWORD						roomID;
		WCHAR						roomName[30];
		std::vector<SessionID>		sessions;
	};

public:
	ChatServer();
private:
	virtual bool OnConnectionRequest(unsigned int ipAddress, unsigned short port) override;
	virtual void OnClientJoin(SessionID identity) override;
	virtual void OnClientLeave(SessionID identity) override;
	virtual void OnRecv(SessionID identity, Packet* pPacket) override;
	virtual void OnError(int errorCode, const wchar_t* errorMsg) override;

private:
	static unsigned CALLBACK UpdateThread(LPVOID arg);

private:
	void updateThread();
	bool processMessage(const Message& message);

	void sendRoomList(SessionID sessionID);
	void createRoom(SessionID sessionID, Packet* pPacket);
	void enterRoom(SessionID sessionID, Packet* pPacket);

private:
	SRWLOCK										_mapLock;
	std::unordered_map<SessionID, Player*>		_playerMap;
	MemoryPoolTLS<Player>						_playerPool;

	HANDLE										_messageQEvent;

	Queue<Message>								_messageQueue;

	MemoryPoolTLS<Room>							_roomPool;
	std::unordered_map<DWORD, Room*>			_roomMap;
	DWORD										_roomCounter;

	HANDLE										_updateThread;
	DWORD										_threadID;
};