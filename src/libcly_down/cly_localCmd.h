#pragma once
#include "cly_d.h"
#include "cl_thread.h"

class cly_localCmd : public cl_thread
{
public:
	cly_localCmd(void);
	virtual ~cly_localCmd(void);
public:
	int run();
	void end();
	virtual int work(int e);
	
	int share_file(const string& path,const string& name,bool try_update_name,string& outHash, bool up_task);
	int scanf() { m_scan_tick = 1000000; return 0;} //立即执行

	
private:
	void down_test();
	int share_list(list<string>& ls);
	void share_byfile(const string& path);
	void scan_share();
	//关联旧版本节目数据
	void oldhash_relation(const string& srcpath,const string& newhash);

private:
	bool			m_brun;
	int				m_scan_tick;
};

typedef cl_singleton<cly_localCmd> cly_localCmdSngl;
