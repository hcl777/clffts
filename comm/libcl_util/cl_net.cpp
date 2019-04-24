#include "cl_net.h"

#include <assert.h>
#include <stdio.h>

//#include "cl_incnet.h"
#include "cl_bstream.h"

#ifdef _WIN32
	#pragma warning(disable:4996)
	#include <Iphlpapi.h>
	//#pragma comment(lib,"Iphlpapi.lib")
	#include "ws2ipdef.h"
#else
	#include <signal.h>
	#include <pthread.h>
#endif

#include "cl_DIPCache.h"

cl_net::cl_net(void)
{
}

cl_net::~cl_net(void)
{
}

int cl_net::socket_init()
{
#ifdef _WIN32
	WSADATA wsaData;
	if(0!=WSAStartup(0x202,&wsaData))
	{
		perror("WSAStartup false! : ");
		return -1;
	}
	return 0;
#else
	signal(SIGPIPE, SIG_IGN); //忽略Broken pipe,否则socket对端关闭时，很容易写会出现Broken pipe（管道破裂）
#endif
	return 0;
}
void cl_net::socket_fini()
{
#ifdef _WIN32
	WSACleanup();
#endif
}
void cl_net::sockaddr_format(sockaddr_in& addr,unsigned int nip,unsigned short nport)
{
	memset(&addr,0,sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = nport;
	addr.sin_addr.s_addr = nip;
}
void cl_net::sockaddr_format(sockaddr_in& addr,const char* ip,unsigned short port)
{
	sockaddr_format(addr,ip_aton_try_explain_ex(ip),htons(port));
}
int cl_net::sock_set_nonblock(int fd,int nonblock/*=1*/)
{
#ifdef _WIN32
	//NONBLOCKING=1
	u_long val = nonblock;
	if(INVALID_SOCKET!=fd)
		return ioctlsocket(fd,FIONBIO,&val);
	return -1;
#elif defined(_ECOS_8203)
	int val = nonblock;
	return ioctl(fd,FIONBIO,&val);
#else
	int opts;
	opts = fcntl(fd,F_GETFL);
	if(-1 == opts)
	{
		perror("fcntl(fd,GETFL)");
		return -1;
	}
	if(nonblock)
		opts |= O_NONBLOCK;
	else
		opts &= ~O_NONBLOCK;
	if(-1 == fcntl(fd,F_SETFL,opts))
	{
		perror("fcntl(fd,SETFL,opts); ");
		return -1;
	}
	return 0;
#endif
	
}
int cl_net::sock_set_timeout(int fd,int timeo_ms)
{
	//设置超时:
	int ret;
#ifdef _WIN32
	int x = timeo_ms;
#else
	struct timeval x;  
	x.tv_sec = timeo_ms/1000;
	x.tv_usec = (timeo_ms%1000) * 1000;
#endif
	if(0!=(ret=setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,(char*)&x,sizeof(x))))
	{
		perror("setsockopt SO_RCVTIMEO");
	}
	if(0!=(ret=setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,(char*)&x,sizeof(x))))
	{
		perror("setsockopt SO_SNDTIMEO");
	}
	return ret;
}
int cl_net::sock_set_udp_broadcast(int fd)
{
#ifdef _WIN32
	bool isbroadcast = true;
#else
	int isbroadcast = 1;
#endif
	int ret = 0;
	if(0!=(ret=setsockopt(fd,SOL_SOCKET,SO_BROADCAST,(const char*)&isbroadcast,sizeof(isbroadcast))))
	{
		perror("*** setsockopt(SO_BROADCAST): ");
	}
	return ret;
}
int cl_net::sock_set_udp_multicast(int fd,const char* multi_ip)
{
	struct ip_mreq mreq; 
	mreq.imr_multiaddr.s_addr=inet_addr(multi_ip);    
	mreq.imr_interface.s_addr=INADDR_ANY;//htonl(INADDR_ANY);    
	if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,(const char*)&mreq,sizeof(mreq)) < 0)     
	{    
		perror("setsockopt"); 
		return -1; 
	}
	return 0;

}
int cl_net::sock_bind(int fd,unsigned short port)
{
	sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	return ::bind(fd, (sockaddr *)&addr, sizeof(addr));
}
int cl_net::sock_select_readable(int fd,DWORD dwTimeout/* = 100*/)
{
	//assert(fd != INVALID_SOCKET);

	timeval timeout;
	timeout.tv_sec = dwTimeout / 1000;
	timeout.tv_usec = dwTimeout % 1000;
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	int n = ::select(fd+1, &fds, NULL, NULL, &timeout);
	if (n>0)
	{
		if(FD_ISSET(fd,&fds))
			return 1;
		return 0;
	}
	return -1;
}
int cl_net::sock_select_writable(int fd,DWORD dwTimeout/* = 100*/)
{
	//assert(fd != INVALID_SOCKET);

	timeval timeout;
	timeout.tv_sec = dwTimeout / 1000;
	timeout.tv_usec = dwTimeout % 1000;
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	int nStatus = ::select(fd+1, NULL, &fds, NULL, &timeout);
	if (nStatus != SOCKET_ERROR)
	{
		if(FD_ISSET(fd,&fds))
			return 1;
		return 0;
	}
	return -1;
}
int cl_net::sock_select_send_n(int fd,const char *buf,int size,int timeoutms)
{
	int pos=0;
	int ret=0;
	DWORD begintick = GetTickCount();
	while(pos<size)
	{
		
		if(1==sock_select_writable(fd))
		{
			ret = send(fd,buf+pos,size-pos,0);
			if(ret>0)
			{
				pos += ret;
			}
			else
			{
				return -1;
			}
		}
		//超时 10秒
		if(_timer_after(GetTickCount(),(begintick + timeoutms)))
			break;
	}
	return pos==size?0:-1;
}
int cl_net::sock_select_recv_n(int fd,char *buf,int size,int timeoutms)
{
	int pos=0;
	int ret=0;
	DWORD begintick = GetTickCount();
	while(pos<size)
	{
		if(1==sock_select_readable(fd))
		{
			ret = recv(fd,buf+pos,size-pos,0);
			if(ret>0)
			{
				pos += ret;
			}
			else
			{
				return -1;
			}
		}
		//超时 10秒
		if(_timer_after(GetTickCount(),(begintick + timeoutms)))
			break;
	}
	return pos == size ? 0 : -1;
}

