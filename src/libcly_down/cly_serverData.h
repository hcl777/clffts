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
	unsigned int	index;      //���ؿ��
	unsigned int	pos;        //���ص���ʼλ��
	cl_memblock*	block;

	bool			bready; //��¼�Ƿ���readyfile
	int				state; //0=CLY_SVR_BLOCK_UNREAD
	string			path; //�����첽������ΪPATH�ڶ��߳�ʵʱȡ��̫��ȫ
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
	//string						peer_name; //req subtableʱ������
	int							group_id; //����ID�������ٶȿ���
	list<cly_svrBlockInfo_t>	bs; //�����б�
	int							thread_i; //���̺߳�

	string						curr_down_hash; //
	string						curr_ready_hash;
	uint64						fsize;
	cl_ERDBFile64				file; //readyfile ��
	DWORD						last_active_tick;

public:
	static cly_svrPeerData* new_instance(){return new cly_svrPeerData();}
	void del(){while(0!=m_ref) Sleep(0);delete this;} //���߳�ִ��
	void refer() {Lock l(m_mt); m_ref++;} //���߳�ִ��
	void release(){Lock l(m_mt); m_ref--;} //���߳�ִ��

	void add_block(const char* hash,unsigned int index,unsigned int offset,bool bready,const string& path);
	void remove_block(const char* hash,unsigned int index);
	void pop_front_block();
	void update_block(const string& hash,unsigned int index,cl_memblock* block);
private:
	Mutex		m_mt;
	int			m_ref;
};

