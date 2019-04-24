#pragma once
#include "cl_singleton.h"
#include "cly_config.h"
#include "cl_MessageQueue.h"
#include "cly_downinfo.h"
#include "cly_trackerProto.h"
#include "cly_downAutoInfo.h"

void cly_print_help();

class cly_d
{
public:
	cly_d(void);
	~cly_d(void);

	int init(cly_config_t* pc);
	void fini();
	void handle_root();

	int get_state(clyd_state_t& s);

	int create_download(const string& hash,int ftype,const string& path);
	void refer_file(const string& hash);
	void release_file(const string& hash);
	int check_exist_share_by_path(const string& path); //检查是否已经存在共享
	int share_file(const string& hash,const string& subhash,const string& path,const string& name,bool report);
	int delete_file(const string& hash,bool isDelPhy);
	int get_fileinfo(const string& hash,cly_downloadInfo_t& di);
	int get_readyinfo_by_path(const string& path,cly_readyInfo_t& ri);


	void load_ddlist(const string& path);
	int add_source(const string& hash,list<string>& ls);

	int save_readyfile_utf8(const string& path); //用于tracker校验上报总表
	int update_progress();
	
	int downauto_add(const string& hash,const string& name,int priority, int save_original);
	int downauto_allinfo(DownAutoInfoList& ls); //用于返回下载进度信息
	int downauto_start(const string& hash);
	int downauto_stop(const string& hash);

	int update_keepalive_ack(cly_seg_keepalive_ack_t* seg);
	int update_login_config(cly_seg_config_t *conf);
private:
	static void _ClearMessageFun(cl_Message* msg);
	void update_used(int used);
	
private:
	bool m_binit;
	cl_MessageQueue m_msgQueue;
	int		m_unused_down_times;
};
typedef cl_singleton<cly_d> clyd;

