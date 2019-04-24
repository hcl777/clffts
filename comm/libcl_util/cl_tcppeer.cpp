#include "cl_tcppeer.h"
#include <assert.h>

cl_tcppeer::cl_tcppeer(uint32 stx,int maxpsize)
{
	m_ch = new cl_tcpchannel();
	m_ch->set_listener(static_cast<cl_tcpchannelListener*>(this));
	m_stx = stx;
	m_block = cl_memblock::allot(maxpsize);
	m_block->wpos = 8; //使用wpos作为限制读大小
	m_peerdata = NULL;
}


cl_tcppeer::~cl_tcppeer(void)
{
	assert(CL_DISCONNECTED==m_ch->get_state());
	m_ch->set_listener(NULL);
	delete m_ch;
	if(m_block)
	{
		m_block->release();
		m_block = NULL;
	}
	
}
int cl_tcppeer::connect(unsigned int ip,unsigned short port)
{
	return m_ch->connect(ip,port);
}

int cl_tcppeer::disconnect()
{
	if(CL_DISCONNECTED==m_ch->get_state())
	{
		//未连接时也要回调，上层在Disconnected()时才删除
		call(cl_tcppeerListener::Disconnected(),this);
		return 0;
	}
	return m_ch->disconnect();
}

void cl_tcppeer::on(Connected,cl_tcpchannel* ch)
{
	call(cl_tcppeerListener::Connected(),this);
}
void cl_tcppeer::on(Disconnected,cl_tcpchannel* ch)
{
	m_block->rpos=0;
	m_block->wpos=8;
	call(cl_tcppeerListener::Disconnected(),this);
}
void cl_tcppeer::on(Readable,cl_tcpchannel* ch,int* pwait)
{	
	int cpsize = 0;
	while(CL_CONNECTED==m_ch->get_state())
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
void cl_tcppeer::on(Writable,cl_tcpchannel* ch)
{
	call(cl_tcppeerListener::Writable(),this);
}
void cl_tcppeer::on_data()
{
	if(8==m_block->wpos)
	{
		int size = cl_bstream::ltoh32(*((uint32*)(m_block->buf+4)));
		unsigned int stx = cl_bstream::ltoh32(*((uint32*)(m_block->buf)));
		if(m_stx!= stx || size<=8 || size>m_block->buflen)
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
		call(cl_tcppeerListener::Packet(),this,m_block->buf[8],m_block->buf+9,m_block->wpos-9);
		if(CL_CONNECTED!=m_ch->get_state())
			return;
		m_block->wpos = 8;
	}
}

