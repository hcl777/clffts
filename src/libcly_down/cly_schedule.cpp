#include "cly_schedule.h"
#include "cl_timer.h"
#include "uac.h"
#include "uac_SocketSelector.h"
#include "cl_memblock.h"
#include "cly_d.h"
#include "cly_filemgr.h"
#include "cl_util.h"
#include "cly_peerRecycling.h"
#include "cly_server.h"
#include "cly_downloadMgr.h"
#include "cly_localCmd.h"
#include "cly_downAuto.h"
#include "cly_trackerCmd.h"
#include "cly_httpapi.h"
#include "cl_reactor.h"

cly_schedule::cly_schedule(void)
:m_brun(false)
{
}
cly_schedule::~cly_schedule(void)
{
}

int cly_schedule::run(cly_config_t* pc)
{
	if(m_brun) return 1;
	if(0!=init(pc))
	{
		fini();
		return -1;
	}
	m_brun = true;
	activate();

	cly_checkhashSngl::instance()->run();
	cly_localCmdSngl::instance()->run();
	cly_httpapiSngl::instance()->open();
	cly_trackerCmdSngl::instance()->run();

	return 0;
}
void cly_schedule::end()
{
	if(!m_brun) return;
	//先停其它线程，因为其它线程可能会请求主线程并等待结果。
	cly_httpapiSngl::instance()->close();
	cly_httpapiSngl::destroy();
	cly_localCmdSngl::instance()->end();
	cly_checkhashSngl::instance()->end();

	m_brun = false;
	wait();
	fini();
}
int cly_schedule::work(int e)
{
	int ret = 0;
	while(m_brun)
	{
		clyd::instance()->handle_root();
		cl_timerSngl::instance()->handle_root();
		cl_reactorSngl::instance()->handle_root();
		UAC_SocketSelectorSngl::instance()->handle_accept();
		ret = UAC_SocketSelectorSngl::instance()->handle_readwrite();

		if(cly_pc->limit_share_threads>0)
		{
			if(-1==ret)
				Sleep(8);
		}
		else
			uac_loop();
	}
	return 0;
}


int cly_schedule::init(cly_config_t* pc)
{
	int ret = 0;
	int sizes[]={1024,CLYP_MAX_BLOCKDATA_SIZE,102400,1024000};

	cl_timerSngl::instance();
	cly_settingSngl::instance()->init(pc);
	cl_memblockPoolSngl::instance()->init(sizes,4);
	cl_reactorSngl::instance(new cl_reactorSelect(64));

	string stun = cl_util::get_string_index(pc->server_stun,0,":");
	unsigned short stun_port = (unsigned short)atoi(cl_util::get_string_index(pc->server_stun,1,":").c_str());
	if(stun_port==0) stun_port = 11800;
	uac_config_t uac_conf;
	uac_conf.limit_sendspeed_i = cly_pc->dconf.limit_share_speediKB<<10;
	uac_conf.limit_min_sendspeed_i = cly_pc->dconf.limit_share_min_speediKB << 10;
	uac_conf.limit_sendspeed = cly_pc->dconf.limit_share_speedKB<<10;
	uac_conf.limit_recvspeed = cly_pc->dconf.limit_down_speedKB<<10;

	uac_conf.callback_onnatok = cly_trackerCmd::on_uac_natok;
	uac_conf.callback_onipportchanged = cly_trackerCmd::on_uac_ipport;
	if (cly_pc->uac_max_sendwin > 0) uac_conf.max_send_win_num = cly_pc->uac_max_sendwin;
	if (cly_pc->uac_sendtimer_ms > 0) uac_conf.sendtimer_ms = cly_pc->uac_sendtimer_ms;
	
#ifdef ANDROID
	//TODO android 版本可能线程锁使用有可能失败导致coredump 
	cly_pc->limit_share_threads = 0;
#endif
	ret = uac_init(pc->uac_port,stun.c_str(),stun_port,&uac_conf, cly_pc->limit_share_threads>0?true:false);
	if(pc->uac_mtu>0) 
		uac_set_mtu(pc->uac_mtu);
	UAC_SocketSelectorSngl::instance();
	cly_peerRecyclingSngl::instance();
	cly_trackerCmdSngl::instance()->init(); //放在这里，filemgr可能会初始化时因文件丢失调用。

	cly_filemgrSngl::instance()->init();
	cly_serverSngl::instance()->init(cly_pc->limit_share_cnns,cly_pc->limit_share_threads);
	cly_downloadMgrSngl::instance()->init();
	cly_downAutoSngl::instance()->Init();

	return ret;
}
void cly_schedule::fini()
{
	cly_trackerCmdSngl::instance()->fini();
	cly_downAutoSngl::instance()->Fini();
	cly_downloadMgrSngl::instance()->fini();
	cly_serverSngl::instance()->fini();
	cly_filemgrSngl::instance()->fini();
	uac_fini();
	cly_peerRecyclingSngl::instance()->clear_pending();
	cl_memblockPoolSngl::instance()->fini();

	cly_trackerCmdSngl::destroy();
	cly_downAutoSngl::destroy();
	cly_localCmdSngl::destroy();
	cly_downloadMgrSngl::destroy();
	cly_checkhashSngl::destroy();
	cly_serverSngl::destroy();
	cly_filemgrSngl::destroy();
	cly_peerRecyclingSngl::destroy();
	UAC_SocketSelectorSngl::destroy();
	cl_reactorSngl::destroy();
	cl_memblockPoolSngl::destroy();
	cly_settingSngl::destroy();
	cl_timerSngl::destroy();
}

