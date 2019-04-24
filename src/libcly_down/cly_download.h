#pragma once
#include "cly_peer.h"
#include "cly_fileinfo.h"
#include "cl_speedometer.h"
#include "cly_downinfo.h"
#include "cl_httpcc.h"

class cly_download : 
	public cly_peerListener
	,public cl_timerHandler
	,public cl_httpccListener
{
	typedef struct tag_blockInfo
	{
		unsigned int index;
		cl_memblock* block;
	}blockInfo_t;
	typedef struct tag_peerData
	{
		uint64				source_id;
		list<blockInfo_t>	bis;
		int					err;
		cl_bittable			bt;
		bool				is_bt_fini;
		unsigned int		last_req_subtable_tick;
		int					user_type;
		cl_speedometer<uint64,20> speed;

		tag_peerData(void)
			:source_id(0)
			,err(0)
			,is_bt_fini(false)
			,last_req_subtable_tick(0)
			,user_type(0)
		{}
		void remove_bi(unsigned int index)
		{
			for(list<blockInfo_t>::iterator it=bis.begin();it!=bis.end();++it)
			{
				if((*it).index == index)
				{
					bis.erase(it);
					return;
				}
			}
		}
	}peerData_t;

	typedef map<int,list<cly_peer*> > BlockRefMap;
	typedef BlockRefMap::iterator BlockRefIter;
public:
	cly_download(void);
	~cly_download(void);

public:
	int create(const string& hash,int ftype,const string& path, bool bsave_original);
	void close();
	int add_source_list(list<string>& ls);
	int add_source(const string& s,uint32 ip,uint16 port,int ntype,int user_type);
	bool is_finished() const {return 2==m_finished_state;}
	int get_downloadinfo(cly_downloadInfo_t& inf);
	int get_conn_num()const { return m_sources.size(); }
public:
	virtual void on(cly_peerListener::Connected,cly_peer* peer);
	virtual void on(cly_peerListener::Disconnected,cly_peer* peer);
	virtual void on(cly_peerListener::Data,cly_peer* peer,char* buf,int len);
	virtual void on(cly_peerListener::Writable,cly_peer* peer){}

	virtual void on_timer(int e);
private:
	int create_connect(cly_source_t* s);
	void clear_pending();

	void on_rsp_subtable(cly_peer* peer,cl_ptlstream& ps);
	void on_cancel_blocks(cly_peer* peer,cl_ptlstream& ps);
	void on_rsp_block_data(cly_peer* peer,cl_ptlstream& ps);

	int assign_job(cly_peer* peer);
	void cancel_job(unsigned int index);

	int add_block_ref(int index,cly_peer* peer);
	int del_block_ref(int index,cly_peer* peer);
	int get_block_ref_num(int index);
	int get_block_ref_speed(int index,int seconds);
private:
	cly_fileinfo*				m_fi;
	list<cly_peer*>				m_peers,m_pending;
	map<uint64,cly_source_t*>	m_sources;
	list<cly_source_t*>			m_sources_free;
	BlockRefMap					m_blockRefs;
	cl_speedometer<uint64>		m_speed;
	cly_downloadInfo_t			m_downInfo;
	DWORD						m_tick;
	unsigned int				m_cnn_create_speed;
	int							m_finished_state;//0=下载中，1=等待校验，2=完成

	//http
	cl_httpcc*					m_hpeer;
};

