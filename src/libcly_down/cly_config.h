#pragma once
#include "cl_basetypes.h"
#include "cl_singleton.h"
#include "uac.h"
#include "cly_trackerProto.h"

#define CLY_DOWN_VERSION "cly_down_20181217"

/*************************
主配置: cly_down.xml
1.linux程序运行时分为:
监控进程,主进程,副进程.
由监控进程加载配置,并拉起主副进程.
主进程只有一个.
副进程多个,只执行文件共享功能.
主配置为只读.
2.win/android直接单进程load主配置运行.
*************************/
//process_config_t 为程序启动时的只读配置.

typedef struct tag_cly_dynamic_conf
{
	int			use_local_limit; //是否使用本地限速配置
	int			limit_share_speediKB;
	int			limit_share_min_speediKB; //指定最少速度
	int			limit_share_speedKB;
	int			limit_down_speedKB;
	tag_cly_dynamic_conf(void)
	{
		use_local_limit = 0;
		limit_share_speediKB = 0;
		limit_share_min_speediKB = 0;
		limit_share_speedKB = 0;
		limit_down_speedKB = 0;
	}
}cly_dynamic_conf_t;

typedef struct tag_cly_config
{
	int				lang_utf8; //1表示系统编码为utf8
	int				main_process; //1:主进程,0:副进程
	string			peer_name; //主机ID,用MAC+路径生成.
	int				used;

	int				peer_id;
	int				group_id; //
	string			rootdir;
	string			server_tracker;//server:port
	string			server_stun;//server:port
	string			sn;
	unsigned short	http_port;
	int				http_threadnum;
	
	//share
	string			share_path;
	string			share_suffix;

	//down_auto
	string			down_path;
	int				down_freespaceG; //最少保留空间
	int				down_active_num; //自动的并发下载个数
	unsigned int	down_encrypt; //自动下载是否加密保存

	//upload
	string			upload_hids;

	//limit
	int				limit_share_cnns; //客户端不要太大
	int				limit_share_threads; //开多少条线程读数据

	//uac
	int				uac_mtu;
	unsigned short	uac_port;
	int				uac_max_sendwin;
	int				uac_sendtimer_ms;
	string			uac_default_addr;	//

	//cache
	int				cache_vod_disk; //0:不写盘,是否写硬盘
	int				cache_win_num;

	cly_dynamic_conf_t dconf; //dynamic config;
	//timer
	int				timer_report_progressS; //秒
	int				timer_get_ddlistS; //秒,0表示不上报

	//oldhash 关联
	string			url_oldhash_relation;
}cly_config_t;


int cly_config_load(cly_config_t& conf,const char* rootdir=NULL);

void cly_config_sprint(cly_config_t* pc,char* buf,int size);

int cly_dconf_load(cly_dynamic_conf_t& dc,const string& path);
void cly_dconf_sprint(cly_dynamic_conf_t& dc,char* buf);


//*******************************************************
//以下是进程内部使用

#define CLYSET cly_settingSngl::instance()
#define cly_pc cly_settingSngl::instance()->m_pc

#define CLY_DIR_INFO cly_settingSngl::instance()->get_info_path()
#define CLY_DIR_LOG cly_settingSngl::instance()->get_log_path()
#define CLY_DIR_SHARE cly_settingSngl::instance()->get_share_path()
#define CLY_DIR_AUTO cly_settingSngl::instance()->get_auto_path()
#define CLY_DIR_TRACKER cly_settingSngl::instance()->get_tracker_path()
class cly_setting
{
public:
	cly_setting(void){}
	~cly_setting(void){}
public:
	int init(cly_config_t* pc);
	list<string>& get_down_path_list(){ return m_down_path_list;}
	string find_exist_fullpath(const string& path); //相对down_path,share_path,或者绝对路径
	string find_new_fullpath(const string& path); //相对的找空间最大的down_path
	string find_path_name(const string& path); //查找用于恢复全路径的子路径
	int get_freespace_GB();
	int update_speed(int share_speediKB,int share_speedKB,int down_speedKB,bool bnetset);
private:
	void format_down_path(const string& paths);
public:
	cly_config_t*			m_pc;
	list<string>			m_down_path_list;
	list<string>			m_share_path_list;
	list<string>			m_sn_list;
	UAC_sockaddr			m_uacaddr;
	string					m_local_ip;
	string					m_begin_time;
protected:
	GETSET(string,m_info_path,_info_path);
	GETSET(string,m_log_path,_log_path);
	GETSET(string,m_share_path,_share_path);
	GETSET(string,m_auto_path,_auto_path);
	GETSET(string,m_tracker_path,_tracker_path);
};
typedef cl_singleton<cly_setting> cly_settingSngl;

//
////扫描hash缓冲
//bool cly_put_hash_to_shafile(const string& path,const string& hash,const string& subhash);
//bool cly_get_hash_from_shafile(const string& path,string& hash,string& subhash);
