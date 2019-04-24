#pragma once
#include "cl_cyclist.h"
#include "cl_timer.h"
#include "cl_singleton.h"
#include "cl_synchro.h"
/*************************************
说明:
本内存池实现n组固定大小内存块的预分配管理.
每组的块大小相同.不同组块大小不相同.
本内存池操作不加锁.只适合单线程使用.
一个cl_memblockQueue为一组相同大小的cl_memblock..
一个cl_memblockPool管理多个cl_memblockQueue.
cl_memblockQueue中的内存块会周期进行删减,每20秒执行一次.
**************************************/

#define CL_MBLOCK_NEW_RETURN_INT(name,size,ret) cl_memblock *name = cl_memblock::allot(size); if(NULL==name) {return ret;} 

class cl_memblock
{
	friend class cl_memblockQueue;
	friend class cl_memblockPool;
public:
	char *buf;
	//int buflen,datapos,datalen;
	int buflen,rpos,wpos;
private:
	int i;
	int ref;
public:
	cl_memblock(int i,int size)
		:buf(new char[size])
		,buflen(size)
		,rpos(0)
		,wpos(0)
		,i(i)
		,ref(0)//初始0
	{
	}
	~cl_memblock(void)
	{
		delete[] buf;
	}

public:
	static cl_memblock* allot(int size);
	void refer(){ref++;}
	void release();
	void clear() { wpos = rpos = 0; }
	int length() const { return wpos-rpos; }
	char* read_ptr() const { return buf+rpos; }
	char* write_ptr() const { return buf + wpos; }
	int write(const char* b,int len,int offset)
	{
		if(offset>wpos)
			return 0;
		int tmp = wpos - offset; //去掉重叠部分
		if(len<=tmp)
			return 0;
		len -= tmp;
		b += tmp;
		assert(wpos+len<=buflen);
		if(wpos+len>buflen) len = buflen-wpos;
		memcpy(buf+wpos,b,len);
		wpos += len;
		return len;
	}
};

class cl_memblockQueue
{
public:
	cl_memblockQueue(int i,int blocksize);
	~cl_memblockQueue(void);

	typedef list<cl_memblock*> BlockList;
	typedef BlockList::iterator BlockIter;
public:
	void put(cl_memblock* b);
	cl_memblock* get();

	void reduce(); //裁减,周期减
	int get_blocksize() const { return m_blocksize;}
private:
	BlockList m_ls;
	int m_i;
	int m_blocksize;	//block 大小
	int m_outs;			//已经调出去的.
	int m_maxouts;		//最近周期内最大需求量
};

class cl_memblockPool : public cl_timerHandler
{
	typedef cl_SimpleMutex Mutex;
	typedef cl_TLock<Mutex> Lock;
public:
	cl_memblockPool();
	~cl_memblockPool(void);
public:
	int init(int blocksize[],int n);
	void fini();

	cl_memblock* get_block(int size);
	void put_block(cl_memblock* b);

	virtual void on_timer(int e);
private:
	Mutex	m_mt;
	cl_memblockQueue **m_queue;
	int m_queue_num;
	int m_outs;
};

typedef cl_singleton<cl_memblockPool> cl_memblockPoolSngl;


