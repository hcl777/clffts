#pragma once
#include "cl_thread.h"
#include "cl_singleton.h"
#include "cl_MessageQueue.h"
#include "cl_timer.h"
#include "cly_cTrackerInfo.h"

#define CLY_CTRACKER_MSG_CONF  101
#define CLY_CTRACKER_MSG_KEEPALIVE_ACK 102
#define CLY_CTRACKER_MSG_SOURCE  103
#define CLY_CTRACKER_MSG_DDLIST 104
#define CLY_CTRACKER_CHSUM_DISTINCT  105
#define CLY_CTRACKER_MSG_GET_FINIFILES 106


typedef void (*CLY_CTRACKER_CALLBACK)(int msg,void* param,void* param2);
typedef struct tag_cTrackerConf
{
	string				peer_name;
	int					peer_id;
	int					group_id;
	int					lang_utf8;
	string				tracker;
	string				app_version;
	string				workdir;
	string				progress_fields;
	string				fdfile_fields; //指定字符串格式
	string				uacaddr;
	CLY_CTRACKER_CALLBACK func;
}cTrackerConf_t;
typedef struct tag_cTrackerInfo
{
	string			down_state;
	string			up_state;
}cTrackerInfo_t;

class cly_cTracker : public cl_thread,public cl_timerHandler
{
public:
	cly_cTracker(void);
	virtual ~cly_cTracker(void);
	typedef cl_SimpleMutex Mutex;
	typedef cl_TLock<Mutex> Lock;
public:
	int init(const cTrackerConf_t& conf);
	void run();
	void end();
	virtual int work(int e);
	virtual void on_timer(int e);
	bool is_login()const {return m_blogin;}

	void set_checksum(cly_seg_fchsum_t* pseg);
	void update_nat(const string& uacaddr);//ip:port:nattype
	void search_source(const string& hash);
	void report_fdfile(const string& fdfile,cly_seg_fchsum_t* pseg);
	void report_progress(list<string>& ls);
	void report_error(const cly_report_error_t& errinf);
	void get_ddlist(const string& taskid);
	void get_finifiles();
	void put_finifiles(const string& path);

	int _update_filename(const string& hash,const string& name); //阻塞直接执行
	int _get_fileinfo(const string& hash,clyt_fileinfo_t& fi); //阻塞直接执行
	int _upload_task(const string& hids, const string& hash);
private:
	static void _ClearMessageFun(cl_Message* msg);

	void _login();
	void _keepalive();
	void _update_nat(const string& uacaddr);
	void _search_source(string& hash);
	void _report_progress(list<string>& ls);
	void _report_fdfile();
	void _report_error(cly_report_error_t* errinf);
	void _get_ddlist(const string& taskid);
	void _check_fini_chsum();
	void _get_finifiles();
	void _put_finifiles(const string& path);
public:
	int m_login_fails;
	int m_keepalive_fails;
	cTrackerInfo_t		m_info;
private:
	Mutex				m_mt;
	bool				m_brun;
	bool				m_blogin;
	cTrackerConf_t		m_conf;
	cl_MessageQueue		m_msgQueue;

	
	cly_seg_fchsum_t	m_seg_fchsum;
	bool				m_bchecksum;
	list<string>		m_fdlist;
	list<string>		m_pgls;
};

typedef cl_singleton<cly_cTracker> cly_cTrackerSngl;

#define CLY_CGI_ "/clyun"

