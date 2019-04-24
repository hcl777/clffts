#pragma once
#include "uac_Socket.h"
#include "cl_speaker.h"
#include "cl_incnet.h"
#include "cl_memblock.h"

enum {DISCONNECTED=0,CONNECTING=1,CONNECTED=2};
class cl_uchannel;
class cl_uchannelListener
{
public:
	virtual ~cl_uchannelListener(void){}

	template<int I>
	struct S{enum{T=I};};
	
	typedef S<2> Connected;
	typedef S<3> Disconnected;
	typedef S<4> Data;
	typedef S<4> Readable;
	typedef S<5> Writable;
	typedef S<6> ReadLimit;

	virtual void on(Connected,cl_uchannel* ch){}
	virtual void on(Disconnected,cl_uchannel* ch){}
	virtual void on(Readable,cl_uchannel* ch,const int& wait){}
	virtual void on(Writable,cl_uchannel* ch){}
};


class cl_uchannel : public UAC_Socket,public cl_speaker<cl_uchannelListener>
{
public:
	cl_uchannel(void);
	virtual ~cl_uchannel(void);
	void reset();

	int attach(SOCKET s,sockaddr_in& addr);
	int connect(unsigned int ip,unsigned short port,int nattype=0);
	int disconnect();
	int send(cl_memblock *b,bool more=false);  //-1:false; 0:send ok; 1:put int sendlist
	int recv(char *b,int size);
	int get_state() const {return m_state;}

	int set_bandwidth(unsigned int bwB){return uac_setbandwidth(bwB);}
	int set_sndbuf(int bufsize){return uac_setsendbuf(bufsize);}
	int set_rcvbuf(int bufsize){return uac_setrecvbuf(bufsize);}

	virtual int uac_on_read();
	virtual void uac_on_write();
	virtual void uac_on_connected(); //重载通知上层
	virtual void uac_on_connect_broken();

	unsigned int get_hip() const { return m_hip; }
	unsigned short get_hport() const { return m_hport; }
	
public:
	int	m_state;
	unsigned int m_hip;
	unsigned short m_hport;
	unsigned int m_last_active_tick;
	bool m_is_accept;

};



