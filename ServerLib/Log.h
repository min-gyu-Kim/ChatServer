#include <Windows.h>

class Log
{
public:
	enum class eLogLevel	: char
	{
		LL_DEBUG,
		LL_SYSTEM,
		LL_ERROR
	};
private:
	static unsigned long long _numLog;
	static eLogLevel _logLevel;
	static SRWLOCK	_lock;
	static wchar_t _directory[260];

public:
	static void Initialize();

	static void SET_LOGLEVEL(eLogLevel level);
	static void SET_DIRECTORY(const wchar_t* wstrDirectory);

	static void LOG(const wchar_t* wstrType, eLogLevel logLevel, const wchar_t* wstrFormat, ...);
};