int cl_net::sock_send_n(int fd,const char *buf,int size)
{
	int pos=0;
	int ret=0;
	while(pos<size)
	{
		ret = send(fd,buf+pos,size-pos,0);
		if(ret>0)
		{
			pos += ret;
		}
		else
		{
			//int err = WSAGetLastError();
			return -1;
		}
	}
	return 0;
}

int cl_net::sock_recv_n(int fd,char *buf,int size)
{
	int pos=0;
	int ret=0;
	while(pos<size)
	{
		ret = recv(fd,buf+pos,size-pos,0);
		if(ret>0)
		{
			pos += ret;
		}
		else
		{
			return -1;
		}
	}
	return 0;
}
int cl_net::sock_get_myaddr(int fd,unsigned int& nip,unsigned short& nport)
{
	sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	socklen_t len = sizeof(addr);

	//只有connect或者bind的才可以获取成功
	if(0!=getsockname(fd,(sockaddr*)&addr,&len))
		return -1;
	nip = addr.sin_addr.s_addr;
	nport = addr.sin_port;
	return 0;
}
////*************************************************
////umac参数必须大于6个字节
//int get_mac_by_ifname(const char *ifname,unsigned char umac[])
//{
//#ifdef _WIN32
//	return -1;
//#else
//	int sock,ret;
//	struct ifreq ifr;
//	sock = socket(AF_INET,SOCK_STREAM,0);
//	if(sock<0)
//	{
//		perror("socket()");
//		return -1;
//	}
//	memset(&ifr,0,sizeof(ifr));
//	strcpy(ifr.ifr_name,ifname);
//	ret = ioctl(sock,SIOCGIFHWADDR,&ifr,sizeof(ifr));
//	close(sock);
//	if(0==ret)
//	{
//		memcpy(umac,ifr.ifr_hwaddr.sa_data,6);
//	}
//	else
//	{
//		perror("ioctl()");
//	}
//	return ret;
//#endif
//}
int cl_net::get_umac(unsigned char umac[])
{
	unsigned int iumac[6];
	string mac = get_mac();
	sscanf(mac.c_str(),"%02X%02X%02X%02X%02X%02X", 
		&iumac[0], &iumac[1], &iumac[2], &iumac[3], &iumac[4], &iumac[5]);
	for(int i=0;i<6;++i)
		umac[i] = (unsigned char) iumac[i];
	return 0;
}

