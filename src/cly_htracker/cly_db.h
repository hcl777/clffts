#pragma once

#include "cl_mysqlConn.h"
#include "cl_singleton.h"
#include "cl_timer.h"
#include "cly_trackerProto.h"
#include "cl_synchro.h"
#include "cly_source.h"

typedef struct tag_cly_task_node
{
	int file_id;
	int peer_id;
	int progress;
}cly_task_node_t;
typedef struct tag_cly_ddlist_node
{
	int state;
	int file_id;
	int priority;
	string file_hash;
	string file_name;
}cly_ddlist_node_t;

class cly_dbmgr : public cl_timerHandler
{
public:
	cly_dbmgr(void);
	~cly_dbmgr(void);

public:
	int init();
	void fini();
	virtual void on_timer(int e);

	int db_load();
	//所有接口返回错误码
	int db_login(const cly_login_info_t& inf,cly_seg_config_t& seg);
	int db_update_group_id(int peer_id,int group_id);
	int db_keepalive(cly_seg_keepalive_t& sk, cly_seg_keepalive_ack_t& seg);
	int db_update_nat(int peer_id,const string& addr);
	int db_report_finifile(clyt_peer_t* peer,const cly_seg_fdfile_t& seg,bool bupdate_checksum);
	int db_update_filename(const string& hash,const string& name);
	int db_report_delfile(int peer_id,const cly_seg_fdfile_t& seg);
	int db_delsource(int peer_id,int file_id);
	int db_report_progress(int peer_id,list<string>& ls);
	int db_report_error(const cly_report_error_t& inf);
	int db_new_task(const string& task_name,list<int>& ls_peerids,list<int>& ls_fids,int& task_id);
	int db_set_task_state(const string& task_name,int state);
	int db_get_task_info(const string& task_name,list<cly_task_node_t>& ls);
	int db_get_ddlist(int peer_id,int& update_id,list<cly_ddlist_node_t>& ls);
	int db_check_fini_chsum(int peer_id,const cly_seg_fchsum_t& seg);
	int db_reset_chsum(int peer_id,int ff_num,unsigned long long ff_checksum);
private:
	int get_last_id(int& id);
	int update_ff_checksum(int peer_id,const string& hash,bool bdel);
	int get_file_id(int& file_id,const string& hash);
	int get_peer_task_update_id(int peer_id,int& update_id,list<int>& ls_sync_peers);
	int get_task_id(int& task_id,const string& name); //name
private:
	cl_mysqlConn m_db;
};

typedef cl_singleton<cly_dbmgr> cly_dbmgrSngl;


/*
mysql 类型：
datetime: sql语句可用 now() 函数。
date: 可用 curdate()函数。
*/


