#pragma once
#include "cl_basetypes.h"
#include "cl_singleton.h"
#include "cl_mysqlConn.h"
#include "cl_timer.h"
#include "cly_trackerProto.h"
#include "cly_source.h"

class cly_usermgr : public cl_timerHandler
{
public:
	cly_usermgr(void);
	~cly_usermgr(void);
	
	typedef cl_RecursiveMutex Mutex;
	typedef cl_TLock<Mutex> Lock;



public:
	int init();
	void fini();
	virtual void on_timer(int e);

	int login(const cly_login_info_t& inf,cly_seg_config_t& seg_conf);
	int keepalive(cly_seg_keepalive_t& sk, cly_seg_keepalive_ack_t& seg);
	int update_nat(int peer_id,const string& addr);
	int report_fdfile(int peer_id,const string& fields,const string& vals);
	int report_progress(int peer_id,list<string>& ls);
	int search_source(int peer_id,const string& hash,list<string>& ls);
	int update_filename(const string& hash,const string& name);
	int get_fileinfo(const string& hash,clyt_fileinfo_t& fi);
	int report_error(const cly_report_error_t& inf);
	int new_task(const string& task_name,list<string>& ls_hids,list<string>& ls_files,int& task_id);
	int set_task_state(const string& task_name,int state);
	int get_task_info(const string& task_name,string& path);
	int get_ddlist(int peer_id,int update_id,string& path);
	int check_fini_chsum(int peer_id,const cly_seg_fchsum_t& seg);
	int get_finifiles(int peer_id,string& path);
	int put_finifiles(int peer_id,const char* path);
	
	//
	int get_state(clyt_state_t& s);
	int get_peer_state(const string& peer_name,clyt_peer_t& s);
	int get_server(list<string>& ls);
private:
	Mutex m_mt;
	clyt_SourceMap m_src;
	clyt_PeerMap m_peer;
};

typedef cl_singleton<cly_usermgr> cly_usermgrSngl;
