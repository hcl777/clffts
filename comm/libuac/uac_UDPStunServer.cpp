#include "uac_UDPStunServer.h"
#include "uac_Util.h"

namespace UAC
{

StunServer						*g_stun1 = NULL;
StunServer						*g_stun2 = NULL;

int StunServer::stun_open(const PTL_STUN_RspStunsvrConfig_t& config)
{
	if(NULL!=g_stun1)
		return 1;
	char ip[64];
	strcpy(ip,Util::ip_htoa(config.accept_ip));
	g_stun1 = new StunServer();
	g_stun2 = new StunServer();
	if(0!=g_stun1->open(config.accept_port1,ip,config,g_stun2))
	{
		stun_close();
		return -1;
	}
	if(0!=g_stun2->open(config.accept_port2,ip,config,g_stun1))
	{
		stun_close();
		return -1;
	}
	return 0;
}
void StunServer::stun_close()
{
	if(NULL!=g_stun1)
	{
		g_stun1->close();
		delete g_stun1;
		g_stun1 = NULL;
	}
	if(NULL!=g_stun2)
	{
		g_stun2->close();
		delete g_stun2;
		g_stun2 = NULL;
	}
}
int StunServer::stun_check_config(PTL_STUN_RspStunsvrConfig_t& local_config,PTL_STUN_RspStunsvrConfig_t& remote_config)
{
	if(!g_stun1)
		return -1;
	int n = 0;
	local_config = g_stun1->m_local_config;
	g_stun1->ptl_request_stuns_config();
	Sleep(500);
	while(n++<10)
	{
		if(2==g_stun1->m_get_config_state)
		{
			remote_config = g_stun1->m_remote_config;
			return 0;
		}
		g_stun1->ptl_request_stuns_config();
		Sleep(500);
	}
	return -1;
}
int StunServer::handle_root(unsigned long delay_usec/*=0*/)
{
	int ret1 = g_stun1->handle_select_read(0);
	int ret2 = g_stun2->handle_select_read(0);
	if(-1==ret1 && -1==ret2)
		return -1;
	return 0;
}


//************************************************

StunServer::StunServer(void)
:m_get_config_state(0)
,read_ss(4096)
,write_ss(4096)
{
	reset_cmd_num();
}

StunServer::~StunServer(void)
{
}
int StunServer::open(unsigned short port,const char* ip,const PTL_STUN_RspStunsvrConfig_t& config,StunServer* stun2)
{
	m_local_config = config;
	rsp_stunb.stunB_ip = m_local_config.stunB_ip;
	rsp_stunb.stunB_port1 = m_local_config.stunB_port1;

	memset(&tmp_addr,0,sizeof(tmp_addr));
	memset(&nat1_addr,0,sizeof(nat1_addr));
	tmp_addr.sin_family = AF_INET;
	nat1_addr.sin_family = AF_INET;

	nat1_addr.sin_addr.s_addr = htonl(m_local_config.stunB_ip);
	nat1_addr.sin_port = htons(m_local_config.stunB_port2); //一定要使用port2,因为port1会被检测nat4时发送
	
	m_stun2 = stun2;
	return open_sock(port,ip);
}
void StunServer::close()
{
	close_sock();
}

int StunServer::handle_input()
{
	tmp_recv_addrlen = sizeof(tmp_recv_addr);
	memset(&tmp_recv_addr,0,sizeof(tmp_recv_addr));
	read_ss.zero_rw();
	while(0<(tmp_ret = recvfrom(m_fd,read_ss.buffer(),read_ss.buffer_size(),0,(sockaddr*)&tmp_recv_addr,&tmp_recv_addrlen)))
	{
		read_ss.skipw(tmp_ret);
		//TODO:目前不校验包正确性
		//if(PTL_STUN_HEAD_STX==(read_ss.buffer()[0])
			on_line(read_ss,tmp_recv_addr);

		read_ss.zero_rw();
		tmp_recv_addrlen = sizeof(tmp_recv_addr);
		memset(&tmp_recv_addr,0,sizeof(tmp_recv_addr));
	}
	return 0;
}
void StunServer::on_line(PTLStream& ss,sockaddr_in& addr)
{
	m_cmd_num++;
	if(0!=ss>>head)
	{
		UACLOG("***ERR PACK: %s - %d \n",Util::ip_ntoa(addr.sin_addr.s_addr),ss.tellw());
		return;
	}
	switch(head.cmd)
	{
	case PTL_STUN_REQ_BINDING_ADDR:
		{
			on_ptl_request_binding_addr(addr);
		}
		break;
	case PTL_STUN_REQ_STUNB_ADDR:
		{
			on_ptl_request_stunb_addr(addr);
		}
		break;
	case PTL_STUN_REQ_STUNSVR_CONFIG:
		{
			on_ptl_request_stunsvr_config(addr);
		}
		break;
	case PTL_STUN_RSP_STUNSVR_CONFIG:
		{
			if(0==ss>>rsp_config)
				on_ptl_response_stunsvr_config(addr,rsp_config);
		}
		break;
	case PTL_STUN_REQ_NAT1:
		{
			on_ptl_request_nat1(addr);
		}
		break;
	case PTL_STUN_RSP_NAT1:
		{
			if(0==ss>>rsp_nat1)
				on_ptl_response_nat1(addr,rsp_nat1);
		}
		break;
	case PTL_STUN_REQ_NAT4:
		{
			on_ptl_request_nat4(addr);
		}
		break;
	case PTL_STUN_REQ_NAT2:
		{
			on_ptl_request_nat2(addr);
		}
		break;
	case PTL_STUN_REQ_HOLE:
		{
			if(0==ss>>req_hole)
				on_ptl_request_hole(addr,req_hole);
		}
		break;
	case PTL_STUN_RSP_HOLE:
		{
			if(0==ss>>rsp_hole)
				on_ptl_response_hole(addr,rsp_hole);
		}
		break;
	case PTL_STUN_REPORT_NATTYPE:
		{
			if(0==ss>>inf_nattype)
				on_ptl_report_nattype(addr,inf_nattype);
		}
		break;
	case PTL_STUN_REPORT_CONN_SF:
		{
			if(0==ss>>inf_connsf)
				on_ptl_report_connsf(addr,inf_connsf);
		}
		break;
	case PTL_STUN_REPORT_CONN_STAT:
		{
			if(0==ss>>inf_connstat)
				on_ptl_report_connstat(addr,inf_connstat);
		}
		break;
	default:
		m_wrong_cmd_num++;
		UACLOG("#*** StunServer unkown packet cmd=%d (%s:%d) \n",(unsigned int)head.cmd,inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
		break;
	}
	if(0!=ss.length())
	{
		UACLOG("#*** StunServer unkown packet cmd=%d (%s:%d) \n",(unsigned int)head.cmd,inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
	}
	assert(0==ss.length());
}

void StunServer::on_ptl_request_binding_addr(sockaddr_in& addr)
{
	//UACLOG("#P-%d : on_ptl_request_binding_addr.\n",m_hport);
	rsp_binding.eyeIP = ntohl(addr.sin_addr.s_addr);
	rsp_binding.eyePort = ntohs(addr.sin_port);
	ptl_stun_send_packet_T2(m_fd,PTL_STUN_RSP_BINDING_ADDR,rsp_binding,addr,head,write_ss);
}
void StunServer::on_ptl_request_stunb_addr(sockaddr_in& addr)
{
	UACLOG("#P-%d : on_ptl_request_stunb_addr.\n",m_hport);
	ptl_stun_send_packet_T2(m_fd,PTL_STUN_RSP_STUNB_ADDR,rsp_stunb,addr,head,write_ss);
}
void StunServer::on_ptl_request_stunsvr_config(const sockaddr_in& addr)
{
	UACLOG("#P-%d : on_ptl_request_stunsvr_config.\n",m_hport);
	m_local_config.eyeIP = ntohl(addr.sin_addr.s_addr);
	m_local_config.eyePort = ntohs(addr.sin_port);
	ptl_stun_send_packet_T2(m_fd,PTL_STUN_RSP_STUNSVR_CONFIG,m_local_config,addr,head,write_ss);

}
void StunServer::on_ptl_response_stunsvr_config(const sockaddr_in& addr,const PTL_STUN_RspStunsvrConfig_t& rsp)
{
	UACLOG("#P-%d : on_ptl_response_stunsvr_config.\n",m_hport);
	if(1==m_get_config_state)
	{
		m_remote_config = rsp;
		m_get_config_state = 2;
	}
}

void StunServer::on_ptl_request_nat1(const sockaddr_in& addr)
{
	UACLOG("#P-%d : on_ptl_request_nat1.\n",m_hport);
	rsp_nat1.des_nip = (uint32)addr.sin_addr.s_addr;
	rsp_nat1.des_nport = (uint16)addr.sin_port;
	//转发给stunB2
	ptl_stun_send_packet_T2(m_fd,PTL_STUN_RSP_NAT1,rsp_nat1,nat1_addr,head,write_ss);
}
void StunServer::on_ptl_response_nat1(const sockaddr_in& addr,const PTL_STUN_RspNat1_t& rsp)
{
	UACLOG("#P-%d : on_ptl_response_nat1.(%s,%d)\n",m_hport,Util::ip_ntoa(rsp.des_nip),ntohs(rsp.des_nport));
	tmp_addr.sin_addr.s_addr = rsp.des_nip;
	tmp_addr.sin_port = rsp.des_nport;
	ptl_stun_send_packet_T2(m_fd,PTL_STUN_RSP_NAT1,rsp,tmp_addr,head,write_ss);
}
void StunServer::on_ptl_request_nat4(const sockaddr_in& addr)
{
	UACLOG("#P-%d : on_ptl_request_nat4.\n",m_hport);
	rsp_nat4.eyeIP = ntohl(addr.sin_addr.s_addr);
	rsp_nat4.eyePort = ntohs(addr.sin_port);
	ptl_stun_send_packet_T2(m_fd,PTL_STUN_RSP_NAT4,rsp_nat4,addr,head,write_ss);
}

void StunServer::on_ptl_request_nat2(const sockaddr_in& addr)
{
	UACLOG("#P-%d : on_ptl_request_nat2.\n",m_hport);
	rsp_nat2.des_nip = (uint32)addr.sin_addr.s_addr;
	rsp_nat2.des_nport = (uint16)addr.sin_port;
	ptl_stun_send_packet_T2(m_stun2->m_fd,PTL_STUN_RSP_NAT2,rsp_nat2,addr,head,write_ss);
}

void StunServer::on_ptl_request_hole(const sockaddr_in& addr,PTL_STUN_ReqHole_t& req)
{
	UACLOG("#P-%d : on_ptl_request_hole.(%s,%d)\n",m_hport,Util::ip_ntoa(req.des_nip),ntohs(req.des_nport));
	tmp_addr.sin_addr.s_addr = req.des_nip;
	tmp_addr.sin_port = req.des_nport;
	//发起方的自己IP信息不一定是公网的，在此更改以便准确回复
	req.src_nip = addr.sin_addr.s_addr;
	req.src_nport = addr.sin_port;
	ptl_stun_send_packet_T2(m_fd,PTL_STUN_REQ_HOLE,req,tmp_addr,head,write_ss);
}

void StunServer::on_ptl_response_hole(const sockaddr_in& addr,const PTL_STUN_RspHole_t& req)
{
	UACLOG("#P-%d : on_ptl_response_hole.(%s,%d)\n",m_hport,Util::ip_ntoa(req.des_nip),ntohs(req.des_nport));
	tmp_addr.sin_addr.s_addr = req.des_nip;
	tmp_addr.sin_port = req.des_nport;
	ptl_stun_send_packet_T2(m_fd,PTL_STUN_RSP_HOLE,req,tmp_addr,head,write_ss);
}

void StunServer::on_ptl_report_nattype(const sockaddr_in& addr,const PTL_STUN_ReportNattype_t& inf)
{
	sprintf(tmp_buf,"%d-%s:%d",inf.nattype,inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
	Util::write_debug_log(tmp_buf,"nattype.log");
}
void StunServer::on_ptl_report_connsf(const sockaddr_in& addr,const PTL_STUN_ReportConnSF_t& inf)
{
	sprintf(tmp_buf,"%d-%s:%d ==> %d-%s:%d %s",(int)inf.src_nattype,inet_ntoa(addr.sin_addr),(int)ntohs(addr.sin_port)
		,(int)inf.des_nattype,Util::ip_ntoa(inf.des_nip),(int)ntohs(inf.des_nport),inf.bsucceed?"":" --fail");
	Util::write_debug_log(tmp_buf,"connsf.log");
}
void StunServer::on_ptl_report_connstat(const sockaddr_in& addr,const PTL_STUN_ReportConnStat_t& inf)
{
	unsigned int spKB = inf.sizeKB;
	unsigned int rate = 100;
	if(inf.sec) spKB/=inf.sec;
	if(inf.send_num) rate=inf.resend_num*100/inf.send_num;
	sprintf(tmp_buf,"%d-%s:%d ==> %d-%s:%d, sKB=%d,sec=%d,spKB=%d,ttlMS=%d,rr/trs/rs/sn=%d/%d/%d/%d [%d%%]",
		(int)inf.src_nattype,inet_ntoa(addr.sin_addr),(int)ntohs(addr.sin_port)
		,(int)inf.des_nattype,Util::ip_ntoa(inf.des_nip),(int)ntohs(inf.des_nport)
		,inf.sizeKB,inf.sec,spKB,inf.ttlMS,inf.other_rerecv_num,inf.resend_timeo_num,inf.resend_num,inf.send_num,rate);
	Util::write_debug_log(tmp_buf,"connstat.log");
}

int StunServer::ptl_request_stuns_config()
{
	UACLOG("#P-%d : ptl_request_stuns_config.\n",m_hport);
	m_get_config_state = 1;
	ptl_stun_send_packet2(m_fd,PTL_STUN_REQ_STUNSVR_CONFIG,nat1_addr,head,write_ss);
	return 0;
}

}

