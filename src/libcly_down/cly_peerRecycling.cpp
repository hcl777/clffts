#include "cly_peerRecycling.h"


cly_peerRecycling::cly_peerRecycling(void)
{
	cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),1,2000);
}


cly_peerRecycling::~cly_peerRecycling(void)
{
	cl_timerSngl::instance()->unregister_all(static_cast<cl_timerHandler*>(this));
}

void cly_peerRecycling::put_peer(cly_peer* peer)
{
	m_pending_dels.push_back(peer);
}
void cly_peerRecycling::clear_pending()
{
	list<cly_peer*> ls;
	ls.swap(m_pending_dels);
	for(list<cly_peer*>::iterator it=ls.begin();it!=ls.end();++it)
	{
		(*it)->disconnect();
		delete (*it);
	}
	ls.clear();
}
void cly_peerRecycling::on_timer(int e)
{
	clear_pending();
}

