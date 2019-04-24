#include "cly_peer.h"
#include "cl_net.h"

cly_peer::cly_peer(void)
:m_peerdata(NULL)
{
	m_ch = new cl_uchannel();
	m_ch->add_listener(this);
	m_block = cl_memblock::allot(CLYP_MAX_BLOCKDATA_SIZE);
	m_block->wpos = 8; //使用wpos作为限制读大小
	DEBUGMSG("#NEW cly_peer() \n");
}

cly_peer::~cly_peer(void)
{
	assert(DISCONNECTED==m_ch->get_state());
	m_ch->remove_listener(this);
	delete m_ch;
	if(m_block)
	{
		m_block->release();
		m_block = NULL;
	}
	
	DEBUGMSG("#DELETE cly_peer() \n");
}
int cly_peer::connect(unsigned int ip,unsigned short port,int nattype/*=0*/)
{
	DEBUGMSG("# connect to(%s:%d:%d)\n",cl_net::ip_htoa(ip),port,nattype);
	return m_ch->connect(ip,port,nattype);
}

int cly_peer::disconnect()
{
	if(DISCONNECTED==m_ch->get_state())
	{
		//未连接时也要回调，上层在Disconnected()时才删除
		call(cly_peerListener::Disconnected(),this);
		return 0;
	}
	return m_ch->disconnect();
}

int cly_peer::send(cl_memblock *b)
{
	return m_ch->send(b);
}
void cly_peer::on(Connected,cl_uchannel* ch)
{
	call(cly_peerListener::Connected(),this);
}
void cly_peer::on(Disconnected,cl_uchannel* ch)
{
	m_block->rpos=0;
	m_block->wpos=8;
	call(cly_peerListener::Disconnected(),this);
}
void cly_peer::on(Readable,cl_uchannel* ch,const int& wait)
{	
	int cpsize = 0;
	while(CONNECTED==m_ch->get_state())
	{
		cpsize = m_block->length();
		cpsize = ch->recv(m_block->read_ptr(),cpsize);
		if(cpsize<=0)
			return;
		m_block->rpos += cpsize;
		if(0==m_block->length())
			on_data();
	}
}
void cly_peer::on(Writable,cl_uchannel* ch)
{
	call(cly_peerListener::Writable(),this);
}
void cly_peer::on_data()
{
	if(8==m_block->wpos)
	{
		int size = cl_bstream::ltoh32(*((uint32*)(m_block->buf+4)));
		if(CLY_PTL_STX_32!= *(uint32*)m_block->buf || size<=8 || size>CLYP_MAX_BLOCKDATA_SIZE)
		{
			DEBUGMSG("#****wrong packet!****\n");
			disconnect(); //里面会删除m_block
			return;
		}
		m_block->wpos = size;
	}
	else
	{
		m_block->rpos = 0;
		call(cly_peerListener::Data(),this,m_block->buf,m_block->wpos);
		if(CONNECTED!=m_ch->get_state())
			return;
		m_block->wpos = 8;
	}
}
