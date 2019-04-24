#include "cl_error.h"


cl_error::cl_error(void)
	:m_last_err(0)
	,m_bopen(false)
{
}


cl_error::~cl_error(void)
{
}
void cl_error::set_error(int err) 
{
	m_last_err=err;
	if(0!=err && m_bopen)
	{
	}
}
int cl_error::open_log(const string& appname,const string& appver,
		const string& logpath,const string& report_url)
{
	return 0;
}
void cl_error::close()
{
}