string cl_net::get_mac()
{
	ifinfo_s inf;
	inf.ifcount = 10;
	int i=0;
	string name = "~"; //126
	string mac = "000000000000";
	if(get_macall(&inf)>0)
	{
		//返回在工作 的 && !lookback && 字符串最小的那个
		for(i=0;i<inf.ifcount;++i)
		{
			if(inf.ifs[i].isup && !inf.ifs[i].isloopback && strcmp(inf.ifs[i].name,name.c_str())<0)
			{
				mac = inf.ifs[i].mac;
				name = inf.ifs[i].name;
			}
		}
		//返回!lookback
		if(mac=="000000000000")
		{
			for(i=0;i<inf.ifcount;++i)
			{
				if(!inf.ifs[i].isloopback && 0!=strcmp(inf.ifs[i].mac,"000000000000"))
				{
					mac = inf.ifs[i].mac;
					break;
				}
			}
		}
	}
	return mac;
}
int cl_net::get_macall(ifinfo_s *inf)
{
#ifdef _WIN32
	int i = 0;
	int n = inf->ifcount;
	inf->ifcount = 0;
	IP_ADAPTER_INFO AdapterInfo[16];       // Allocate information 
	DWORD dwBufLen = sizeof(AdapterInfo);  // Save memory size of buffer
	DWORD dwStatus = GetAdaptersInfo(AdapterInfo,&dwBufLen);
	if(dwStatus != ERROR_SUCCESS)
		return -1;
	//memcpy(umac,AdapterInfo->Address,6);
	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo; // Contains pointer to
	do
	{
		strcpy(inf->ifs[i].name,pAdapterInfo->AdapterName);
		sprintf(inf->ifs[i].mac,"%02X%02X%02X%02X%02X%02X", 
			pAdapterInfo->Address[0], 
			pAdapterInfo->Address[1], 
			pAdapterInfo->Address[2], 
			pAdapterInfo->Address[3], 
			pAdapterInfo->Address[4], 
			pAdapterInfo->Address[5]); 
		inf->ifs[i].isup = true;
		inf->ifs[i].running = true;
		if(MIB_IF_TYPE_LOOPBACK==AdapterInfo->Type)
			inf->ifs[i].isloopback = true;
		else
			inf->ifs[i].isloopback = false;
		strcpy(inf->ifs[i].ip,pAdapterInfo->IpAddressList.IpAddress.String);
		strcpy(inf->ifs[i].mask,pAdapterInfo->IpAddressList.IpMask.String);


		i = ++inf->ifcount;
		if(inf->ifcount>=n) break;
	}while(NULL!=(pAdapterInfo=pAdapterInfo->Next));
	return inf->ifcount;
#else
	int fd;
	struct ifconf conf;
	struct ifreq *ifr;
	sockaddr_in *sin;
	int i;
	char buff[2048];

	struct ifreq ifr2;
	

	conf.ifc_len = 2048;
	conf.ifc_buf = buff;
	fd = socket(AF_INET,SOCK_DGRAM,0);
	ioctl(fd,SIOCGIFCONF,&conf); //SIOCGIFCONF只能获取IP层的信息，所以没设置IP的网卡获取不到

	inf->ifcount = conf.ifc_len / sizeof(struct ifreq);
	ifr = conf.ifc_req;
	if(inf->ifcount>10) inf->ifcount = 10;
	for(i=0;i < inf->ifcount;++i)
	{
		sin = (struct sockaddr_in*)(&ifr->ifr_addr);
		ioctl(fd,SIOCGIFFLAGS,ifr);
		strcpy(inf->ifs[i].name,ifr->ifr_name);
		strcpy(inf->ifs[i].ip,inet_ntoa(sin->sin_addr));
		if(ifr->ifr_flags & IFF_LOOPBACK) 
			inf->ifs[i].isloopback = true;
		else
			inf->ifs[i].isloopback = false;
		if(ifr->ifr_flags & IFF_UP)
			inf->ifs[i].isup = true;
		else
			inf->ifs[i].isup = false;
		if(ifr->ifr_flags & IFF_RUNNING)
			inf->ifs[i].running = true;
		else
			inf->ifs[i].running = false;

		ifr++;
	}

	for(i=0;i < inf->ifcount;++i)
	{
		memset(inf->ifs[i].mac,0,16);
		memset(&ifr2,0,sizeof(ifr2));
		strcpy(ifr2.ifr_name,inf->ifs[i].name);
#ifndef _OS
		if(0==ioctl(fd,SIOCGIFHWADDR,&ifr2,sizeof(struct ifreq)))
		{
			unsigned char* umac = (unsigned char*)ifr2.ifr_hwaddr.sa_data;
			sprintf(inf->ifs[i].mac,"%02X%02X%02X%02X%02X%02X", umac[0], umac[1], umac[2], umac[3], umac[4], umac[5]);
		}
#endif
	}
	close(fd);
	return inf->ifcount;
#endif
}
void cl_net::print_mac()
{
	ifinfo_s inf;
	inf.ifcount = 10;
	int i=0;
	printf("#[muc begin] mac=%s \n	 (name - mac - ip - up - running - lookback):\n",get_mac().c_str());
	if(get_macall(&inf)>0)
	{
		for(i=0;i<inf.ifcount;++i)
		{
			printf("	%d : %s - %s - %s - %s - %s - %s \n",i,
				inf.ifs[i].name,inf.ifs[i].mac,inf.ifs[i].ip,
				inf.ifs[i].isup?"up":"down",
				inf.ifs[i].running?"running":"no",
				inf.ifs[i].isloopback?"loopback":"");
		}
	}
	printf("#[muc end]!\n");
}


