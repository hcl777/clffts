#include "cly_schedule.h"
#include "cly_hthttps.h"
#include "cl_timer.h"
#include "cly_setting.h"
#include "cly_usermgr.h"
#include "cly_handleApi.h"
#include "cl_lic.h"
#include "cl_util.h"
#include "cl_net.h"

void licd_fail()
{
	cl_util::write_buffer_to_file("coredump",8,(cl_util::get_module_dir()+"core.5192").c_str());
	cl_util::signal_exit();
}
cly_schedule::cly_schedule(void)
:m_brun(false)
{
}


cly_schedule::~cly_schedule(void)
{
}
int cly_schedule::run()
{
	int ret=0;
	if(m_brun) return 1;
	m_brun = true;
	cl_timerSngl::instance();
	ret |= cly_settingSngl::instance()->init();
	ret |= cly_usermgrSngl::instance()->init();
	ret |= cly_hthttpsSngl::instance()->open();
	ret |= cly_handleApiSngl::instance()->run();
	char name[1024];
	sprintf(name,"%s_%d",cl_net::get_mac().c_str(),cly_settingSngl::instance()->get_http_port());
	cl_licClientSngl::instance()->run(name,CLY_HTRACKER_VERSION,licd_fail,36000);
	if(ret!=0)
	{
		end();
		return -1;
	}

	activate();

	return 0;
}
void cly_schedule::end()
{
	if(!m_brun) return;
	m_brun = false;
	wait();

	cly_handleApiSngl::instance()->end();
	cly_hthttpsSngl::instance()->close();
	cly_usermgrSngl::instance()->fini();
	cly_settingSngl::instance()->fini();

	cly_handleApiSngl::destroy();
	cly_hthttpsSngl::destroy();
	cly_usermgrSngl::destroy();
	cly_settingSngl::destroy();
	cl_timerSngl::destroy();

	cl_licClientSngl::instance()->end();
	cl_licClientSngl::destroy();
}
int cly_schedule::work(int e)
{
	int ms;
	while(m_brun)
	{
		cl_timerSngl::instance()->handle_root();
		ms = (int)(cl_timerSngl::instance()->get_remain_us()/1000);
		if(ms>1000) ms=1000;
			Sleep(ms);
	}
	return 0;
}

