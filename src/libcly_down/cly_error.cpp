#include "cly_error.h"
#include "cly_cTracker.h"
#include "cly_config.h"
#include "cl_util.h"

void cly_error_report(int err,const char* msg)
{
	cly_report_error_t i;
	i.appname = "cly_down";
	i.appver = CLY_DOWN_VERSION;
	i.peer_name = cly_pc->peer_name;
	i.systemver = cl_util::get_system_version_str();
	i.err = err;
	i.description = msg;
	cly_cTrackerSngl::instance()->report_error(i);
}





