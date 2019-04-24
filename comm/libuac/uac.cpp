#include "uac.h"
#include "uac_mempool.h"
#include "uac_sockpool.h"
#include "uac_Scheduler.h"
#include "uac_Timer.h"
#include "uac_statistics.h"

using namespace UAC;

//#ifdef __cplusplus
//extern "C"
//{
//#endif

bool g_buac_init = false;
#define UNINIT_RETURNE(i) if(!g_buac_init) return i


//setting
void uac_setcallback_onnatok(UAC_CALLBACK_ONNATOK fun)
{
	g_uac_conf.callback_onnatok = fun;
}
void uac_setcallback_onipportchanged(UAC_CALLBACK_ONIPPORTCHANGED fun)
{
	g_uac_conf.callback_onipportchanged = fun;
}
int uac_get_nattype()
{
	return g_uac_conf.nattype;
}
int uac_init(unsigned short bindport,const char* stunsvr,unsigned short stunport,uac_config_t* conf, bool runthread/* = true*/)
{
	if(g_buac_init)
		return 0;

	g_buac_init = true;
	if(conf)
		g_uac_conf = *conf;

	TimerSngl::instance();
	mempoolsngl::instance()->init();
	statisticsSngl::instance()->init();
	if(0!=sockpoolsngl::instance()->init(bindport,stunsvr,stunport))
	{
		uac_fini();
		return -1;
	}

	if(runthread)
		SchedulerSngl::instance()->run();
	return 0;
}
int uac_fini()
{
	if(!g_buac_init)
		return 0;
	g_buac_init = false;

	SchedulerSngl::instance()->end();
	sockpoolsngl::instance()->fini();
	statisticsSngl::instance()->fini();
	mempoolsngl::instance()->fini();

	SchedulerSngl::destroy();
	sockpoolsngl::destroy();
	statisticsSngl::destroy();
	mempoolsngl::destroy();
	TimerSngl::destroy();
	return 0;
}
void uac_loop()
{
	ULONGLONG delay_usec = 0;
	TimerSngl::instance()->handle_root();

	delay_usec = TimerSngl::instance()->get_remain_us();
	delay_usec = delay_usec / 2;
	if (delay_usec>10000) delay_usec = 10000;

	sockpoolsngl::instance()->handle_root((long)delay_usec);
}
int uac_set_mtu(unsigned int m)
{
	UNINIT_RETURNE(-1);
	return sockpoolsngl::instance()->set_mtu(m);
}

int uac_get_conf(uac_config_t* conf)
{
	*conf = g_uac_conf;
	return 0;
}
int uac_set_conf(uac_config_t* conf)
{
	if(conf)
		g_uac_conf = *conf;
	return 0;
}
int uac_get_statspeed(UAC_statspeed_t& ss)
{
	UNINIT_RETURNE(-1);
	return statisticsSngl::instance()->get_statspeed(ss);
}
UAC_SOCKET uac_accept(UAC_sockaddr* sa_client)
{
	UNINIT_RETURNE(-1);
	return sockpoolsngl::instance()->accept(sa_client);
}
UAC_SOCKET uac_connect(const UAC_sockaddr* sa_client)
{
	UNINIT_RETURNE(-1);
	return sockpoolsngl::instance()->connect(sa_client);
}
int uac_closesocket(UAC_SOCKET fd)
{
	UNINIT_RETURNE(-1);
	return sockpoolsngl::instance()->close_socket(fd);
}
int uac_setbandwidth(UAC_SOCKET fd,int size)
{
	UNINIT_RETURNE(-1);
	return sockpoolsngl::instance()->setbandwidth(fd,size);
}
int uac_setsendbuf(UAC_SOCKET fd,int size)
{
	UNINIT_RETURNE(-1);
	return sockpoolsngl::instance()->setsendbuf(fd,size);
}
int uac_setrecvbuf(UAC_SOCKET fd,int size)
{
	UNINIT_RETURNE(-1);
	return sockpoolsngl::instance()->setrecvbuf(fd,size);
}
UAC_sendspeed_t& uac_getsendspeed(UAC_SOCKET fd)
{
	//UNINIT_RETURNE(-1);
	return sockpoolsngl::instance()->get_sendspeed(fd);
}
UAC_recvspeed_t& uac_getrecvspeed(UAC_SOCKET fd)
{
	//UNINIT_RETURNE(-1);
	return sockpoolsngl::instance()->get_recvspeed(fd);
}
bool uac_is_read(UAC_SOCKET fd)
{
	UNINIT_RETURNE(false);
	return sockpoolsngl::instance()->is_read(fd);
}
bool uac_is_write(UAC_SOCKET fd)
{
	UNINIT_RETURNE(false);
	return sockpoolsngl::instance()->is_write(fd);
}
bool uac_is_connected(UAC_SOCKET fd)
{
	UNINIT_RETURNE(false);
	return sockpoolsngl::instance()->is_connected(fd);
}
int uac_select(UAC_fd_set* rset,UAC_fd_set* wset)
{
	UNINIT_RETURNE(-1);
	return sockpoolsngl::instance()->select(rset,wset);
}
int uac_send(UAC_SOCKET fd,const char* buf,int len)
{
	UNINIT_RETURNE(-1);
	return sockpoolsngl::instance()->send(fd,buf,len);
}
int uac_recv(UAC_SOCKET fd,char* buf,int len)
{
	UNINIT_RETURNE(-1);
	return sockpoolsngl::instance()->recv(fd,buf,len);
}



//#ifdef __cplusplus
//}
//#endif


