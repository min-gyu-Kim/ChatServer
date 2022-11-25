#pragma once

#define WIN32_LEAN_AND_MEAN

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include <stdio.h>

#include <queue>
#include <stack>

#pragma comment(lib, "ws2_32.lib")

#include "Stack.h"
#include "Queue.h"
#include "MemoryPool.h"
#include "MemoryPoolTLS.h"
#include "Packet.h"
#include "RingBuffer.h"