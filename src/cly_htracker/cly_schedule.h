#pragma once
#include "cl_thread.h"
#include "cl_singleton.h"

class cly_schedule : public cl_thread
{
public:
	cly_schedule(void);
	virtual ~cly_schedule(void);
public:
	int run();
	void end();
	virtual int work(int e);

private:
	bool m_brun;
};
typedef cl_singleton<cly_schedule> cly_scheduleSngl;