string cl_net::get_mac_codeid()
{
	char pid[32];
	unsigned char umac[10]; //只有6位
	unsigned char umac2[10];
	cl_net::get_umac(umac2);
	for(int i=0;i<6;++i)
	{
		umac[i] = ~(umac2[5-i] + 77);
	}
	sprintf(pid,"%02X%02X%02X%02X%02X%02X",umac[0],umac[1],umac[2],umac[3],umac[4],umac[5]);
	return pid;
}
int cl_net::get_mtu()
{
	//取所有网卡中最小的mtu
#ifdef _WIN32
	PIP_ADAPTER_ADDRESSES pad = NULL;
	ULONG padlen = 0;
	DWORD mtu = 1500;
	DWORD ret = 0;
	GetAdaptersAddresses(AF_UNSPEC,0, NULL, pad,&padlen);
	pad = (PIP_ADAPTER_ADDRESSES) malloc(padlen);
	if(NO_ERROR==(ret=GetAdaptersAddresses(AF_INET,GAA_FLAG_SKIP_ANYCAST,0,pad,&padlen)))
	{
		mtu = pad->Mtu;
		PIP_ADAPTER_ADDRESSES p = pad->Next;
		while(p)
		{
			if(p->Mtu>0 && mtu > p->Mtu)
				mtu = p->Mtu;
			p = p->Next;
		};
	}

	free(pad);
	if(mtu<=0) mtu = 1500;
	return (int)mtu;
#else
	struct ifreq *ifr;
	struct ifconf conf;
	int fd,mtu,n,i;

	conf.ifc_len = 0;
	conf.ifc_buf = NULL;
	mtu = 1500;

	if((fd = socket(AF_INET,SOCK_DGRAM,0))<=0)
		goto fail;

	if(0!=ioctl(fd,SIOCGIFCONF,&conf))
		goto fail;
	conf.ifc_buf = (char*)malloc(conf.ifc_len);
	if(0!=ioctl(fd,SIOCGIFCONF,&conf))
		goto fail;
	
	n = conf.ifc_len / sizeof(struct ifreq);
	//printf("ifc_len = %d, n = %d, sizeof(ifreq)=%d \n",conf.ifc_len,n,sizeof(struct ifreq));
	if(n>0)
	{
		ifr = conf.ifc_req;
		if(0==ioctl(fd, SIOCGIFMTU, (void*)ifr))
		{
			mtu = ifr->ifr_mtu;
			//printf("ifr: %s , mtu=%d \n",ifr->ifr_name,ifr->ifr_mtu);
			for(i=1;i<n;++i)
			{
				ifr++;
				if(0==ioctl(fd, SIOCGIFMTU, (void*)ifr))
				{
					if(ifr->ifr_mtu>0 && mtu>ifr->ifr_mtu)
						mtu = ifr->ifr_mtu;
					//printf("ifr: %s , mtu=%d \n",ifr->ifr_name,ifr->ifr_mtu);
				}
			}
		}
	}

fail:
	if(fd>0)
		close(fd);
	if(conf.ifc_buf)
		free(conf.ifc_buf);
	if(mtu<=0) mtu = 1500;
	return mtu;

#endif
}

