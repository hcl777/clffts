#include "uac_stunc_test.h"
#include "cl_socketbase.h"
#include "uac_UDPStunProtocol.h"
using namespace UAC;

uac_stunc_test::uac_stunc_test()
	:m_binit(false)
{
}


uac_stunc_test::~uac_stunc_test()
{
}

int uac_stunc_test::start(const char* stunsvr,unsigned short port)
{
	if (0 != cl_socketbase::open_udp_sock(m_fd, htons(port), INADDR_ANY, 10240, 10240))
		return -1;
	cl_net::sock_set_nonblock(m_fd);

	m_binit = true;
	string s = stunsvr;
	m_si.stunA_ip = cl_net::ip_atoh_try_explain_ex(cl_util::get_string_index(s, 0, ":").c_str());
	m_si.stunA_port1 = cl_util::atoi(cl_util::get_string_index(s, 1, ":").c_str());
	printf("begin check nat:\n stunsvr : %s(%s:%d)\n", stunsvr, cl_net::ip_htoas(m_si.stunA_ip).c_str(), (int)m_si.stunA_port1);

	ptl_request_binding_addr();
	Sleep(200);
	//nat begin
	m_state = REQS_STUNB_ADDR;
	m_resend_count = 0;
	m_nat4_port1 = 0;
	m_nat4_port2 = 0;
	cl_timerSngl::instance()->register_timer(this, 1, 600);

	activate(1);

	return 0;
}
void uac_stunc_test::end()
{
	m_binit = false;
	wait();
	cl_timerSngl::instance()->unregister_all(this);
}
int uac_stunc_test::work(int e)
{
	char buf[4096];
	int n;
	sockaddr_in from;
	socklen_t fromlen;
	fromlen = sizeof(sockaddr_in);
	memset((void*)&from, 0, fromlen);
	while (m_binit)
	{
		cl_timerSngl::instance()->handle_root();
		if (cl_net::sock_select_readable(m_fd))
		{
			while ((n = recvfrom(m_fd, buf, 4096, 0, (sockaddr*)&from, &fromlen)) > 0)
				on_data(buf, n, from);
		}
	}
	return 0;
}

