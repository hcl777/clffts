#include "cly_serverData.h"


cly_svrPeerData::cly_svrPeerData(void)
	:group_id(0)
	,thread_i(0)
	,fsize(0)
	, last_active_tick(GetTickCount())
	,m_ref(0)
{
}


cly_svrPeerData::~cly_svrPeerData(void)
{
	for(list<cly_svrBlockInfo_t>::iterator it=bs.begin();it!=bs.end();++it)
		if((*it).block)
			(*it).block->release();
}

void cly_svrPeerData::add_block(const char* hash,unsigned int index,unsigned int offset,bool bready,const string& path)
{
	Lock l(m_mt);
	cly_svrBlockInfo_t b;
	b.hash = hash;
	b.index = index;
	b.pos = offset;
	b.bready = bready;
	b.path = path;
	for(list<cly_svrBlockInfo_t>::iterator it=bs.begin();it!=bs.end();++it)
	{
		cly_svrBlockInfo_t& b = *it;
		if(b.hash ==hash && b.index==index)
			return;
	}
	bs.push_back(b);
}
void cly_svrPeerData::remove_block(const char* hash,unsigned int index)
{
	Lock l(m_mt);
	for(list<cly_svrBlockInfo_t>::iterator it=bs.begin();it!=bs.end();)
	{
		cly_svrBlockInfo_t& b = *it;
		if(b.hash ==hash && b.index==index)
		{
			if(b.block)
				b.block->release();
			bs.erase(it++);
		}
		else
			it++;
	}
}
void cly_svrPeerData::pop_front_block()
{
	Lock l(m_mt);
	if(!bs.empty())
	{
		cly_svrBlockInfo_t& bi = bs.front();
		if(bi.block)
			bi.block->release();
		bs.pop_front();
	}
}
void cly_svrPeerData::update_block(const string& hash,unsigned int index,cl_memblock* block)
{
	Lock l(m_mt);
	for(list<cly_svrBlockInfo_t>::iterator it=bs.begin();it!=bs.end();++it)
	{
		cly_svrBlockInfo_t& b = *it;
		if(b.hash ==hash && b.index==index)
		{
			assert(b.state == CLY_SVR_BLOCK_READING);
			b.block = block;
			if(block)
				b.state = CLY_SVR_BLOCK_OK;
			else
				b.state = CLY_SVR_BLOCK_FAIL;
			return;
		}
	}
	if(block) block->release(); //更新不到则直接释放
}



