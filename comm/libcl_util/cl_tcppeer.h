#pragma once
#include "cl_tcpchannel.h"
#include "cl_ntypes.h"
#include "cl_bstream.h"

class cl_tcppeer;
class cl_tcppeerListener
{
public:
	virtual ~cl_tcppeerListener(void){}

	template<int I>
	struct S{enum{T=I};};

	typedef S<1> Connected;
	typedef S<2> Disconnected;
	typedef S<3> Packet;
	typedef S<4> Writable;

	virtual void on(Connected,cl_tcppeer* peer){}
	virtual void on(Disconnected,cl_tcppeer* peer){}
	virtual void on(Packet,cl_tcppeer* peer,uchar cmd,char* buf,int len){}
	virtual void on(Writable,cl_tcppeer* peer){}
};

class cl_tcppeer : public cl_tcpchannelListener
	,public cl_caller<cl_tcppeerListener>
{
public:
	//stx:包标志，maxpsize: 最大接收包大小
	cl_tcppeer(uint32 stx,int maxpsize);
	~cl_tcppeer(void);
	int connect(unsigned int ip,unsigned short port);
	int disconnect();
	//int send(cl_memblock *b){return m_ch->send(b);}
	//外部直接获取channel然后attach
	cl_tcpchannel* get_channel(){return m_ch;}
	//peerdata:
	void* get_peerdata()const {return m_peerdata;}
	void set_peerdata(void* a) {m_peerdata = a;}

	virtual void on(Connected,cl_tcpchannel* ch);
	virtual void on(Disconnected,cl_tcpchannel* ch);
	virtual void on(Readable,cl_tcpchannel* ch,int* pwait);
	virtual void on(Writable,cl_tcpchannel* ch);

public:
	template<typename T>
	int send_ptl_packet(uchar cmd,T& inf,int maxsize)
	{
		CL_MBLOCK_NEW_RETURN_INT(block,maxsize,-1)
		cl_ptlstream ss(block->buf,block->buflen,0);
		ss << m_stx;
		ss.skipw(4); //size
		ss << cmd;
		ss << inf;
		ss.fitsize32(4);
		block->wpos = ss.length();
		return m_ch->send(block);
	}
	int send_ptl_packet2(uchar cmd,int maxsize)
	{
		CL_MBLOCK_NEW_RETURN_INT(block,maxsize,-1)
		cl_ptlstream ss(block->buf,block->buflen,0);
		ss << m_stx;
		ss.skipw(4); //size
		ss << cmd;
		ss.fitsize32(4);
		block->wpos = ss.length();
		return m_ch->send(block);
	}
private:
	void on_data();

private:
	cl_tcpchannel*		m_ch;
	uint32				m_stx;
	cl_memblock *		m_block;
	void*				m_peerdata;
};

