#ifndef _MemoryMgr_hpp_
#define _MemoryMgr_hpp_
#include <stdlib.h>
#include <assert.h>

#ifdef _DEBUG
#include <stdio.h>
	#define xPrintf(...) printf(__VA_ARGS__)
#else
	#define xPrintf(...) printf(x) 
#endif

#define MAX_MEMORY_SIZE 128
class MemoryAlloc;
//�ڴ�� ��С��Ԫ
class MemoryBlock
{
public:
	//�ڴ����
	int nID;
	//���ô���
	int nRef;
	//�����ڴ棨�飩��
	MemoryAlloc* pAlloc;
	//��һ��λ��
	MemoryBlock* pNext;
	//�Ƿ����ڴ����
	bool bPool;
private:
	//Ԥ��
	char c1;
	char c2;
	char c3;
};
//�ڴ��
class MemoryAlloc
{
public:
	MemoryAlloc()
	{
		_pBuf = nullptr;
		_pHeader = nullptr;
		_nSize = 0;
		_nBlockSize = 0;
	}
	~MemoryAlloc()
	{
		if (_pBuf)
			free(_pBuf);
	}

	//�����ڴ�
	void* allocMemory(size_t nSize)
	{
		if (!_pBuf)
		{
			initMemory();
		}
		MemoryBlock* pReturn = nullptr;
		if (nullptr == _pHeader)
		{
			pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
		}
		else {
			pReturn = _pHeader;
			_pHeader = _pHeader->pNext;
			assert(0 == pReturn->nRef);
			pReturn->nRef = 1;
		}
		xPrintf("allocMem: %x, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);

		return ((char*)pReturn+sizeof(MemoryBlock));
	}
	//�ͷ��ڴ�
	void  freeMemory(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		assert(1 == pBlock->nRef);
		if (--pBlock->nRef  != 0)
		{
			return;
		}
		if (pBlock->bPool)
		{
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else {
			free(pBlock);
			return;
		}
	}
	//��ʼ��
	void initMemory()
	{
		assert(nullptr == _pBuf);
		if (_pBuf)
			return;
		//�����ڴ�ش�С
		size_t realSize = _nSize + sizeof(MemoryBlock);
		size_t bufSize = realSize * _nBlockSize;
		//��ϵͳ����ص��ڴ�
		_pBuf = (char*)malloc(bufSize);
		//��ʼ���ڴ��
		_pHeader = (MemoryBlock*)_pBuf;
		_pHeader->bPool = true;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->pAlloc = this;
		_pHeader->pNext = nullptr;
		//�����ڴ���ʼ��
		MemoryBlock* pTemp1 = _pHeader;
		for (size_t n = 1; n < _nBlockSize; n++)
		{
			MemoryBlock* pTemp2 = (MemoryBlock*)(_pBuf + n*realSize);
			pTemp2->bPool = true;
			pTemp2->nID = n;
			pTemp2->nRef = 0;
			pTemp2->pAlloc = this;
			pTemp2->pNext = nullptr;
			pTemp1->pNext = pTemp2;
			pTemp1 = pTemp2;
		}
	}
protected:
	//�ڴ�ص�ַ
	char* _pBuf;
	//ͷ���ڴ浥Ԫ
	MemoryBlock* _pHeader;
	//�ڴ浥Ԫ��С
	size_t _nSize;
	//�ڴ浥Ԫ����
	size_t _nBlockSize;
};
//������������Ա����ʱ�����ʼ��
template<size_t nSize,size_t nBlockSize>
class MemoryAlloctor : public MemoryAlloc
{
public:
	MemoryAlloctor()
	{
		const size_t n = sizeof(void*);

		_nSize = (nSize/n)*n+(nSize%n ? n : 0);
		_nBlockSize = nBlockSize;
	}
};

//�ڴ��������
class MemoryMgr
{
private:
	MemoryMgr()
	{
		init_szAlloc(0, 64, &_mem64);
		init_szAlloc(65, 128, &_mem128);
		//init_szAlloc(129, 256, &_mem256);
		//init_szAlloc(257, 512, &_mem512);
		//init_szAlloc(513, 1024, &_mem1024);
	}
	~MemoryMgr()
	{

	}

public:
	static MemoryMgr& Instance()
	{
		//����ģʽ
		static MemoryMgr mgr;
		return mgr;
	}
	//�����ڴ�
	void* allocMem(size_t nSize)
	{
		if (nSize <= MAX_MEMORY_SIZE)
		{
			return  _szAlloc[nSize]->allocMemory(nSize);
		}
		else {
			MemoryBlock* pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
			xPrintf("allocMem: %x, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
			return ((char*)pReturn + sizeof(MemoryBlock));;
		}
	}
	//�ͷ��ڴ�
	void  freeMem(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		xPrintf("freeMem: %x, id=%d\n", pBlock, pBlock->nID);

		if (pBlock->bPool)
		{
			pBlock->pAlloc->freeMemory(pMem);
		}
		else {
			if (--pBlock->nRef == 0)
				free(pBlock);
		}
	}
	//�����ڴ����
	void addRef(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		++pBlock->nRef;
	}
private:
	//��ʼ���ڴ��ӳ������
	void init_szAlloc(int nBegin, int nEnd, MemoryAlloc* pMemA)
	{
		for (int n = nBegin; n <= nEnd; n++)
		{
			_szAlloc[n] = pMemA;
		}
	}
private:
	MemoryAlloctor<64,1000000> _mem64;
	MemoryAlloctor<128, 1000000> _mem128;
	//MemoryAlloctor<256, 100000> _mem256;
	//MemoryAlloctor<512, 100000> _mem512;
	//<1024, 100000> _mem1024;
	MemoryAlloc* _szAlloc[MAX_MEMORY_SIZE + 1];
};

#endif // !_MemoryMgr_hpp_