unsigned int cl_net::get_server_time_sec1830_tcp(const char* timesvr)
{
	unsigned int t = 0;
	string srv = timesvr;
	if(srv.empty())
		srv = "us.ntp.org.cn";
	string ip = cl_net::ip_explain_ex(srv.c_str());
	SOCKET sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock == INVALID_SOCKET)
		return t;
	
	sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(123);
	addr.sin_addr.s_addr = inet_addr(ip.c_str());
	if(SOCKET_ERROR == connect(sock,(sockaddr*)&addr,sizeof(addr)))
	{
		closesocket(sock);
		return t;
	}
	unsigned char nTime[16];
	int n = recv(sock,(char*)nTime,16,0);
	if(n<4)
	{
		closesocket(sock);
		return t;
	}
	closesocket(sock);
	assert(4==n);
	t = *(unsigned int*)nTime;
	t = ntohl(t);
	return t;
}

//linux的 ntpdate us.ntp.org.cn 表示连美国官方的时间服务器
int cl_net::get_server_time(time_t *t,const char* serve/*r="us.ntp.org.cn"*/) //从us.ntp.org.cn 37 中获取1970年以来的秒数
{
	//此接口经常获取不到，不是标准的 ntp协议（udp 123 协议）
	unsigned int dwTime = get_server_time_sec1830_tcp(serve);
	if(0==dwTime) 
	{
		*t = 0;
		return -1;
	}
	dwTime -= (unsigned int)2208988800UL; //取得的是1830年后的时间，转成1900年时间（减70年）
	//dwTime += 8*3600; //转为本地时间,localtime()会转
	*t = (time_t)dwTime;
	return 0;
}
//**********************************************
bool cl_net::is_ip(const char* ip)
{
	unsigned int ip_n[4];
	if(NULL!=ip && 4==sscanf(ip,"%d.%d.%d.%d",&ip_n[0],&ip_n[1],&ip_n[2],&ip_n[3]))
		return true;
	return false;
}
bool cl_net::is_dev(const char* ip)
{
	if(ip && ip[0]!='\0' && !is_ip(ip))
		return true;
	return false;
}

