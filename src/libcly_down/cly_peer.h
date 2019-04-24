#pragma once
#include "cl_uchannel.h"
#include "cly_proto.h"

class cly_peer;
class cly_peerListener
{
public:
	virtual ~cly_peerListener(void){}

	template<int I>
	struct S{enum{T=I};};

	typedef S<2> Connected;
	typedef S<3> Disconnected;
	typedef S<5> Data;
	typedef S<6> Writable;

	virtual void on(Connected,cly_peer* peer){}
	virtual void on(Disconnected,cly_peer* peer){}
	virtual void on(Data,cly_peer* peer,char* buf,int len){}
	virtual void on(Writable,cly_peer* peer){}
};

class cly_peer: public cl_uchannelListener
	,public cl_caller<cly_peerListener>
{
public:
	cly_peer(void);
	~cly_peer(void);

public:
	int connect(unsigned int ip,unsigned short port,int nattype=0);
	int disconnect();
	int send(cl_memblock *b);
	//外部直接获取channel然后attach
	cl_uchannel* get_channel(){return m_ch;}
	//peerdata:
	void* get_peerdata()const {return m_peerdata;}
	void set_peerdata(void* a) {m_peerdata = a;}

	virtual void on(Connected,cl_uchannel* ch);
	virtual void on(Disconnected,cl_uchannel* ch);
	virtual void on(Readable,cl_uchannel* ch,const int& wait);
	virtual void on(Writable,cl_uchannel* ch);

public:
	template<typename T>
	static int send_ptl_packet(cly_peer* peer,uchar cmd,const char* hash,T& inf,int maxsize)
	{
		CL_MBLOCK_NEW_RETURN_INT(block,maxsize,-1)
		cl_ptlstream ss(block->buf,block->buflen,0);
		clyp_head_t head;
		head.cmd = cmd;
		strcpy(head.hash,hash);
		ss << head;
		ss << inf;
		ss.fitsize32(4);
		block->wpos = ss.length();
		return peer->send(block);
	}
	static int send_ptl_packet2(cly_peer* peer,uchar cmd,const char* hash,int maxsize)
	{
		CL_MBLOCK_NEW_RETURN_INT(block,maxsize,-1)
		cl_ptlstream ss(block->buf,block->buflen,0);
		clyp_head_t head;
		head.cmd = cmd;
		strcpy(head.hash,hash);
		ss << head;
		ss.fitsize32(4);
		block->wpos = ss.length();
		return peer->send(block);
	}
private:
	void on_data();

private:
	cl_uchannel* m_ch;
	cl_memblock * m_block;
	void* m_peerdata;
};
