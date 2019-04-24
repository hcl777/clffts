#pragma once
#include "cl_reactor.h"
#include "cl_memblock.h"
#include "cl_speaker.h"


class cl_tcpchannel;
class cl_tcpchannelListener
{
public:
	virtual ~cl_tcpchannelListener(void){}

	template<int I>
	struct S{enum{T=I};};
	
	typedef S<1> Connected;
	typedef S<2> Disconnected;
	typedef S<3> Readable;
	typedef S<4> Writable;

	virtual void on(Connected,cl_tcpchannel* ch){}
	virtual void on(Disconnected,cl_tcpchannel* ch){}
	virtual void on(Readable,cl_tcpchannel* ch,int* pwait){}
	virtual void on(Writable,cl_tcpchannel* ch){}
};

class cl_tcpchannel : public cl_rthandle
	,public cl_caller<cl_tcpchannelListener>
{
	typedef list<cl_memblock*> SendList;
	typedef SendList::iterator SendIter;
public:
	cl_tcpchannel(void);
	virtual ~cl_tcpchannel(void);
public:
	int attach(SOCKET s,sockaddr_in& addr);
	int connect(unsigned int ip,unsigned short port);
	int disconnect();
	int send(cl_memblock *b,bool more=false);
	int recv(char *b,int size);

	virtual int sock(){return (int)m_fd;}
	virtual int handle_input();
	virtual int handle_output();
	virtual int handle_error();

protected:
	virtual void reset();
	void close_socket();
	int on_connected();

protected:
	SOCKET			m_fd;
	SendList		m_slist; //sending list
	int				m_smore; //recode last fire writable if call send();
	int				m_is_regwrite; //only be used after connected

	CL_GET(int,m_state,_state)
	CL_GET(unsigned int,m_last_active_tick,_last_active_tick)
	CL_GET(unsigned int,m_hip,_hip)
	CL_GET(unsigned short,m_hport,_hport)
};

