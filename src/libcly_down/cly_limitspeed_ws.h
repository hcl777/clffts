#pragma once
#include "cl_thread.h"
#include "cl_timer.h"
#include "cl_basetypes.h"

class cly_limitspeed_ws : public cl_thread,public cl_timer
{
public:
	cly_limitspeed_ws(void);
	virtual ~cly_limitspeed_ws(void);
public:
	int init();
	void fini();
	virtual int work(int e);
	virtual void on_timer(int e);

	int get_limitsendB(const string& peer_name);
};

