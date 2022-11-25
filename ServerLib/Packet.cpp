#include "pch.h"
#include "Packet.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <exception>

MemoryPoolTLS<Packet> Packet::_pool;

Packet::Packet()
{
    _buffer = (char*)malloc(DEFAULT_BUFFER_SIZE);
    _bufferSize = DEFAULT_BUFFER_SIZE;
    _readPos = 0;
    _writePos = 0;
    _dataSize = 0;
    _ref = 0;
}

Packet::Packet(int bufferSize)
{
    _buffer = (char*)malloc(bufferSize);
    _bufferSize = bufferSize;
    _readPos = 0;
    _writePos = 0;
    _dataSize = 0;
    _ref = 0;
}

Packet::~Packet()
{
    Release();
}

void Packet::Release()
{
    free(_buffer);
}

void Packet::Clear()
{
    _readPos = 0;
    _writePos = 0;
    _dataSize = 0;
}

int Packet::MoveWritePos(int size)
{
    int moveSize;
    if (_writePos + size > _bufferSize)
    {
        moveSize = _bufferSize - _writePos - 1;
    }
    else
    {
        moveSize = size;
    }

    _writePos += moveSize;
    _dataSize = _writePos - _readPos;

    return moveSize;
}

int Packet::MoveReadPos(int size)
{
    int moveSize;
    if (_readPos + size >= _bufferSize)
    {
        moveSize = _bufferSize - _readPos - 1;
    }
    else
    {
        moveSize = size;
    }

    _readPos += moveSize;
    _dataSize = _writePos - _readPos;

    return moveSize;
}

int Packet::GetData(char* inDest, int iSize)
{
    if (_dataSize < iSize)
    {
        iSize = _dataSize;
        throw std::exception("Packet error! : GetData");
    }
    memcpy(inDest, _buffer + _readPos, iSize);

    return MoveReadPos(iSize);
}

int Packet::PutData(char* outSrc, int iSrcSize)
{
    if (GetRemainSize() < iSrcSize)
    {
        Resize(_bufferSize * 2);
        printf("Resize()\n");
    }

    memcpy(_buffer + _writePos, outSrc, iSrcSize);

    return MoveWritePos(iSrcSize);
}

void Packet::Resize(int size)
{
    char* tmpBuffer = (char*)malloc(size);
    memcpy(tmpBuffer, _buffer, _bufferSize);

    Release();
    printf("buffer resize: %d -> %d\n", _bufferSize, size);
    _bufferSize = size;
    _buffer = tmpBuffer;
}

Packet& Packet::operator=(Packet& srcPacket)
{
    Resize(srcPacket._bufferSize);
    _readPos = srcPacket._readPos;
    _writePos = srcPacket._writePos;
    _dataSize = srcPacket._dataSize;
    return *this;
}

Packet& Packet::operator<<(unsigned char byValue)
{
    PutData((char*)&byValue, sizeof(unsigned char));
    return *this;
}

Packet& Packet::operator<<(char chValue)
{
    PutData((char*)&chValue, sizeof(char));
    return *this;
}

Packet& Packet::operator<<(unsigned short wValue)
{
    PutData((char*)&wValue, sizeof(unsigned short));
    return *this;
}

Packet& Packet::operator<<(short shValue)
{
    PutData((char*)&shValue, sizeof(short));
    return *this;
}

Packet& Packet::operator<<(int iValue)
{
    PutData((char*)&iValue, sizeof(int));
    return *this;
}

Packet& Packet::operator<<(unsigned int uiValue)
{
    PutData((char*)&uiValue, sizeof(unsigned int));
    return *this;
}

Packet& Packet::operator<<(long lValue)
{
    PutData((char*)&lValue, sizeof(long));
    return *this;
}

Packet& Packet::operator<<(unsigned long ulValue)
{
    PutData((char*)&ulValue, sizeof(unsigned long));
    return *this;
}

Packet& Packet::operator<<(float fValue)
{
    PutData((char*)&fValue, sizeof(float));
    return *this;
}

Packet& Packet::operator<<(double dValue)
{
    PutData((char*)&dValue, sizeof(double));
    return *this;
}

Packet& Packet::operator<<(__int64 i64Value)
{
    PutData((char*)&i64Value, sizeof(__int64));
    return *this;
}

Packet& Packet::operator<<(unsigned __int64 ui64Value)
{
    PutData((char*)&ui64Value, sizeof(unsigned __int64));
    return *this;
}

Packet& Packet::operator>>(unsigned char& byValue)
{
    GetData((char*)&byValue, sizeof(unsigned char));
    return *this;
}

Packet& Packet::operator>>(char& chValue)
{
    GetData((char*)&chValue, sizeof(char));
    return *this;
}

Packet& Packet::operator>>(unsigned short& wValue)
{
    GetData((char*)&wValue, sizeof(unsigned short));
    return *this;
}

Packet& Packet::operator>>(short& shValue)
{
    GetData((char*)&shValue, sizeof(short));
    return *this;
}

Packet& Packet::operator>>(int& iValue)
{
    GetData((char*)&iValue, sizeof(int));
    return *this;
}

Packet& Packet::operator>>(unsigned int& uiValue)
{
    GetData((char*)&uiValue, sizeof(unsigned int));
    return *this;
}

Packet& Packet::operator>>(long& lValue)
{
    GetData((char*)&lValue, sizeof(long));
    return *this;
}

Packet& Packet::operator>>(unsigned long& ulValue)
{
    GetData((char*)&ulValue, sizeof(unsigned long));
    return *this;
}

Packet& Packet::operator>>(float& fValue)
{
    GetData((char*)&fValue, sizeof(float));
    return *this;
}

Packet& Packet::operator>>(double& dValue)
{
    GetData((char*)&dValue, sizeof(double));
    return *this;
}

Packet& Packet::operator>>(__int64& i64Value)
{
    GetData((char*)&i64Value, sizeof(__int64));
    return *this;
}

Packet& Packet::operator>>(unsigned __int64& ui64Value)
{
    GetData((char*)&ui64Value, sizeof(unsigned __int64));
    return *this;
}