string cl_net::ip_explain(const char* s)
{
	string ip="";
	if(NULL==s || 0==strlen(s))
		return ip;
	if(INADDR_NONE != inet_addr(s))
	{
		return s;
	}
	else
	{
		ip = cl_DIPCache::findip(s);
		if(!ip.empty())
			return ip;

		in_addr sin_addr;
		hostent* host = gethostbyname(s);
		if (host == NULL) 
		{
			printf("gethostbyname return null\n");
#ifdef _WIN32

#else
			printf("----cl_net::ip_explain: use ping to explain (%s) ----\n",s);
			char cmd[1024];
			sprintf( cmd, "ping -c 1 %s", s );
			FILE * pd = popen( cmd , "r");
			if ( pd != NULL )
			{
				if( fgets( cmd, 1000, pd ) != NULL )
				{
					char * s1 = strtok( cmd, "(" );
					if ( s1 != NULL )
					{
						char * ip_e = strtok(NULL,")");
						if ( is_ip(ip_e) )
						{
							printf("read ip from ping: %s\n", ip_e );
							ip = ip_e;
						}
					}
				}
				pclose( pd );
			}
#endif
		}
		else
		{
			sin_addr.s_addr = *((unsigned long*)host->h_addr);
			ip = inet_ntoa(sin_addr);
		}
		if(!ip.empty() && is_ip(ip.c_str()))
			cl_DIPCache::addip(s,ip);
		return ip;
	}
}
typedef struct tagIPFD
{
	string dns;
	string ip;
	int is_set;
	int del;
	tagIPFD(void)
	{
		is_set = 0;
		del = 0;
	}
}IPFD;

#ifdef _WIN32
DWORD WINAPI ip_explain_T(void *p)
#else
void *ip_explain_T(void *p)
#endif
{
	IPFD *ipf = (IPFD*)p;
	ipf->ip = cl_net::ip_explain(ipf->dns.c_str());
	ipf->is_set = 1;
	while(!ipf->del) 
		Sleep(100);
	delete ipf;

#ifdef _WIN32
	return 0;
#else
	return (void*)0;
#endif
}
string cl_net::ip_explain_ex(const char* s,int maxTick/*=5000*/)
{
	if(NULL==s || 0==strlen(s))
		return "";
	string ip;
	if(INADDR_NONE != inet_addr(s))
	{
		return s;//就是ip
	}
	else
	{
		ip = cl_DIPCache::findip(s);
		if(!ip.empty())
			return ip;

		IPFD *ipf = new IPFD();
		if(!ipf)
			return s;
		ipf->dns = s;
#ifdef _WIN32
		DWORD thid=0;
		HANDLE h = CreateThread(NULL,0,ip_explain_T,(void*)ipf,0,&thid);
		if(INVALID_HANDLE_VALUE==h)
			return s;
		else
			CloseHandle(h);
#else
		pthread_t hthread;
		if(0==pthread_create(&hthread,NULL,ip_explain_T,(void*)ipf))
			pthread_detach(hthread);
		else
			return s;
#endif
		Sleep(10);
		int i=maxTick/100;
		
		while(1)
		{
			if(ipf->is_set)
			{
				ip = ipf->ip;
				ipf->del = 1; 
				return ip;
			}
			if(i<=0)
				break;
			Sleep(100);
			i--;
		}
		ipf->del = 1;
		//return s;
		return "";
		//inet_addr("")=0,inet_addr(NULL)=-1,inet_addr("asdfs.s322.dassadf")=-1;
		//
	}
}
//****************************************************

