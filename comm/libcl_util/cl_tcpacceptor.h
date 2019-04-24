#pragma once

#include "cl_reactor.h"

class cl_tcpacceptorHandle
{
public:
	virtual ~cl_tcpacceptorHandle(void){}
public:
	virtual bool attach_tcp_socket(SOCKET fd,sockaddr_in& addr)=0;
};

class cl_tcpacceptor : public cl_rthandle
{
public:
	cl_tcpacceptor(void);
	virtual ~cl_tcpacceptor(void);
	
	int open(unsigned short port,const char* ip,cl_tcpacceptorHandle* chf,cl_reactor* reactor);
	void close();
	virtual int sock(){return m_fd;}
	virtual int handle_input();
	virtual int handle_output(){return 0;}
	virtual int handle_error(){return 0;}

private:
	cl_tcpacceptorHandle *m_handle;
	SOCKET m_fd;
	cl_reactor *m_reactor;
	GETSET(unsigned int, m_hip,_hip)
	GETSET(unsigned short, m_hport,_hport)
};

