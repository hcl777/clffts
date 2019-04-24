#pragma once
#include "cl_basetypes.h"
#include "cl_ntypes.h"
#include "cl_synchro.h"
#include "cl_singleton.h"
#include "cly_trackerProto.h"

/*
注：
1。由于需要支持peer_type动态变动，所以source表不再区多链表，否则处理变更类型复杂。
   单独另外保存一个服务器类型表。
*/

typedef struct tag_clyt_file
{
	int		file_id;
	string	hash;
	string	name;
	string  subhash;
}clyt_file_t;


typedef struct tag_clyt_peer
{
	bool	is_login;
	int		keepalive_count;
	int		peer_id;
	int		group_id;
	string	peer_name;
	int		peer_type;
	int		nattype;
	string	addr;
	DWORD	last_tick;
	string	last_time;
	map<int,clyt_file_t*> fini_files;
	int		fini_num;
	unsigned long long fini_chsum;

	bool add_fini_file(clyt_file_t* file)
	{
		map<int,clyt_file_t*>::iterator it = fini_files.find(file->file_id);
		if(it!=fini_files.end())
			return false;
		fini_files[file->file_id] = file;
		return true;
	}
}clyt_peer_t;


typedef struct tag_clyt_progress
{
	int					progress;
	clyt_peer_t*		peer;
}clyt_progress_t;
typedef struct tag_clyt_source
{
	clyt_file_t *file;
	list<clyt_progress_t> ls_peer;
	int last_i_peer;
}clyt_source_t;

typedef struct tag_clyt_state
{
	string			begin_time;
	int				file_num;
	int				peer_num;
	int				svr_num;
	list<string>	peers;
}clyt_state_t;


typedef map<int,clyt_file_t*> clyt_FileMap;
typedef clyt_FileMap::iterator clyt_FileIter;

typedef map<int,clyt_peer_t*> clyt_PeerMap;
typedef clyt_PeerMap::iterator clyt_PeerIter;
typedef map<string,clyt_peer_t*> clyt_PeerHidMap;
typedef clyt_PeerHidMap::iterator clyt_PeerHidIter;

typedef map<string,clyt_source_t*> clyt_SourceMap;
typedef clyt_SourceMap::iterator clyt_SourceIter;

class cly_source
{
public:
	cly_source(void);
	~cly_source(void);
	typedef cl_RecursiveMutex Mutex;
	typedef cl_TLock<Mutex> Lock;
public:
	void clear();
	clyt_file_t* get_file(int file_id);
	clyt_peer_t* get_peer(int peer_id);
	clyt_peer_t* get_peer(const string& peer_name);
	clyt_source_t* get_source(int file_id);
	clyt_source_t* get_source(const string& hash);
	clyt_progress_t* get_pg(list<clyt_progress_t>& ls,int peer_id);
	void del_pg(list<clyt_progress_t>& ls,int peer_id,clyt_file_t* file);
	int get_all_peerid(list<int>& ls);
	int get_peerids_by_hids(list<string>& ls_hids,list<int>& ls_peerid);
	int get_fileids_by_hashs(list<string>& ls_hash,list<int>& ls_fids);

	int src_login(int peer_id,const string& peer_name,int peer_type,int group_id);
	int src_update_group_id(int peer_id,int group_id);
	int src_keepalive(int& peer_id,const string& peer_name);
	void src_update_nat(int peer_id,const string& addr,int nattype);
	int src_finifile(clyt_peer_t* peer,int file_id,const string& hash,const string& suhash,const string& name);
	int src_update_filename(const string& hash,const string& name);
	int src_get_fileinfo(const string& hash,clyt_fileinfo_t& fi);
	int src_delfile(int peer_id,int file_id);
	int src_delfile(clyt_peer_t* peer,const string& hash,int& file_id); //同时返回file_id,提高效率
	int src_update_progress(int peer_id,const string& hash,int progress); 
	int src_search_source(int peer_id,const string& hash,list<string>& ls);
	int src_save_peer_finifiles(int peer_id,const string& path);

	
	//
	int src_get_state(clyt_state_t& st);
	int src_get_peer_state(const string& peer_name,clyt_peer_t& p);
	int src_get_server(list<string>& ls);
private:
	static void get_source_from_svr(int file_id,list<string>& srcls,clyt_peer_t* peer,list<clyt_peer_t*>& ls,int& index,int maxnum);
	static void get_source_from_client(list<string>& srcls,clyt_peer_t* peer,list<clyt_progress_t>& ls,int& index,int maxnum);
	void change_peer_type(clyt_peer_t* peer,int type);
public:
	clyt_FileMap		m_files;
	clyt_PeerMap		m_peers;
	clyt_PeerHidMap		m_hidpeers; //同m_peer,只是索引不同
	list<clyt_peer_t*>	m_svr_peers; //只保存服务器类型表，用于搜索
	int					last_i_svr;
	clyt_SourceMap		m_srcs;
};
typedef cl_singleton<cly_source> cly_sourceSngl;
