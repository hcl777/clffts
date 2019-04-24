#pragma once
#include "cl_basetypes.h"
#include "cl_incnet.h"

typedef struct tag_ifinfoi
{
	bool isloopback;
	bool isup;
	bool running; //是否插网线
	char name[128];
	char ip[32];
	char mask[32];
	char mac[16];
}ifinfoi_s;

typedef struct tag_ifinfo
{
	int ifcount;
	ifinfoi_s ifs[10];
}ifinfo_s;

class cl_net
{
private:
	cl_net(void);
	~cl_net(void);

public:
	
	static int socket_init();
	static void socket_fini();

	static void sockaddr_format(sockaddr_in& addr,unsigned int nip,unsigned short nport);
	static void sockaddr_format(sockaddr_in& addr,const char* ip,unsigned short port);
	static int sock_set_nonblock(int fd,int nonblock=1); //inoblock=1 表示非阻塞
	static int sock_set_timeout(int fd,int timeo_ms);
	static int sock_set_udp_broadcast(int fd);
	static int sock_set_udp_multicast(int fd,const char* multi_ip);
	static int sock_bind(int fd,unsigned short port);
	static int sock_select_readable(int fd,DWORD dwTimeout = 100);
	static int sock_select_writable(int fd,DWORD dwTimeout = 100);
	static int sock_select_send_n(int fd,const char *buf,int size,int timeoutms);
	static int sock_select_recv_n(int fd,char *buf,int size,int timeoutms);
	static int sock_send_n(int fd,const char *buf,int size);
	static int sock_recv_n(int fd,char *buf,int size);
	static int sock_get_myaddr(int fd,unsigned int& nip,unsigned short& nport);

	static int get_umac(unsigned char umac[]);
	static string get_mac();
	static int get_macall(ifinfo_s *inf);
	static string get_mac_codeid(); //获取mac后通过算法生成新ID。
	static int get_mtu();
	static void print_mac();

	//获取时间服务器的时间
	//获取1830年后的秒数.格林治时间如果转北京时间,要+8小时.
	static unsigned int get_server_time_sec1830_tcp(const char* timesvr);
	static int get_server_time(time_t *t,const char* server="us.ntp.org.cn");

	static bool is_ip(const char* ip);
	static bool is_dev(const char* ip);

	static string ip_explain(const char* s);
	static string ip_explain_ex(const char* s,int maxTick=5000);
	static char* ip_htoa(unsigned int ip);
	static char* ip_ntoa(unsigned int nip);
	static string ip_htoas(unsigned int ip);
	static string ip_ntoas(unsigned int nip);
	static string ip_ntoas(unsigned int nip,unsigned short nport);
	static unsigned int ip_atoh(const char* ip);
	static unsigned int ip_aton(const char* ip);
	static unsigned int ip_atoh_try_explain_ex(const char* s,int maxTick=5000){return  ip_atoh(ip_explain_ex(s,maxTick).c_str());}
	static unsigned int ip_aton_try_explain_ex(const char* s,int maxTick=5000){return  ip_aton(ip_explain_ex(s,maxTick).c_str());}

	static  string get_local_private_ip();
	static  string get_local_private_ip_ex(int timeout_tick=5000);
	static  bool is_private_ip(const string& ip) ;

};

