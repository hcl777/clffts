#pragma once
#include "cly_peer.h"
#include "cl_timer.h"
#include "cl_singleton.h"

class cly_peerRecycling : public cl_timerHandler
{
public:
	cly_peerRecycling(void);
	virtual ~cly_peerRecycling(void);
public:
	void put_peer(cly_peer* peer);
	void clear_pending();
	virtual void on_timer(int e);
	
private:
	list<cly_peer*>		m_pending_dels;
};
typedef cl_singleton<cly_peerRecycling> cly_peerRecyclingSngl;
