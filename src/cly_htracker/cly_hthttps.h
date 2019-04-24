#pragma once
#include "cl_singleton.h"
#include "cl_httpserver.h"
#include "cly_trackerProto.h"


class cly_hthttps
{
public:
	cly_hthttps(void);
	~cly_hthttps(void);
	
	int open();
	void close();

	static int on_request(cl_HttpRequest_t* req);

private:
	void index(cl_HttpRequest_t* req);
	void show_httpconf(cl_HttpRequest_t* req);
	static void rsp_error(int fd,const cly_seg_error_t& seg);
	//
	void login(cl_HttpRequest_t* req);
	void keepalive(cl_HttpRequest_t* req);
	void update_nat(cl_HttpRequest_t* req);
	void search_source(cl_HttpRequest_t* req);
	void search_sources(cl_HttpRequest_t* req); //临时兼容旧版本
	void report_progress(cl_HttpRequest_t* req);
	void report_fdfile(cl_HttpRequest_t* req);
	void update_filename(cl_HttpRequest_t* req);
	void get_fileinfo(cl_HttpRequest_t* req);
	void report_error(cl_HttpRequest_t* req);

	//task
	void new_task(cl_HttpRequest_t* req);
	void set_task_state(cl_HttpRequest_t* req);
	void get_task_info(cl_HttpRequest_t* req);
	void get_ddlist(cl_HttpRequest_t* req);

	//checksum
	void check_fini_chsum(cl_HttpRequest_t* req);
	void get_finifiles(cl_HttpRequest_t* req);
	void put_finifiles(cl_HttpRequest_t* req);

	//内部查看接口
	static void get_state(cl_HttpRequest_t* req);
	static void get_peer_state(cl_HttpRequest_t* req);
	static void get_server(cl_HttpRequest_t* req);
private:
	cl_httpserver m_svr;
	cl_httpserver m_svrbak;
};

typedef cl_singleton<cly_hthttps> cly_hthttpsSngl;