char* cl_net::ip_htoa(unsigned int ip)
{
	//非线程安全
	static char buf[32];
	sprintf(buf,"%d.%d.%d.%d",((ip>>24)&0xff),((ip>>16)&0xff),((ip>>8)&0xff),(ip&0xff));
	return buf;
}
char* cl_net::ip_ntoa(unsigned int nip)
{
	//非线程安全
	//inet_addr()使用同一块内存，如果调用一个函数里面有两个参数执行了ip_ntoa，则传入结果是同一个
	static char buf[32];
	unsigned char *ip_n = (unsigned char*)&nip;
	sprintf(buf,"%d.%d.%d.%d",ip_n[0],ip_n[1],ip_n[2],ip_n[3]);
	return buf;
}
string cl_net::ip_htoas(unsigned int ip)
{
	char buf[32];
	sprintf(buf,"%d.%d.%d.%d",((ip>>24)&0xff),((ip>>16)&0xff),((ip>>8)&0xff),(ip&0xff));
	return buf;
}
string cl_net::ip_ntoas(unsigned int nip)
{
	char buf[32];
	unsigned char *ip_n = (unsigned char*)&nip;
	sprintf(buf,"%d.%d.%d.%d",(int)ip_n[0],(int)ip_n[1],(int)ip_n[2],(int)ip_n[3]);
	return buf;
}
string cl_net::ip_ntoas(unsigned int nip,unsigned short nport)
{
	char buf[64];
	sprintf(buf,"%s:%d",ip_ntoas(nip).c_str(),(int)ntohs(nport));
	return buf;
}

unsigned int cl_net::ip_aton(const char* ip)
{
	return cl_bstream::htob32(ip_atoh(ip));
}
unsigned int cl_net::ip_atoh(const char* ip)
{
	//atonl:inet_addr("")=0,inet_addr(NULL)=-1,inet_addr("asdfs.s322.dassadf")=-1
	//sscanf返回：EOF=-1为错误，其它表示成功输入参数的个数,失败返回0或-1
	unsigned int iip;
	unsigned int ip_n[4]={0,0,0,0};
	if(4!=sscanf(ip,"%d.%d.%d.%d",&ip_n[0],&ip_n[1],&ip_n[2],&ip_n[3]))
		return 0;
	//最前的IP放到大值
	iip =(ip_n[0] << 24) + (ip_n[1] << 16) + (ip_n[2] << 8) + ip_n[3];
	return iip;
}