void uac_stunc_test::on_timer(int e)
{
	switch (e)
	{

	case 1:
		send_nat_request();
		break;
	case 2:
	{
	}
	break;
	default:
		assert(false);
		break;
	}
}
void uac_stunc_test::on_nat_ok(int nat_type)
{
	//不是初次检查，而且检查到是5型，不更新
	if (REQS_FINI == m_state)
		return;
	m_state = REQS_FINI;
	m_resend_count = 0;
	printf("nat_type=%d\n", nat_type);
	cl_timerSngl::instance()->unregister_timer(this, 1);
}
int uac_stunc_test::send_nat_request()
{
	switch (m_state)
	{
	case REQS_STUNB_ADDR:
	{
		if (m_resend_count >= 4)
		{
			on_nat_ok(5);
			return 0;
		}
		m_resend_count++;
		return ptl_request_stunb_addr();
	}
	break;
	case REQS_NAT1:
	{
		if (m_resend_count >= 4)
		{
			m_resend_count = 0;
			m_state = REQS_NAT4;
			return send_nat_request();
		}
		m_resend_count++;
		return ptl_request_nat1();
	}
	break;
	case REQS_NAT4:
	{
		if (m_resend_count >= 4)
		{
			on_nat_ok(5);
			return 0;
		}
		m_resend_count++;
		return ptl_request_nat4();
	}
	break;
	case REQS_NAT2:
	{
		if (m_resend_count >= 4)
		{
			on_nat_ok(3);
			return 0;
		}
		m_resend_count++;
		return ptl_request_nat2();
	}
	break;
	default:
		assert(0);
		break;
	}
	return -1;
}
void uac_stunc_test::on_data(char* buf, int size, sockaddr_in& addr)
{
	PTL_STUN_Header_t head;
	PTLStream ss(buf, size, size);
	if (0 != ss >> head)
		return;

	switch (head.cmd)
	{
	case PTL_STUN_RSP_BINDING_ADDR:
	{
		PTL_STUN_RspBindingAddr_t rsp;
		if (0 == (ss >> rsp))
		{
			//在此可以检查是否为一个本地IP
			printf("rsp bind addr: %s:%d \n", cl_net::ip_htoas(rsp.eyeIP).c_str(), (int)rsp.eyePort);
		}
	}
	break;
	case PTL_STUN_RSP_STUNB_ADDR:
	{
		PTL_STUN_RspStunBAddr_t rsp;
		if (0 == ss >> rsp)
		{
			m_si.stunB_ip = rsp.stunB_ip;
			m_si.stunB_port1 = rsp.stunB_port1;
			string ipa = cl_net::ip_htoas(m_si.stunA_ip);
			string ipb = cl_net::ip_htoas(m_si.stunB_ip);
			printf("rsp stunb addr: stunA(%s:%d),stunB(%s:%d)\n"
				, ipa.c_str(), m_si.stunA_port1
				, ipb.c_str(), m_si.stunB_port1);
			m_resend_count = 0;
			m_state = REQS_NAT1;
		}
	}
	break;
	case PTL_STUN_RSP_NAT1:
	{
		//uint32 ip = ntohl(addr.sin_addr.s_addr);
		//uint16 port = ntohs(addr.sin_port);
		PTL_STUN_RspNat1_t rsp;
		if (0 != ss >> rsp)
			break;
		//可能stunC回复的情况
		printf("rsp nat1\n");
		on_nat_ok(1);
	}
	break;
	case PTL_STUN_RSP_NAT4:
	{
		//目前发现虽然可能两stun 返回的端口是一样. 但再向其它IP:PORT发送则端口不一样了.即会将NAT4误判为NAT3
		uint32 ip = ntohl(addr.sin_addr.s_addr);
		uint16 port = ntohs(addr.sin_port);
		PTL_STUN_RspNat4_t rsp;
		if (0 != ss >> rsp)
			break;
		if (ip == m_si.stunA_ip && port == m_si.stunA_port1)
		{
			printf("rsp nat4 ->1: port=%d\n", rsp.eyePort);
			m_nat4_port1 = rsp.eyePort;
		}
		if (ip == m_si.stunB_ip && port == m_si.stunB_port1)
		{
			printf("rsp nat4 ->2: port=%d\n", rsp.eyePort);
			m_nat4_port2 = rsp.eyePort;
		}
		if (m_nat4_port1 && m_nat4_port2)
		{
			if (m_nat4_port1 != m_nat4_port2)
			{
				on_nat_ok(4);
			}
			else
			{
				m_resend_count = 0;
				m_state = REQS_NAT2;
			}
		}
	}
	break;
	case PTL_STUN_RSP_NAT2:
	{
		printf("rsp nat2\n");
		uint32 ip = ntohl(addr.sin_addr.s_addr);
		//uint16 port = ntohs(addr.sin_port);
		PTL_STUN_RspNat2_t rsp;
		if (0 != ss >> rsp)
			break;
		if (ip == m_si.stunA_ip)
		{
			on_nat_ok(2);
		}
	}
	break;

	default:
		assert(0);
		break;
	}
	assert(0 == ss.length());
}
int uac_stunc_test::ptl_request_binding_addr()
{
	printf("# UDPStunClient::ptl_request_binding_addr() \n");
	return ptl_stun_send_packet3(m_fd, PTL_STUN_REQ_BINDING_ADDR, m_si.stunA_ip, m_si.stunA_port1);
}
int uac_stunc_test::ptl_request_stunb_addr()
{
	printf("# UDPStunClient::ptl_request_stunb_addr() \n");
	return ptl_stun_send_packet3(m_fd, PTL_STUN_REQ_STUNB_ADDR, m_si.stunA_ip, m_si.stunA_port1);
}
int uac_stunc_test::ptl_request_nat1()
{
	printf("# UDPStunClient::ptl_request_nat1() \n");
	return ptl_stun_send_packet3(m_fd, PTL_STUN_REQ_NAT1, m_si.stunA_ip, m_si.stunA_port1);
}

int uac_stunc_test::ptl_request_nat4()
{
	if (0 == m_nat4_port1)
	{
		printf("# UDPStunClient::ptl_request_nat4() 1 \n");
		ptl_stun_send_packet3(m_fd, PTL_STUN_REQ_NAT4, m_si.stunA_ip, m_si.stunA_port1);
	}
	if (0 == m_nat4_port2)
	{
		printf("# UDPStunClient::ptl_request_nat4() 2 \n");
		ptl_stun_send_packet3(m_fd, PTL_STUN_REQ_NAT4, m_si.stunB_ip, m_si.stunB_port1);
	}
	return 0;
}
int uac_stunc_test::ptl_request_nat2()
{
	printf("# UDPStunClient::ptl_request_nat2() \n");
	return ptl_stun_send_packet3(m_fd, PTL_STUN_REQ_NAT2, m_si.stunA_ip, m_si.stunA_port1);
}

