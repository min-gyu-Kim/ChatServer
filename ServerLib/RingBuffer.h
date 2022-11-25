#pragma once

class RingBuffer
{
private:
	constexpr static unsigned int DEFAULT_SIZE = 10000;

public:
	RingBuffer();
	RingBuffer(int bufferSize);
	~RingBuffer();

	int GetUseSize() const;
	int GetFreeSize() const;

	int DirectEnqueueSize() const;
	int DIrectDequeueSize() const;

	int Enqueue(const char* buffer, int size);
	int Dequeue(char* outBuffer, int size);
	int Peek(char* outBuffer, int size);

	void MoveRear(int size);
	int MoveFront(int size);

	void ClearBuffer();

	char* GetFrontBufferPtr() const;
	char* GetRearBufferPtr() const;
	char* GetBufferPtr() const
	{
		return _buffer;
	}

	void Lock();
	void Unlock();

private:
	char* _buffer;
	unsigned int _frontOffset;
	unsigned int _rearOffset;
	unsigned int _bufferSize;

	SRWLOCK _lockObj;
};