//*******************************************************
#ifdef _WIN32	
string cl_net::get_local_private_ip()
{
	string tmp;
	hostent* he = NULL;
	//he = gethostbyname(buf);
	//printf("$: gethostbyname(%s) \n",buf);
	////test:
	he = gethostbyname(NULL);
	if(he == NULL || he->h_addr_list[0] == 0)
	{
		return "";
	}

	in_addr addr;
	int i = 0;
	
	// We take the first ip as default, but if we can find a better one, use it instead...
	memcpy(&addr, he->h_addr_list[i++], sizeof addr);
	tmp = inet_ntoa(addr);
	if(strncmp(tmp.c_str(), "127", 3) == 0 || (!cl_net::is_private_ip(tmp) && strncmp(tmp.c_str(), "169", 3) != 0) )
	{
		while(he->h_addr_list[i]) 
		{
			memcpy(&addr, he->h_addr_list[i], sizeof addr);
			string tmp2 = inet_ntoa(addr);
			if(strncmp(tmp2.c_str(), "127", 3) !=0 && (cl_net::is_private_ip(tmp2) || strncmp(tmp2.c_str(), "169", 3) == 0) )
			{
				tmp = tmp2;
			}
			i++;
		}
	}
	//printf("$local_ip = %s \n",tmp.c_str());

	return tmp;
}
#else
string cl_net::get_local_private_ip()
{
	int fd, num,i;
    struct ifconf ifc;
    struct ifreq buf[32];
	struct ifreq *ifr;
	sockaddr_in *sin;
	string ip="0.0.0.0";
	string tmp;

	printf("$: //***********************//\n");
	printf("$: //**get local private ip : \n");
    if((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
	{
        ifc.ifc_len = sizeof buf;
        ifc.ifc_buf = (caddr_t) buf;
        if(!ioctl(fd, SIOCGIFCONF, (char *) &ifc)) 
		{
            num = ifc.ifc_len / sizeof(struct ifreq);
			ifr = ifc.ifc_req;
            printf("$:interface num=%d\n", num);
			for(i=0;i<num;++i,ifr++)
			{
				sin = (struct sockaddr_in*)(&ifr->ifr_addr);
				if(0==ioctl(fd,SIOCGIFFLAGS,ifr))
				{
					if( !(ifr->ifr_flags & IFF_LOOPBACK) && ifr->ifr_flags & IFF_UP ) 
					{
						tmp = inet_ntoa(sin->sin_addr);
						if(strncmp(tmp.c_str(), "127", 3) !=0 && (is_private_ip(tmp) || strncmp(tmp.c_str(), "169", 3) == 0) )
						{
							ip = tmp;
							break;
						}
					}
				}

			}
		}
		close(fd);
	}
	else
        perror("cpm: socket()");
	printf("$: local private ip : %s \n",ip.c_str());
	printf("$: //***********************//\n");
	return ip;
}
#endif
/////////////////////////////////////////////////////////////
typedef struct tagGPIP
{
	string ip;
	int is_set;
	int del;
	tagGPIP(void)
	{
		is_set = 0;
		del = 0;
	}
}GPIP;
#ifdef _WIN32
DWORD WINAPI get_local_private_ip_ex_T(void *p)
#else
void *get_local_private_ip_ex_T(void *p)
#endif
{
	GPIP *ipf = (GPIP*)p;
	ipf->ip = cl_net::get_local_private_ip();
	ipf->is_set = 1;
	while(!ipf->del) 
		Sleep(100);
	delete ipf;
#ifdef _WIN32
	return 0;
#else
	return (void*)0;
#endif
}
string cl_net::get_local_private_ip_ex(int timeout_tick/*=5000*/)
{
	string tmp="0.0.0.0";

	GPIP *ipf = new GPIP();
		if(!ipf)
			return tmp;
#ifdef _WIN32
		DWORD thid=0;
		HANDLE h = CreateThread(NULL,0,get_local_private_ip_ex_T,(void*)ipf,0,&thid);
		if(INVALID_HANDLE_VALUE==h)
			return tmp;
		else
			CloseHandle(h);
#elif defined(_ECOS_8203)
		//TODO_ECOS: 暂时不独立线程创建
		delete ipf;
		tmp = get_local_private_ip();
		return tmp;
#else
		pthread_t hthread;
		if(0==pthread_create(&hthread,NULL,get_local_private_ip_ex_T,(void*)ipf))
			pthread_detach(hthread);
		else
			return tmp;
#endif
		Sleep(10);
		int i=timeout_tick/100;
		while(1)
		{
			if(ipf->is_set)
			{
				tmp = ipf->ip;
				ipf->del = 1; //总时让另外线程删除它最安全
				return tmp;
			}
			if(i<=0)
				break;
			Sleep(100);
			i--;
		}
		ipf->del = 1;
	return tmp;
}

bool cl_net::is_private_ip(string const& ip) 
{
	unsigned long naddr;

	naddr = inet_addr(ip.c_str());

	if (naddr != INADDR_NONE) {
		unsigned long haddr = ntohl(naddr);
		return ((haddr & 0xff000000) == 0x0a000000 || // 10.0.0.0/8
				(haddr & 0xff000000) == 0x7f000000 || // 127.0.0.0/8
				(haddr & 0xfff00000) == 0xac100000 || // 172.16.0.0/12
				(haddr & 0xffff0000) == 0xc0a80000);  // 192.168.0.0/16
	}
	return false;
}

//***********************************************************************
