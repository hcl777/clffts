#pragma once
#include "cl_thread.h"
#include "cly_config.h"
#include "cl_singleton.h"

class cly_schedule : public cl_thread
{
public:
	cly_schedule(void);
	virtual ~cly_schedule(void);
public:
	int run(cly_config_t* pc);
	void end();
	virtual int work(int e);
private:
	int init(cly_config_t* pc);
	void fini();
private:
	bool m_brun;
};
typedef cl_singleton<cly_schedule> cly_schedule_sngl;

