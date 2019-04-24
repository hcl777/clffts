#pragma once
#include "cl_basetypes.h"
#include "cl_singleton.h"
#include "cly_trackerProto.h"

#define CLY_HTRACKER_VERSION "cly_htracker_20181123"

class cly_setting
{
public:
	cly_setting(void);
	~cly_setting(void);
public:
	int init();
	int fini();
private:
	int load_setting();

public:
	cly_seg_config_t m_userconf;
protected:
	GETSET(string,m_begin_time,_begin_time)
	GETSET(string,m_datadir,_datadir)

	GETSET(string,m_dbaddr,_dbaddr)

	GETSET(unsigned short,m_http_port,_http_port)
	GETSET(int,m_http_num,_http_num)
	GETSET(bool, m_http_support_keepalive, _http_support_keepalive)

	GETSET(string,m_api_report_finifile,_api_report_finifile)
	GETSET(string,m_api_report_finifile_hids,_api_report_finifile_hids)
	GETSET(string,m_api_get_group_id,_api_get_group_id)
};

typedef cl_singleton<cly_setting> cly_settingSngl;

