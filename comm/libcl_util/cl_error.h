#pragma once
#include "cl_basetypes.h"
#include "cl_thread.h"
/*
说明:本类记录错误信息缓存本地并且上报服务器.
上报记录为: 程序名|版本号|错误码|描述(125字节)
*/
class cl_error : public cl_thread
{
public:
	cl_error(void);
	virtual ~cl_error(void);

public:
	void set_error(int err); //如果不open,仅临时记
	int get_error()const {return m_last_err;}

	int open_log(const string& appname,const string& appver,
		const string& logpath,const string& report_url);
	void close();

public:
	int m_last_err;
	bool m_bopen;
	string m_appname,m_appver;
	string m_url; //记录上报路径.
	string m_localpath; //永久积累.
	string m_reportpath; //上报完则删除.
};

