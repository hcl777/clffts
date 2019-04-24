#pragma once
#include "cl_singleton.h"
#include "cl_httpserver.h"
#include "cly_trackerProto.h"
#include "cl_synchro.h"

class cly_httpapi
{
public:
	cly_httpapi(void);
	~cly_httpapi(void);
	
	int open();
	void close();

	static int on_request(cl_HttpRequest_t* req);

private:
	void index(cl_HttpRequest_t* req);

	static void rsp_error(int fd,const cly_seg_error_t& seg);

	//系统状态
	void get_state(cl_HttpRequest_t* req);
	void get_setting(cl_HttpRequest_t* req);
	void set_speed(cl_HttpRequest_t* req);

	//文件
	void share_file(cl_HttpRequest_t* req);
	void delete_file(cl_HttpRequest_t* req);
	void get_fileinfo(cl_HttpRequest_t* req);
	void get_allfileinfo(cl_HttpRequest_t* req);
	void read_rdbfile(cl_HttpRequest_t* req);
	void get_readnum(cl_HttpRequest_t* req); //返回读数据线程数,非XML

	//即时更新
	void update_ddlist(cl_HttpRequest_t* req);
	void update_scan(cl_HttpRequest_t* req);
	void update_progress(cl_HttpRequest_t* req);

	//downauto，删除直接使用delete_file接口
	void downauto_add(cl_HttpRequest_t* req);
	void downauto_start(cl_HttpRequest_t* req);
	void downauto_stop(cl_HttpRequest_t* req);
	void downauto_allinfo(cl_HttpRequest_t* req);

private:
	cl_httpserver		m_svr;
	cl_SimpleMutex		m_mt;
	unsigned int		m_readnum; //统计读数据连接数
};

typedef cl_singleton<cly_httpapi> cly_httpapiSngl;
