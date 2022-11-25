#pragma once

#include <stack>
template <typename T>
class Stack
{
public:
	Stack() { InitializeSRWLock(&_lock); }
	size_t Size() const { return _stack.size(); }

	void Push(const T& data)
	{
		AcquireSRWLockExclusive(&_lock);
		_stack.push(data);
		ReleaseSRWLockExclusive(&_lock);
	}

	bool Pop(T& outData)
	{
		AcquireSRWLockExclusive(&_lock);
		if (_stack.size() == 0)
		{
			ReleaseSRWLockExclusive(&_lock);
			return false;
		}
		outData = _stack.top();
		_stack.pop();
		ReleaseSRWLockExclusive(&_lock);
		return true;
	}
private:
	SRWLOCK			_lock;
	std::stack<T>	_stack;
};

