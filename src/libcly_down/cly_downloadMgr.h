#pragma once
#include "cly_download.h"

class cly_downloadMgr : public cl_timerHandler
{
public:
	cly_downloadMgr(void);
	~cly_downloadMgr(void);

	typedef struct tag_releaseInfo
	{
		string         hash;
		unsigned int   beginTick;
		tag_releaseInfo(void) {}
		tag_releaseInfo(const string& _hash,unsigned int _beginTick)
			:hash(_hash),beginTick(_beginTick){}
	}releaseInfo_t;
	typedef struct tag_task
	{
		int ref;
		cly_download* dl;
	}task_t;

	typedef map<string,task_t> DownloadMap;
	typedef DownloadMap::iterator DownloadIter;
public:
	int init();
	void fini();
	virtual void on_timer(int e);
	//可能多处引用同一任务
	int create_download(const string& hash,int ftype,const string& path, bool bsave_original);
	int delete_file(const string& hash,bool isDelPhy);
	void refer(const string& hash);
	void release(const string& hash); //延后2秒关闭
	int add_source(const string& hash,list<string>& ls);
	int get_fileinfo(const string& hash,cly_downloadInfo_t& inf);
	int get_allspeed(int& allspeed,list<string>& ls); //获取总速度和每个下载的速度
	int get_down_num() const { return m_downloads.size(); }
	int get_conn_num();
private:
	
private:
	DownloadMap			m_downloads;
	list<releaseInfo_t> m_release;
};

typedef cl_singleton<cly_downloadMgr> cly_downloadMgrSngl;

