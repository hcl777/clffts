#pragma once
#include "cl_basetypes.h"
#include "cl_singleton.h"
#include "uac.h"
#include "cly_trackerProto.h"

#define CLY_DOWN_VERSION "cly_down_20181217"

/*************************
������: cly_down.xml
1.linux��������ʱ��Ϊ:
��ؽ���,������,������.
�ɼ�ؽ��̼�������,��������������.
������ֻ��һ��.
�����̶��,ִֻ���ļ�������.
������Ϊֻ��.
2.win/androidֱ�ӵ�����load����������.
*************************/
//process_config_t Ϊ��������ʱ��ֻ������.

typedef struct tag_cly_dynamic_conf
{
	int			use_local_limit; //�Ƿ�ʹ�ñ�����������
	int			limit_share_speediKB;
	int			limit_share_min_speediKB; //ָ�������ٶ�
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
	int				lang_utf8; //1��ʾϵͳ����Ϊutf8
	int				main_process; //1:������,0:������
	string			peer_name; //����ID,��MAC+·������.
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
	int				down_freespaceG; //���ٱ����ռ�
	int				down_active_num; //�Զ��Ĳ������ظ���
	unsigned int	down_encrypt; //�Զ������Ƿ���ܱ���

	//upload
	string			upload_hids;

	//limit
	int				limit_share_cnns; //�ͻ��˲�Ҫ̫��
	int				limit_share_threads; //���������̶߳�����

	//uac
	int				uac_mtu;
	unsigned short	uac_port;
	int				uac_max_sendwin;
	int				uac_sendtimer_ms;
	string			uac_default_addr;	//

	//cache
	int				cache_vod_disk; //0:��д��,�Ƿ�дӲ��
	int				cache_win_num;

	cly_dynamic_conf_t dconf; //dynamic config;
	//timer
	int				timer_report_progressS; //��
	int				timer_get_ddlistS; //��,0��ʾ���ϱ�

	//oldhash ����
	string			url_oldhash_relation;
}cly_config_t;


int cly_config_load(cly_config_t& conf,const char* rootdir=NULL);

void cly_config_sprint(cly_config_t* pc,char* buf,int size);

int cly_dconf_load(cly_dynamic_conf_t& dc,const string& path);
void cly_dconf_sprint(cly_dynamic_conf_t& dc,char* buf);


//*******************************************************
//�����ǽ����ڲ�ʹ��

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
	string find_exist_fullpath(const string& path); //���down_path,share_path,���߾���·��
	string find_new_fullpath(const string& path); //��Ե��ҿռ�����down_path
	string find_path_name(const string& path); //�������ڻָ�ȫ·������·��
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
////ɨ��hash����
//bool cly_put_hash_to_shafile(const string& path,const string& hash,const string& subhash);
//bool cly_get_hash_from_shafile(const string& path,string& hash,string& subhash);
