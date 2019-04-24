#include "cly_setting.h"
#include "cl_inifile.h"
#include "cl_util.h"


cly_setting::cly_setting(void)
{
	m_http_port = 8098;
	m_http_num = 5;
	m_begin_time = cl_util::time_to_datetime_string(time(NULL));
	m_http_support_keepalive = true;
}


cly_setting::~cly_setting(void)
{
}
int cly_setting::init()
{	
	m_datadir = cl_util::get_module_dir() + "htdata/";
	cl_util::create_directory_by_filepath(m_datadir);
	cl_util::create_directory_by_filepath(m_datadir+"taskinfo/");
	cl_util::create_directory_by_filepath(m_datadir+"ddlist/");
	cl_util::create_directory_by_filepath(m_datadir+"fflist/");
	cl_util::create_directory_by_filepath(m_datadir + "log/");
	return load_setting();
}
int cly_setting::fini()
{
	return 0;
}
int cly_setting::load_setting()
{
	cl_inifile ini;
	char buf[1024];
	string str;
	string path = cl_util::get_module_path()+".ini";
	cl_util::str_replace(path,".exe","");
	if(-1==ini.open(path.c_str()))
		return -1;
	
	//user:pass@ip:port/dbname
	m_dbaddr = ini.read_string("db","addr","",buf,1024);

	//http
	m_http_port = ini.read_int("http","port",8098);
	m_http_num = ini.read_int("http","num",20);
	if(m_http_num<1) m_http_num = 1;
	if(m_http_port==0) m_http_port = 8098;
	m_http_support_keepalive = ini.read_int("http", "support_keepalive", 1)>0?true:false;

	//http_api
	m_api_report_finifile = ini.read_string("http_api","report_finifile","",buf,1024);
	m_api_report_finifile_hids = ini.read_string("http_api","report_finifile_hids","",buf,1024);
	m_api_get_group_id = ini.read_string("http_api","get_group_id","",buf,1024);

	//userconf
	m_userconf.peer_id = 0;
	m_userconf.limit_share_speediKB = ini.read_int("userconf","limit_share_speediKB",0);
	m_userconf.limit_share_speedKB = ini.read_int("userconf","limit_share_speedKB",0);
	m_userconf.limit_down_speedKB = ini.read_int("userconf","limit_down_speedKB",0);
	m_userconf.timer_keepaliveS = ini.read_int("userconf","timer_keepaliveS",0);
	m_userconf.timer_getddlistS = ini.read_int("userconf","timer_getddlistS",0);
	m_userconf.timer_reportprogressS = ini.read_int("userconf","timer_reportprogressS",0);

	//print
	DEBUGMSG("# [SETTING]: \n");
	DEBUGMSG("# dbaddr = %s \n",m_dbaddr.c_str());
	DEBUGMSG("# http_port = %d \n",m_http_port);
	DEBUGMSG("# http_num = %d \n",m_http_num);
	DEBUGMSG("# api_report_finifile = %s \n",m_api_report_finifile.c_str());
	DEBUGMSG("# [SETTING end]: \n");
	return 0;
}
