#include "pch.h"
#define _PROFILE

#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include "Profile.h"

static THREAD_PROFILE threadProfiles[10];
static int threadCount = 0;
static DWORD tlsIndex;

void ProfileBegin(const wchar_t* tagName)
{
	THREAD_PROFILE* tp = (THREAD_PROFILE*)TlsGetValue(tlsIndex);
	if (tp == nullptr)
	{
		int cnt = InterlockedIncrement((long*)&threadCount);
		tp = &threadProfiles[cnt - 1];
		tp->threadID = GetCurrentThreadId();
		TlsSetValue(tlsIndex, tp);
	}

	for (int idx = 0; idx < tp->count; idx++)
	{
		if (wcscmp(tagName, tp->profileInfos[idx].tagName) == 0)
		{
			tp->profileInfos[idx].callCount++;
			QueryPerformanceCounter(&tp->profileInfos[idx].startTime);
			return;
		}
	}

	wcscpy_s(tp->profileInfos[tp->count].tagName, tagName);
	tp->profileInfos[tp->count].callCount = 1;
	tp->profileInfos[tp->count].minTime = MAXINT64;
	tp->profileInfos[tp->count].maxTime = MININT64;
	QueryPerformanceCounter(&tp->profileInfos[tp->count++].startTime);
}

void ProfileEnd(const wchar_t* tagName)
{
	LARGE_INTEGER endTime;
	QueryPerformanceCounter(&endTime);

	THREAD_PROFILE* tp = (THREAD_PROFILE*)TlsGetValue(tlsIndex);

	for (int idx = 0; idx < tp->count; idx++)
	{
		if (wcscmp(tagName, tp->profileInfos[idx].tagName) == 0)
		{
			__int64 elapse = endTime.QuadPart - tp->profileInfos[idx].startTime.QuadPart;

			tp->profileInfos[idx].totalTime += elapse;

			if (elapse < tp->profileInfos[idx].minTime)
			{
				tp->profileInfos[idx].minTime = elapse;
			}
			
			if (elapse > tp->profileInfos[idx].maxTime)
			{
				tp->profileInfos[idx].maxTime = elapse;
			}

			return;
		}
	}
}

void ProfileDataOutText(const wchar_t* fileName)
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

	wchar_t name[256];
	wcscpy_s(name, fileName);
	wcscat_s(name, L".txt");

	FILE* profileFile = nullptr;
	errno_t result = _wfopen_s(&profileFile, name, L"w");
	if (result == 0)
	{
		fwprintf_s(profileFile, L"%15s|%43s|%15s|%15s|%15s|%15s\n", L"ThreadID", L"Name", L"Average", L"Min", L"Max", L"Call");

		for (int outIdx = 0; outIdx < threadCount; ++outIdx)
		{
			fwprintf_s(profileFile, L"%70s\n", L"------------------------------------------------------------------------------------------------------------------------");

			for (int idx = 0; idx < threadProfiles[outIdx].count; idx++)
			{
				double avg;
				if (threadProfiles[outIdx].profileInfos[idx].callCount == 0)
				{
					continue;
				}

				if (threadProfiles[outIdx].profileInfos[idx].callCount <= 2)
				{
					avg = (threadProfiles[outIdx].profileInfos[idx].totalTime / threadProfiles[outIdx].profileInfos[idx].callCount) / (double)freq.QuadPart * 1000.0 * 1000.0;
				}
				else
				{
					avg = ((threadProfiles[outIdx].profileInfos[idx].totalTime - threadProfiles[outIdx].profileInfos[idx].minTime - threadProfiles[outIdx].profileInfos[idx].maxTime) / (double)(threadProfiles[outIdx].profileInfos[idx].callCount - 2)) / (double)freq.QuadPart * 1000.0 * 1000.0;
				}

				fwprintf_s
				(
					profileFile,
					L"%15d|%40s|%13.2lfus|%13.2lfus|%13.2lfus|%15ld\n",
					threadProfiles[outIdx].threadID,
					threadProfiles[outIdx].profileInfos[idx].tagName,
					avg,
					threadProfiles[outIdx].profileInfos[idx].minTime / (double)freq.QuadPart * 1000.0 * 1000.0,
					threadProfiles[outIdx].profileInfos[idx].maxTime / (double)freq.QuadPart * 1000.0 * 1000.0,
					threadProfiles[outIdx].profileInfos[idx].callCount
				);
			}
		}
		fclose(profileFile);
	}
}

void ProfileReset(void)
{
	for (int outIdx = 0; outIdx < threadCount; ++outIdx)
	{
		for (int idx = 0; idx < threadProfiles[outIdx].count; idx++)
		{
			threadProfiles[outIdx].profileInfos[idx].callCount = 0;
			threadProfiles[outIdx].profileInfos[idx].totalTime = 0;
			threadProfiles[outIdx].profileInfos[idx].minTime = MAXINT64;
			threadProfiles[outIdx].profileInfos[idx].maxTime = MININT64;
		}
	}
}

void ProfileInitialize()
{
	tlsIndex = TlsAlloc();
	if (tlsIndex == TLS_OUT_OF_INDEXES)
	{
		//profile error!
		return;
	}
}

Profile::Profile(const wchar_t* tagName)
{
	wcscpy_s(this->tagName, tagName);
	PRO_BEGIN(this->tagName);
}

Profile::~Profile()
{
	PRO_END(this->tagName);
}
