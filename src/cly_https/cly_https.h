#pragma once
#include "cl_synchro.h"
#include "cl_httpserver.h"

#define CLY_HTTPS_VERSION "cly_https_20180903"

class cly_https
{
public:
	cly_https();
	~cly_https();

public:
	int open(unsigned short port,int threadnum);
	void close(); 
	static int on_request(cl_HttpRequest_t* req);
	
	void index(cl_HttpRequest_t* req);
	void show_httpconf(cl_HttpRequest_t* req);
	void read_rdbfile(cl_HttpRequest_t* req);
	void get_readnum(cl_HttpRequest_t* req); //返回读数据线程数,非XML
private:
	cl_httpserver		m_svr;
	cl_httpserver		m_svrbak;
	cl_SimpleMutex		m_mt;
	unsigned int		m_readnum; //统计读数据连接数
};

