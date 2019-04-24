#pragma once
#include "cl_timer.h"
#include "cl_thread.h"
#include "cl_util.h"
#include "cl_net.h"

typedef struct tagStunsvrConfig
{
	char stunA_ipstr[64];
	unsigned int stunA_ip;
	unsigned short stunA_port1;
	unsigned int stunB_ip;
	unsigned short stunB_port1;
}StunsvrConfig_t;
class uac_stunc_test: public cl_thread
	,public cl_timerHandler
{
public:
	uac_stunc_test();
	virtual ~uac_stunc_test();
	enum { REQS_FINI = 0, REQS_STUNB_ADDR, REQS_NAT1, REQS_NAT4, REQS_NAT2 };
public:
	int start(const char* stunsvr, unsigned short port);
	void end();
	virtual int work(int e);
	virtual void on_timer(int e);

private:
	void on_nat_ok(int nat_type);
	int send_nat_request();
	void on_data(char* buf, int size, sockaddr_in& addr);

	int ptl_request_binding_addr();
	int ptl_request_stunb_addr();
	int ptl_request_nat1();
	int ptl_request_nat4();
	int ptl_request_nat2();
private:
	bool m_binit;
	int m_fd;
	StunsvrConfig_t m_si;

	int m_state;        //记录该发什么包
	int m_resend_count; //记录当前要发的包已经重复了多少次
	int m_nat4_port1, m_nat4_port2;
	//unsigned int m_last_check_nat_tick;
};

