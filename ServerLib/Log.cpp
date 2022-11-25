#include "pch.h"
#include "Log.h"
#include <strsafe.h>
#include <locale>
#include <direct.h>

Log::eLogLevel Log::_logLevel = Log::eLogLevel::LL_DEBUG;
unsigned long long Log::_numLog = 0;
wchar_t Log::_directory[260] = {};
SRWLOCK	Log::_lock;

void Log::SET_LOGLEVEL(eLogLevel level)
{
	_logLevel = level;
}

void Log::SET_DIRECTORY(const wchar_t* wstrDirectory)
{
	GetModuleFileName(nullptr, _directory, MAX_PATH);
	
	size_t len = wcslen(_directory);
	for (int idx = len - 1; idx >= 0; --idx)
	{
		if (_directory[idx] == L'\\')
		{
			_directory[idx + 1] = L'\0';
			break;
		}
	}

	if (wstrDirectory != nullptr)	
	{
		StringCchCat(_directory, MAX_PATH, wstrDirectory);
		_wmkdir(_directory);
		StringCchCat(_directory, MAX_PATH, L"\\");
	}

	/*
	wchar_t tmp;
	tmp = _directory[1];
	_directory[1] = _directory[2];
	_directory[2] = tmp;
	*/
}

void Log::LOG(const wchar_t* wstrType, eLogLevel logLevel, const wchar_t* wstrFormat, ...)
{
	wchar_t inMsg[256];
	wchar_t outMsg[256];

	if (_logLevel > logLevel)
	{
		return;
	}

	va_list va;
	va_start(va, wstrFormat);
	StringCchVPrintf(inMsg, 256, wstrFormat, va);
	va_end(va);

	SYSTEMTIME time;
	GetLocalTime(&time);

	const wchar_t* wstrLogLevel = nullptr;
	switch (logLevel)
	{
	case Log::eLogLevel::LL_DEBUG:
		wstrLogLevel = L"DEBUG";
		break;
	case Log::eLogLevel::LL_SYSTEM:
		wstrLogLevel = L"SYSTEM";
		break;
	case Log::eLogLevel::LL_ERROR:
		wstrLogLevel = L"ERROR";
		break;
	default:
	{
		//crash
		int* a = nullptr;
		*a = 0;
	}
		break;
	}

	StringCchPrintf(outMsg, 256, L"[%s][%d-%02d-%02d %02d:%02d:%02d / %s / %08lld] %s\n", wstrType, time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, wstrLogLevel, _numLog, inMsg);
	size_t msgByteSize = wcslen(outMsg) * sizeof(wchar_t);

	StringCchPrintf(inMsg, 256, L"%ls%d%02d_%ls.txt", _directory, time.wYear, time.wMonth, wstrType);
	
	FILE* pFile;
	AcquireSRWLockExclusive(&_lock);
	errno_t errcode = _wfopen_s(&pFile, inMsg, L"a");

	fseek(pFile, 0, SEEK_END);
	fwprintf(pFile, outMsg);

	fclose(pFile);

	ReleaseSRWLockExclusive(&_lock);
}

void Log::Initialize()
{
	setlocale(LC_ALL, "Korean");
	InitializeSRWLock(&_lock);
}
