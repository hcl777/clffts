#pragma once
#include "cl_basetypes.h"
#include "cl_memblock.h"
#include "cly_fileinfo.h"


enum {
	CLY_SVR_BLOCK_INIT=0,
	CLY_SVR_BLOCK_READING,
	CLY_SVR_BLOCK_OK,
	CLY_SVR_BLOCK_FAIL
};
typedef struct tag_cly_svrBlockInfo
{
	string			hash;
	unsigned int	index;      //下载块号
	unsigned int	pos;        //下载的起始位置
	cl_memblock*	block;

	bool			bready; //记录是否是readyfile
	int				state; //0=CLY_SVR_BLOCK_UNREAD
	string			path; //用于异步读，因为PATH在读线程实时取不太安全
	tag_cly_svrBlockInfo(void){reset();}
	void reset() { hash="";index=0;pos=0;block=NULL;bready=false,state=CLY_SVR_BLOCK_INIT;}
}cly_svrBlockInfo_t;

class cly_readyfile
{
public:
	cly_readyfile(void);
	~cly_readyfile(void);

private:
	uint64			m_size;
};

class cly_svrPeerData
{
	typedef cl_SimpleMutex Mutex;
	typedef cl_TLock<Mutex> Lock;
private:
	cly_svrPeerData(void);
	~cly_svrPeerData(void);
public:
	//string						peer_name; //req subtable时传过来
	int							group_id; //分类ID，用于速度控制
	list<cly_svrBlockInfo_t>	bs; //任务列表
	int							thread_i; //读线程号

	string						curr_down_hash; //
	string						curr_ready_hash;
	uint64						fsize;
	cl_ERDBFile64				file; //readyfile 用
	DWORD						last_active_tick;

public:
	static cly_svrPeerData* new_instance(){return new cly_svrPeerData();}
	void del(){while(0!=m_ref) Sleep(0);delete this;} //主线程执行
	void refer() {Lock l(m_mt); m_ref++;} //主线程执行
	void release(){Lock l(m_mt); m_ref--;} //读线程执行

	void add_block(const char* hash,unsigned int index,unsigned int offset,bool bready,const string& path);
	void remove_block(const char* hash,unsigned int index);
	void pop_front_block();
	void update_block(const string& hash,unsigned int index,cl_memblock* block);
private:
	Mutex		m_mt;
	int			m_ref;
};

