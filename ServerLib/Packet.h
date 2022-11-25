#pragma once

#include "MemoryPoolTLS.h"

class Packet
{
private:
	static constexpr int DEFAULT_BUFFER_SIZE = 1400;
	static MemoryPoolTLS<Packet> _pool;

public:
	static Packet* Alloc()
	{
		Packet* packet = _pool.Alloc();
		packet->Clear();
		packet->_ref = 0;
		return packet;
	}

	bool Free()
	{
		return _pool.Free(this);
	}

	void AddRef()
	{
		InterlockedIncrement(&_ref);
	}

	void SubRef()
	{
		if (InterlockedDecrement(&_ref) == 0)
		{
			Free();
		}
	}

private:
	Packet();
	Packet(int bufferSize);
	virtual ~Packet();
	template<typename T> friend class MemoryPoolTLS;

public:
	void Release();
	void Clear();
	inline int GetBufferSize() const { return _bufferSize; }
	inline int GetDataSize() const { return _dataSize; }
	inline int GetRemainSize() const { return _bufferSize - _dataSize; }

	char* GetBufferPtr() const { return _buffer; }

	int MoveWritePos(int size);
	int MoveReadPos(int size);

	int GetData(char* inDest, int iSize);
	int PutData(char* outSrc, int iSrcSize);

	void Resize(int size);

	Packet& operator =(Packet& srcPacket);
	Packet& operator << (unsigned char byValue);
	Packet& operator << (char chValue);
	Packet& operator << (unsigned short wValue);
	Packet& operator << (short shValue);
	Packet& operator << (int iValue);
	Packet& operator << (unsigned int uiValue);
	Packet& operator << (long lValue);
	Packet& operator << (unsigned long ulValue);
	Packet& operator << (float fValue);
	Packet& operator << (double dValue);
	Packet& operator << (__int64 i64Value);
	Packet& operator << (unsigned __int64 ui64Value);

	Packet& operator >> (unsigned char& byValue);
	Packet& operator >> (char& chValue);
	Packet& operator >> (unsigned short& wValue);
	Packet& operator >> (short& shValue);
	Packet& operator >> (int& iValue);
	Packet& operator >> (unsigned int& uiValue);
	Packet& operator >> (long& lValue);
	Packet& operator >> (unsigned long& ulValue);
	Packet& operator >> (float& fValue);
	Packet& operator >> (double& dValue);
	Packet& operator >> (__int64& i64Value);
	Packet& operator >> (unsigned __int64& ui64Value);

protected:
	char* _buffer;
	int _bufferSize;
	int _dataSize;

	int _writePos;
	int _readPos;

	long _ref;
};