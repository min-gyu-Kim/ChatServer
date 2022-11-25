#include "pch.h"
#include <stdlib.h>
#include <memory.h>
#include <Windows.h>
#include "RingBuffer.h"

RingBuffer::RingBuffer()
{
    _bufferSize = DEFAULT_SIZE;
    _buffer = (char*)malloc(_bufferSize);
    _frontOffset = 0;
    _rearOffset = 0;

    InitializeSRWLock(&_lockObj);
}

RingBuffer::RingBuffer(int bufferSize)
{
    _bufferSize = bufferSize;
    _buffer = (char*)malloc(_bufferSize);
    _frontOffset = 0;
    _rearOffset = 0;

    InitializeSRWLock(&_lockObj);
}

RingBuffer::~RingBuffer()
{
    free(_buffer);
}

int RingBuffer::GetUseSize() const
{
    int size = this->_rearOffset - this->_frontOffset;
    if (size >= 0)
    {
        return size;
    }
    else
    {
        return this->_bufferSize + size;
    }
}

int RingBuffer::GetFreeSize() const
{
    return this->_bufferSize - GetUseSize() - 1;
}

int RingBuffer::DirectEnqueueSize() const
{
    if (this->_rearOffset >= this->_frontOffset)
    {
        if (this->_frontOffset == 0)
        {
            return this->_bufferSize - this->_rearOffset - 1;       //-1
        }
        else
        {
            return this->_bufferSize - this->_rearOffset;       // -0 
        }
    }
    else
    {
        return this->_frontOffset - this->_rearOffset - 1;
    }
}

int RingBuffer::DIrectDequeueSize() const
{
    if (this->_frontOffset > this->_rearOffset)
    {
        return this->_bufferSize - this->_frontOffset;
    }
    else
    {
        return this->_rearOffset - this->_frontOffset;
    }
}

int RingBuffer::Enqueue(const char* buffer, int size)
{
    /*
    int freeSize = this->GetFreeSize();
    if (size >= freeSize)
    {
        size = freeSize;
    }

    int directSize = this->DirectEnqueueSize();
    if (size < directSize)
    {
        memcpy(this->_buffer + this->_rearOffset, buffer, size);
        this->_rearOffset += size;
    }
    else if (size == directSize)
    {
        memcpy(this->_buffer + this->_rearOffset, buffer, size);
        this->_rearOffset = 0;
    }
    else
    {
        memcpy(this->_buffer + this->_rearOffset, buffer, directSize);
        memcpy(this->_buffer, buffer + directSize, size - directSize);
        this->_rearOffset = size - directSize;
    }*/

    int freeSize = this->GetFreeSize();
    if (size >= freeSize)
    {
        size = freeSize;
    }

    for (unsigned int cur = 0; cur < size; cur++)
    {
        *(this->_buffer + this->_rearOffset) = *(buffer + cur);

        ++this->_rearOffset;
        this->_rearOffset %= this->_bufferSize;
    }

    return size;
}

int RingBuffer::Dequeue(char* outBuffer, int size)
{
    /*
    int readSize = this->GetUseSize();

    if (readSize > size)
    {
        readSize = size;
    }

    int directSize = this->DIrectDequeueSize();
    if (readSize <= directSize)
    {
        memcpy(outBuffer, this->_buffer + this->_frontOffset, readSize);
        this->_frontOffset += readSize;
    }
    else
    {
        memcpy(outBuffer, this->_buffer + this->_frontOffset, directSize);
        memcpy(outBuffer + directSize, this->_buffer, readSize - directSize);
        this->_frontOffset = readSize - directSize;
    }*/

    int readSize = this->GetUseSize();

    if (readSize > size)        //읽을수있는 사이즈 100 > 요청 사이즈 80
    {
        readSize = size;
    }

    for (unsigned int cur = 0; cur < readSize; cur++)
    {
        *(outBuffer + cur) = *(this->_buffer + this->_frontOffset);

        ++this->_frontOffset;
        this->_frontOffset %= this->_bufferSize;
    }
    return readSize;
}

int RingBuffer::Peek(char* outBuffer, int size)
{
    /*
    int readSize = this->GetUseSize();

    if (readSize > size)
    {
        readSize = size;
    }

    int directSize = this->DIrectDequeueSize();
    if (readSize <= directSize)
    {
        memcpy(outBuffer, this->_buffer + this->_frontOffset, readSize);
    }
    else
    {
        memcpy(outBuffer, this->_buffer + this->_frontOffset, directSize);
        memcpy(outBuffer + directSize, this->_buffer, readSize - directSize);
    }
    */
    int readSize = this->GetUseSize();

    if (readSize > size)        //읽을수있는 사이즈 100 > 요청 사이즈 80
    {
        readSize = size;
    }
    unsigned int front = this->_frontOffset;

    for (unsigned int cur = 0; cur < readSize; cur++)
    {
        *(outBuffer + cur) = *(this->_buffer + front);

        ++front;
        front %= this->_bufferSize;
    }
    return readSize;
}

void RingBuffer::MoveRear(int size)
{
    int moveOffset = this->_rearOffset + size;
    if (this->_bufferSize <= moveOffset)
    {
        moveOffset -= this->_bufferSize;
    }

    this->_rearOffset = moveOffset;
}

int RingBuffer::MoveFront(int size)
{
    int moveOffset = this->_frontOffset + size;
    if (this->_bufferSize <= moveOffset)
    {
        moveOffset -= this->_bufferSize;
    }

    this->_frontOffset = moveOffset;

    return 0;
}

void RingBuffer::ClearBuffer()
{
    this->_frontOffset = 0;
    this->_rearOffset = 0;
}

char* RingBuffer::GetFrontBufferPtr() const
{
    return this->_buffer + this->_frontOffset;
}

char* RingBuffer::GetRearBufferPtr() const
{
    return this->_buffer + this->_rearOffset;
}

void RingBuffer::Lock()
{
    AcquireSRWLockExclusive(&_lockObj);
}

void RingBuffer::Unlock()
{
    ReleaseSRWLockExclusive(&_lockObj);
}
