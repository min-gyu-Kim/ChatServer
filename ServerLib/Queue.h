#pragma once
#include <queue>

template<typename T>
class Queue
{
public:
	size_t Size() const 
	{ 
		return _q.size(); 
	}

	void Push(const T& data) 
	{ 
		AcquireSRWLockExclusive(&_lock);
		_q.push(data); 
		ReleaseSRWLockExclusive(&_lock);
	}

	bool Pop(T& outData) 
	{ 
		AcquireSRWLockExclusive(&_lock);
		if (_q.size() == 0)
		{
			ReleaseSRWLockExclusive(&_lock);
			return false;
		}
		_q.pop(); 
		outData = _q.front();
		ReleaseSRWLockExclusive(&_lock);
		return true;
	}

private:
	SRWLOCK				_lock;
	std::queue<T>		_q;
};
