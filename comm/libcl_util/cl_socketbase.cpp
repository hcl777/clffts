#include "cl_socketbase.h"

#include <stdio.h>
#include <string.h>
#include "cl_incnet.h"
#include "cl_net.h"
#ifdef _WIN32
#pragma warning(disable:4996)
#endif


cl_socketbase::cl_socketbase(void)
{
}

cl_socketbase::~cl_socketbase(void)
{
}
int cl_socketbase::socket_tcp()
{
	return (int)socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
}
int cl_socketbase::socket_udp()
{
	return (int)socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
}
int cl_socketbase::open_udp_sock(int& fd,unsigned short nport,unsigned int nip,int rcvbuf,int sndbuf)
{
	sockaddr_in addr;

	fd = (int)socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(INVALID_SOCKET == fd)
	{
		return -1;
	}
	//设置接收和发送缓冲
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, ( const char* )&rcvbuf, sizeof(int)); 
	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, ( const char* )&sndbuf, sizeof(int)); 

	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = nport;
	addr.sin_addr.s_addr = nip; //INADDR_ANY
		
	if(SOCKET_ERROR == ::bind(fd, (sockaddr *)&addr, sizeof(addr)))
	{
		close_sock(fd);
		DEBUGMSG("#*** bind(%s) faild \n",cl_net::ip_ntoas(nip,nport).c_str());
		return 1;
	}
	DEBUGMSG("# cl_socketbase::open udp(%s) \n",cl_net::ip_ntoas(nip,nport).c_str());

	return 0;
}
void cl_socketbase::close_sock(int& fd)
{
	if(INVALID_SOCKET!=fd)
	{
#ifdef _WIN32
		::closesocket(fd);
#else
		::close(fd);
#endif
		fd = INVALID_SOCKET;
	}
}

int cl_socketbase::set_nonblock(int fd,int nonblock/*=1*/)
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
int cl_socketbase::set_timeout(int fd,int timeo_ms)
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
int cl_socketbase::set_udp_broadcast(int fd)
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
int cl_socketbase::set_udp_multicast(int fd,const char* multi_ip)
{
#ifdef WIN32
	return -1;
#else
	struct ip_mreq mreq; 
	mreq.imr_multiaddr.s_addr=inet_addr(multi_ip);    
	mreq.imr_interface.s_addr=INADDR_ANY;//htonl(INADDR_ANY);    
	if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0)     
	{    
		perror("setsockopt"); 
		return -1; 
	}
	return 0;
#endif
}
int cl_socketbase::bind_device(int fd,const char* device)
{
#ifdef __GNUC__
	struct ifreq ifr;
	memset(&ifr,0,sizeof(ifr));
	//strncpy(ifr.ifr_name,device,IFNAMSIZ); 
	strcpy(ifr.ifr_name,device);
	if(SOCKET_ERROR==setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, (char*)&ifr, sizeof(ifr)))
	{
		perror("#***bind device failed \n");
		return -1;
	}
	else
	{
		perror("#bind device ok \n");
		return 0;
	}
#endif
	return -1;
}

int cl_socketbase::sendto(int fd,const char* buf,int size,unsigned int ip,unsigned short port)
{
	sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(ip);
	addr.sin_port = htons(port);
	return ::sendto(fd,buf,size,0,(const sockaddr*)&addr,sizeof(addr));
}
int cl_socketbase::recvfrom(int fd,char* buf,int size,unsigned int* ip,unsigned short* port)
{
	sockaddr_in from_addr;
	socklen_t from_len;
	memset(&from_addr,0,sizeof(from_addr));
	from_len = sizeof(from_addr);
	int ret = ::recvfrom(fd,buf,size,0,(sockaddr*)&from_addr,&from_len);
	if(ret>0)
	{
		*ip = ntohl(from_addr.sin_addr.s_addr);
		*port = ntohs(from_addr.sin_port);
	}
	return ret;
}

