#pragma once
#include "cly_downAutoInfo.h"

class cly_downAuto : public cl_timerHandler
{
public:
	cly_downAuto(void);
	virtual ~cly_downAuto(void);

public:
	typedef DownAutoInfoList  FileList;
	typedef FileList::iterator FileIter;
	typedef cl_CriticalSection Mutex;

	int Init();
	int Fini();

	void update_timer(int timer_getddlistS,int timer_reportprogressS);
	void load_ddlist(const string& path);
	int AddDown(const string& hash,const string& filename,int ftype,int priority, int save_original);
	int DelDown(const string& hash);
	int Start(const string& hash);
	int Stop(const string& hash);
	int GetDownInfo(FileList& ls);
	void report_progress();

	virtual void on_timer(int e);

	void pause();
	void resume();
	bool is_pause()const { return m_bpause; }

	void Save();
private:
	cly_downAutoInfo *FindDown(const string& hash);

	static FileIter GetMaxScoreInfo(FileList& ls);
	static FileIter GetMinScoreInfo(FileList& ls);
	static FileIter GetInfo(FileList& ls,const string& hash);
	static cly_downAutoInfo* DetachInfo(FileList& ls,const string& hash);

	void InitActivat(); //初始化时只启动上次为1状态的
	void DownActive(int tactics);
	bool ActiveI(cly_downAutoInfo *inf);
	void DownAdjust();
	void UpdateDownInfo();

	void Load();
	void clear();
protected:
	Mutex				m_mt;
	unsigned int		m_iCurrTick;
	FileList			m_lsWaiting;    //等待下载队列
	FileList			m_lsDowning;	  //正在下载队列
	FileList			m_lsStop;	  //停止下载队列
	bool				m_binitactive;
	bool				m_haveSpeed;
	bool				m_bpause;


	GETSET(string,m_updateid,_updateid)
};
typedef cl_singleton<cly_downAuto> cly_downAutoSngl;
