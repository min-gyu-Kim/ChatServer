#pragma once

#include "MemoryPool.h"

template<class DATA>
class MemoryPoolTLS
{
private:
	struct st_DATA_BLOCK
	{
		DATA	data;
		void* ptrChunk;
		bool isFree;
	};
	class Chunk
	{
	public:
		enum eStatus
		{
			BLOCK_SIZE = 100
		};

	public:
		Chunk*				_ptrChunk;
		st_DATA_BLOCK		_blocks[BLOCK_SIZE];

		long				_allocCount;
		bool				_isAlloc;
		alignas(64)long		_freeCount;

	public:
		Chunk()			
		{	
			Init();

			for (int idx = 0; idx < BLOCK_SIZE; ++idx)
			{
				_blocks[idx].ptrChunk = (void*)this;
				_blocks[idx].isFree = true;
			}
			_ptrChunk = this;
		}

		~Chunk()
		{
			for (int idx = 0; idx < BLOCK_SIZE; ++idx)
			{
				if (_blocks[idx].isFree == false)
				{
					//LOG 누수
					int* ptr = nullptr;
					*ptr = 0;
				}
			}
		}

		void Init()
		{
			_allocCount = BLOCK_SIZE;
			_freeCount = 0;
		}
	};

private:
	MemoryPool<Chunk>				_chunkPool;
	//읽기
	alignas(64) unsigned int		_dwTlsIndex;
	bool							_isPlacementNew;

public:
	int GetChunkCount() const { return _chunkPool.GetAllocCount(); }

	MemoryPoolTLS(bool PlacementNew = false) : 
		_chunkPool(0),
		_isPlacementNew(PlacementNew)
	{
		_dwTlsIndex = TlsAlloc();
		if (_dwTlsIndex == TLS_OUT_OF_INDEXES)
		{
			//TODO: 로깅 
			printf("TLS error! %d\n", GetLastError());
			int* ptrBrk = nullptr;
			*ptrBrk = 0;
		}
	}

	virtual ~MemoryPoolTLS()
	{
		for (int idx = 0; idx < _chunkPool.m_iAllocCount; ++idx)
		{
			Chunk* pChunk = (Chunk*)_chunkPool.m_allocList[idx];
			if (pChunk->_isAlloc == true)
			{
				if (pChunk->_freeCount + pChunk->_allocCount != Chunk::BLOCK_SIZE)
				{
					//LOG 누수
					int* ptr = nullptr;
					*ptr = 0;
				}
			}
		}

		TlsFree(_dwTlsIndex);
	}

	DATA* Alloc()
	{
		Chunk* ptrChunk = (Chunk*)TlsGetValue(_dwTlsIndex);
		if (ptrChunk == nullptr)
		{
			ptrChunk = AllocChunk();
			ptrChunk->_isAlloc = true;
		}

		long allocCount;
		if ((allocCount = --ptrChunk->_allocCount) == 0)
		{
			AllocChunk();
		}

		st_DATA_BLOCK* block = &ptrChunk->_blocks[allocCount];
		if (_isPlacementNew)
		{
			new(&block->data) DATA;
		}

		block->isFree = false;
		return &block->data;
	}

	bool Free(DATA* ptr)
	{
		st_DATA_BLOCK* block = (st_DATA_BLOCK*)ptr;
		Chunk* ptrChunk = (Chunk*)block->ptrChunk;
		if (ptrChunk != ptrChunk->_ptrChunk)
		{
			return false;
		}

		if (block->isFree == true)
		{
			//이미 해제됨
			return false;
		}

		block->isFree = true;

		if (InterlockedIncrement(&ptrChunk->_freeCount) == Chunk::BLOCK_SIZE)
		{
			_chunkPool.Free(ptrChunk);
			ptrChunk->_isAlloc = false;
		}

		return true;
	}

private:
	Chunk* AllocChunk()
	{
		Chunk* ptrChunk = _chunkPool.Alloc();
		if (TlsSetValue(_dwTlsIndex, ptrChunk) == 0)
		{
			//TODO: LOG GetLastError
			int* ptrBrk = nullptr;
			*ptrBrk = 0;
		}

		//new(ptrChunk) Chunk;
		ptrChunk->Init();		

		return ptrChunk;
	}
};