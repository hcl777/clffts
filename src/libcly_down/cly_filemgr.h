#pragma once
#include "cly_fileinfo.h"
#include "cl_thread.h"
#include "cly_cTracker.h"

class cly_filemgr
{
public:
	cly_filemgr(void);
	virtual ~cly_filemgr(void);
	
	typedef cl_CriticalSection Mutex;
	typedef map<string,cly_fileinfo*> filemap;
	typedef filemap::iterator fileiter;
	
public:
	int init();
	void fini();

	cly_fileinfo* create_downinfo(const string& hash,int ftype,const string& path,bool bsave_original);
	int create_readyinfo(const string& hash,const string& subhash,const string& path,const string& name, bool report);
	int delete_downinfo(const string& hash);
	int delete_readyinfo(const string& hash,bool delPhyFile);
	int delete_info(const string& hash,bool delPhyFile);
	int get_ready_num() const {
		return m_readyInfo.size();
	}
	cly_fileinfo* get_downinfo(const string& hash);
	cly_fileinfo* get_readyinfo(const string& hash);
	cly_fileinfo* get_readyinfo_by_path(const string& path);
	cly_fileinfo* get_fileinfo(const string& hash);
	int check_exist_share_bypath(const string& path);
	cl_memblock* read_block(const string& hash,unsigned int index);
	int refer_file(const string& hash); //不执行打开文件（实际读写时打开)
	int release_file(const string& hash); //ref==0时，若是ready文件则close
	int read_down_data(const string& hash,size64_t pos,char* buf,int size); //hash校验使用

	int on_file_done(const string& hash);
	int save_readyfile_utf8(const string& path);
private:
	void load_downinfo();
	void save_downinfo_main(); //只保存总信息
	void load_readyinfo();
	void save_readyinfo();

	void report_finifile(cly_fileinfo* fi);
	void report_delefile(cly_fileinfo* fi);
private:
	bool			m_binit;
	Mutex			m_mt;
	filemap			m_downInfo,m_readyInfo;
	string			m_downinfo_path,m_readyinfo_path;
	
	int				m_readyinfo_mtime;
	int				m_max_no;
	cly_seg_fchsum_t	m_seg_fchsum;
};
typedef cl_singleton<cly_filemgr> cly_filemgrSngl;
