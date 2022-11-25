#include "ChatServer.h"
#define _PROFILE
#include <Profile.h>

#pragma comment(lib, "ServerLib.lib")
#include <CrashDump.h>

long CrashDump::_DumpCount = 0;
CrashDump dump;

int main()
{
	ChatServer server;
	ProfileInitialize();

	if (server.Start(INADDR_ANY, 6000, 8, 4))
	{
		return -1;
	}

	while (true)
	{
		Sleep(1000);
	}

	SAVE_PROFILE_DATA(L"Profile");

	return 0;
}