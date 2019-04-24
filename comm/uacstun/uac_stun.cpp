#include "uac_stun.h"
#include "uac_Setting.h"
#include "uac_UDPStunServer.h"

using namespace UAC;

int socket_init()
{
#ifdef _WIN32
	WSADATA wsaData;
	if(0!=WSAStartup(0x202,&wsaData))
	{
		perror("#***WSAStartup false! : ");
		return -1;
	}
	return 0;
#endif
	return 0;
}
void socket_fini()
{
#ifdef _WIN32
	WSACleanup();
#endif
}


bool g_is_init = false;
int stunsvr_init()
{
	if(g_is_init)
	{
		assert(false);
		return 0;
	}
	g_is_init = true;
	socket_init();

	SettingSngl::instance();

	SettingSngl::instance()->init();
	if(0!=StunServer::stun_open(SettingSngl::instance()->get_stunsvr_config()))
	{
		return -1;
	}
	SchedulerSngl::instance()->run();
	return 0;
}

void stunsvr_fini()
{
	if(!g_is_init)
		return;
	g_is_init = false;

	SchedulerSngl::instance()->end();

	StunServer::stun_close();
	SettingSngl::instance()->fini();

	SchedulerSngl::destroy();
	SettingSngl::destroy();
	
	socket_fini();
}

bool check_config()
{
	PTL_STUN_RspStunsvrConfig_t local_config,remote_config;
	if(0!=StunServer::stun_check_config(local_config,remote_config))
	{
		printf("*** check config failed! *** \n");
		return false;
	}
	else
	{
		//stunA为提供给客户的自己信息，必须为stunB所见的一样
		//stunB为对方绑定的
		if((local_config.accept_port1==remote_config.eyePort || local_config.accept_port2==remote_config.eyePort) &&
			local_config.stunB_port1 == remote_config.accept_port1 && local_config.stunB_port2 == remote_config.accept_port2)
		{
			printf(" check config ok ( 与 stunB 握手成功) \n");
			return true;
		}
		printf("*** check config failed! 与 stunB 握手失败) *** \n");
		return false;
	}
}


int main(int argc,char** argv)
{
	if(argc>1 && 0==strcmp(argv[1],"-v"))
	{
		printf("[version] : uac_stun_20170313 \n");
		return 0;
	}
	printf("uac stunsvr start...\n");
	socket_init();
	if(0!=stunsvr_init())
	{
		stunsvr_fini();
		socket_fini();
		return -1;
	}

	Sleep(2000);
	check_config();

	//while('q'!=getchar());
	while(1) Sleep(10000);

	stunsvr_fini();
	socket_fini();
	printf("uac stunsvr stop! \n");
	return 0;
}

//*************************************************
Scheduler::Scheduler(void)
:m_brun(false)
{
}

Scheduler::~Scheduler(void)
{
}
int Scheduler::run()
{
	if(m_brun)
		return 1;
	m_brun = true;
	this->activate();
	return 0;
}
void Scheduler::end()
{
	if(!m_brun)
		return;
	m_brun = false;
	wait();
}
int Scheduler::work(int e)
{
	int ret;
	while(m_brun)
	{
		ret = StunServer::handle_root(0);
		if(-1==ret)
			Sleep(8);
	}
	return 0;
}


