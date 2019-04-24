#include "cl_memblock.h"
#include <assert.h>
#include "cl_sorter.h"

cl_memblock* cl_memblock::allot(int size)
{
	cl_memblock*b = cl_memblockPoolSngl::instance()->get_block(size);
	if(b) b->refer();
	return b;
}
void cl_memblock::release()
{
	assert(ref>0);
	if(--ref==0)
		cl_memblockPoolSngl::instance()->put_block(this);
}

//**************************** [cl_memblockQueue] ****************************
cl_memblockQueue::cl_memblockQueue(int i,int blocksize)
:m_i(i)
,m_blocksize(blocksize)
,m_outs(0)
,m_maxouts(0)
{
}

cl_memblockQueue::~cl_memblockQueue(void)
{
	assert(0==m_outs);
	for(BlockIter it=m_ls.begin();it!=m_ls.end();++it)
		delete *it;
	m_ls.clear();
}

void cl_memblockQueue::put(cl_memblock* b)
{
	assert(b->i == m_i);
	b->rpos = b->wpos = 0;
	b->buflen = m_blocksize;
	m_ls.push_back(b);
	m_outs--;
}
cl_memblock* cl_memblockQueue::get()
{
	cl_memblock* b=NULL;
	if(!m_ls.empty())
	{
		b = m_ls.front();
		m_ls.pop_front();
	}
	else
	{
		b = new cl_memblock(m_i,m_blocksize);
	}
	m_outs++;
	if(m_outs>m_maxouts) m_maxouts = m_outs;
	return b;
}

void cl_memblockQueue::reduce() //裁减,周期减
{
	assert(m_maxouts>=m_outs);
	cl_memblock *b = NULL;
	unsigned int n = m_maxouts - m_outs + 5; //总共块数最多保持最近最大需求量+5;
	int i=0; //
	while(m_ls.size()>n)
	{
		b = m_ls.front();
		m_ls.pop_front();
		delete b;
		//一次释放10块
		if(++i>=10)
			break;
	}
	m_maxouts = m_outs; //m_maxouts 最大需求量重置

}


//**************************** [cl_memblockPool] ****************************

cl_memblockPool::cl_memblockPool()
:m_queue(NULL)
,m_queue_num(0)
,m_outs(0)
{

}
cl_memblockPool::~cl_memblockPool(void)
{
	assert(NULL==m_queue);
}
int cl_memblockPool::init(int blocksize[],int n)
{
	m_queue_num = n;
	//按升序排列.
	cl_sort_bubble<int>(blocksize,n);
	if(m_queue_num)
	{
		m_queue = new cl_memblockQueue*[m_queue_num];
		for(int i=0;i<m_queue_num;++i)
			m_queue[i] = new cl_memblockQueue(i,blocksize[i]);
		
		cl_timerSngl::instance()->register_timer(this,1,20000);
	}
	return 0;
}
void cl_memblockPool::fini()
{
	assert(m_outs==0);
	if(m_queue)
	{
		cl_timerSngl::instance()->unregister_all(this);
		for(int i=0;i<m_queue_num;++i)
			delete m_queue[i];
		delete[] m_queue;
		m_queue = NULL;
		m_queue_num = 0;
	}
}
cl_memblock* cl_memblockPool::get_block(int size)
{
	Lock l(m_mt);
	cl_memblock *b = NULL;
	if(size<=0)
		return NULL;
	for(int i=0;i<m_queue_num;++i)
	{
		if(size <= m_queue[i]->get_blocksize())
		{
			//在此仅提示用户预分配的定长内存块尺寸不适合
			assert(m_queue[i]->get_blocksize()<size+2048);
			b = m_queue[i]->get();
			break;
		}
	}
	if(NULL==b)
	{
		//在此加assert()仅提示用户预计的定长内存块尺寸不适合
		assert(false);
		//-1表示游离的
		b = new cl_memblock(-1,size);
	}
	m_outs++;
	return b;
}
void cl_memblockPool::put_block(cl_memblock* b)
{
	Lock l(m_mt);
	assert(b);
	if(!b)
		return;
	if(-1==b->i)
		delete b;
	else
		m_queue[b->i]->put(b);
	m_outs--;
}
void cl_memblockPool::on_timer(int e)
{
	Lock l(m_mt);
	for(int i=0;i<m_queue_num;++i)
		m_queue[i]->reduce();
}


