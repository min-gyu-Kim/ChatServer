#pragma once

#ifdef _PROFILE
	#define PRO_BEGIN(Name)		ProfileBegin(Name)
	#define PRO_END(Name)		ProfileEnd(Name)
	#define PROFILE()			Profile ATTACH(p, __LINE__)(UNI(__FUNCSIG__))
	#define SAVE_PROFILE_DATA(Name) ProfileDataOutText(Name)

	#define UNI(text)			__UNI(text)
	#define __UNI(text)			L##text
	#define ATTACH(a, b)		__ATTACH(a, b)
	#define __ATTACH(a, b)		a##b
#else
	#define PRO_BEGIN(Name)
	#define PRO_END(Name)
	#define PROFILE()
	#define SAVE_PROFILE_DATA(Name)
#endif

typedef struct
{
	wchar_t			tagName[256];
	LARGE_INTEGER	startTime;

	__int64		totalTime;
	__int64		callCount;

	__int64		minTime;
	__int64		maxTime;
} PROFILE_INFO;

typedef struct
{
	PROFILE_INFO  profileInfos[50];
	DWORD		count;
	DWORD		threadID;
}THREAD_PROFILE;

void ProfileBegin(const wchar_t* tagName);
void ProfileEnd(const wchar_t* tagName);

void ProfileDataOutText(const wchar_t* fileName);
void ProfileReset(void);

void ProfileInitialize();

class Profile final
{
public:
	Profile(const wchar_t* tagName);
	~Profile();
private:
	wchar_t tagName[256];
};