#pragma once
#include "cl_timer.h"

class cly_trackerCmd : public cl_timerHandler
{
public:
	cly_trackerCmd(void);
	~cly_trackerCmd(void);
public:
	int init();
	void run();
	void fini();
	virtual void on_timer(int e);
	static void on_ctracker_msg(int msg,void* param,void* param2);

	static void on_uac_natok(int ntype);
	static void on_uac_ipport(unsigned int ,unsigned short);

private:
	static void _update_nattype();
};

typedef cl_singleton<cly_trackerCmd> cly_trackerCmdSngl;

