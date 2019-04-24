#pragma once

#include "cl_httprsp.h"
#include "cl_thread.h"

class cl_httpserver : public cl_thread
{
public:
	cl_httpserver(void);
	~cl_httpserver(void);
	typedef struct tag_task_info
	{
		int state; //0ø’œ–,1”–»ŒŒÒ
		SOCKET s;
		sockaddr_in addr;
		cl_httprsp* h;
		cl_Semaphore sem;
	}task_info_t;

	

public:
	int open(unsigned short port,const char *ip=0,
		FUN_HANDLE_HTTP_REQ_PTR fun=0,void* fun_param=NULL,
		bool multi_thread=true,int max_client_num=20,
		const char* strver=NULL,bool bsupport_keepalive=true);
	int stop();
	bool is_open() const {return m_brun;}

	virtual int work(int e);

	const cl_httppubconf_t *get_conf() const { return &m_conf; }
	//int get_max_client_num()const { return m_max_client_num; }
	//int get_client_num()const {return m_client_num;}
	//int get_request_amount()const { return m_request_amount; }
	//int get_ip_num()const {return m_client_ips.size();}
private:
	int accpet_root();
	int get_idle();
	
	void add_client_ip(unsigned int nip);
	void del_client_ip(unsigned int nip);
private:
	char			m_ip[64];
	bool			m_brun;

	int				m_sock;
	task_info_t**	m_ti;
	cl_httppubconf_t	m_conf;
};

