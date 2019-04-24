#pragma once
#include "cl_thread.h"
#include "cl_MessageQueue.h"
#include "cl_singleton.h"
#include "cl_timer.h"

class cly_handleApi : public cl_thread,public cl_timerHandler
{
public:
	cly_handleApi(void);
	virtual ~cly_handleApi(void);
	
	typedef cl_SimpleMutex Mutex;
	typedef cl_TLock<Mutex> Lock;
public:
	int run();
	void end();
	virtual int work(int e);
	virtual void on_timer(int e);

	void report_finifile(const string& hid,const string& hash,const string& name);
	int get_group_id(const string& peer_name);
private:
	static void _ClearMessageFun(cl_Message* msg);

	
	void _report_finifile();
private:
	bool				m_brun;
	Mutex				m_mt;
	list<string>		m_finilist;
	cl_MessageQueue		m_queue;
	map<string,int>		m_report_fini_hids_map;
};
typedef cl_singleton<cly_handleApi> cly_handleApiSngl;


