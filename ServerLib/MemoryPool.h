#pragma once
#include <Windows.h>
#include <new.h>
#include <exception>
#include <stdio.h>

template <class DATA>
class MemoryPool 
{
private:
	struct st_BLOCK_NODE
	{
		st_BLOCK_NODE()
		{
			stpNextBlock = NULL;
		}
		DATA data;
		st_BLOCK_NODE *stpNextBlock;
	};

private:

	void newNode()
	{
		st_BLOCK_NODE* node = new st_BLOCK_NODE;
		TOP t;

		do
		{
			t._top = _freeNode._top;
			t._count = _freeNode._count;
			node->stpNextBlock = _freeNode._top;
		} while (!InterlockedCompareExchange128((LONG64*)&_freeNode, (LONG64)node, _freeNode._count + 1, (LONG64*)&t));
		int allocCnt = InterlockedIncrement((LONG*)&m_iAllocCount);
		m_allocList[allocCnt - 1] = node;
	}

public:
	MemoryPool(int iBlockNum, bool bPlacementNew = false)
	{
		_freeNode._count = 0;
		_freeNode._top = nullptr;
		m_iAllocCount = 0;
		m_iUseCount = 0;
		m_bPlacementNew = bPlacementNew;
		m_allocList = (st_BLOCK_NODE**)malloc(sizeof(st_BLOCK_NODE*) * 1000);

		for (int i = 0; i < iBlockNum; i++)
		{
			newNode();
		}
	}
	virtual	~MemoryPool()
	{
		int idx;
		for (idx = 0; idx < m_iAllocCount; ++idx)
		{
			delete m_allocList[idx];
		}
		free(m_allocList);
	}

	DATA* Alloc(void)
	{		
		DATA* data = nullptr;
		st_BLOCK_NODE* next = nullptr;
		TOP t;
		do
		{
			t._top = _freeNode._top;
			t._count = _freeNode._count;

			if (_freeNode._top == nullptr)
			{
				newNode();
				continue;
			}
			else
			{
				next = _freeNode._top->stpNextBlock;
			}

			data = &_freeNode._top->data;
		} while (!InterlockedCompareExchange128((LONG64*)&_freeNode, (LONG64)next, t._count + 1, (LONG64*)&t));

		if (m_bPlacementNew)
		{
			new (data)DATA;
		}
		InterlockedIncrement((LONG*)&m_iUseCount);
		return data;
	}

	bool	Free(DATA* pData)
	{
		st_BLOCK_NODE* node = (st_BLOCK_NODE*)pData;
		TOP t;

		do
		{
			t._top = _freeNode._top;
			t._count = _freeNode._count;
			node->stpNextBlock = _freeNode._top;
		} while (!InterlockedCompareExchange128((LONG64*)&_freeNode, (LONG64)node, t._count + 1 ,(LONG64*)&t));

		InterlockedDecrement((LONG*)&m_iUseCount);

		return true;
	}

	int		GetAllocCount(void) const { return m_iAllocCount; }

	int		GetUseCount(void) const { return m_iUseCount; }

private:
	alignas(64)		bool m_bPlacementNew;
	alignas(64)		int m_iAllocCount;
	alignas(64)		int m_iUseCount;
	alignas(64)		st_BLOCK_NODE** m_allocList;

	struct alignas(64) TOP
	{
		unsigned long long _count;
		st_BLOCK_NODE* _top;
	};
	TOP _freeNode;

	template< typename T> friend class MemoryPoolTLS;
};