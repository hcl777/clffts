#include "cl_uchannel.h"
#include "cl_util.h"
#include "cl_net.h"

cl_uchannel::cl_uchannel(void)
:cl_speaker<cl_uchannelListener>(1)
{
	m_last_active_tick = GetTickCount();
	reset();
}

cl_uchannel::~cl_uchannel(void)
{
}
void cl_uchannel::reset()
{
	m_state = DISCONNECTED;
	//m_hip = 0; //断开时上层有时候需要查IP
	//m_hport = 0;
	m_is_accept = false;
}
int cl_uchannel::attach(SOCKET s,sockaddr_in& addr)
{
	UAC_sockaddr uac_addr;
	uac_addr.ip = ntohl(addr.sin_addr.s_addr);
	uac_addr.port = ntohs(addr.sin_port);
	uac_addr.nattype = 0;

	m_hip = ntohl(addr.sin_addr.s_addr);
	m_hport = ntohs(addr.sin_port);
	m_is_accept = true;
	return uac_attach((int)s,uac_addr);
}

int cl_uchannel::connect(unsigned int ip,unsigned short port,int nattype/*=0*/)
{
	m_hip = ip;
	m_hport = port;
	m_state = CONNECTING;
	int ret=this->uac_connect(ip,port,nattype);
	if(0!=ret)
		m_state = DISCONNECTED;
	return ret;
}
int cl_uchannel::disconnect()
{
	if(DISCONNECTED!=m_state)
	{
		m_state = DISCONNECTED;
		reset();
		uac_disconnect();
		fire(cl_uchannelListener::Disconnected(),this);
	}
	return 0;
}
int cl_uchannel::send(cl_memblock *b,bool more/*=false*/)  //-1:false; 0:send ok; 1:put int sendlist
{
	m_last_active_tick = GetTickCount();
	assert(b);
	if(CONNECTED != m_state)
	{
		b->release();
		return -1;
	}
	int sendsize = b->length();
	if(sendsize<=0)
	{
		b->release();
		return 0;
	}
	int ret = uac_send(b->read_ptr(),sendsize);
	b->release();
	if(ret!=sendsize)
	{
		disconnect();
		return -1;
	}
	
	return uac_is_write()?0:1; //0继续可以写
}
int cl_uchannel::recv(char *b,int size)
{
	m_last_active_tick = GetTickCount();
	int ret = uac_recv(b,size);
	if(-1==ret)
	{
		disconnect();
		return -1;
	}
	return ret;
}

int cl_uchannel::uac_on_read()
{
	int wait = 0;
	fire(cl_uchannelListener::Readable(),this,wait);
	return wait;
}
void cl_uchannel::uac_on_write()
{
	if(UAC_CONNECTED==m_uac_state)
	{	
		fire(cl_uchannelListener::Writable(),this);
	}
	else
	{
		UAC_Socket::uac_on_write();
	}
}
void cl_uchannel::uac_on_connected()
{
	m_state = CONNECTED;
#ifndef ANDROID
	uac_setsendbuf(500000);
	uac_setrecvbuf(1000000);
#else
	uac_setsendbuf(200000);
	uac_setrecvbuf(600000);
#endif
	UAC_Socket::uac_on_connected();
	fire(cl_uchannelListener::Connected(),this);
}
void cl_uchannel::uac_on_connect_broken()
{
	disconnect();
}

