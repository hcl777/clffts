#include "cl_umpsearch.h"

#include "cl_net.h"
#include "cl_util.h"

cl_umpsearch::cl_umpsearch(void)
	:m_brun(false)
{
}


cl_umpsearch::~cl_umpsearch(void)
{
}
int cl_umpsearch::run(const char* multiaddr)
{
	if(m_brun) return 1;

	string ip,str;
	str = multiaddr;
	ip = cl_util::get_string_index(str,0,":");
	m_port = cl_util::atoi(cl_util::get_string_index(str,1,":").c_str());
	if(ip.empty()) ip = "239.0.0.2";
	if(0==m_port) m_port = 7770;
	strcpy(m_multiip,ip.c_str());

	m_brun = true;
	this->activate(1);
	return 0;
}
void cl_umpsearch::end()
{
	if(!m_brun) return;
	m_brun = false;
	wait();
}
int cl_umpsearch::work(int e)
{
	sockaddr_in from;
	socklen_t fromlen;
	int n = 0;
	char buf[4096];
	SOCKET fd;

	fromlen = sizeof(sockaddr_in);
	memset(&from,0,fromlen);
	fd = socket(AF_INET,SOCK_DGRAM,0);
	if(INVALID_SOCKET == fd)
		return -1;

	cl_net::sock_set_udp_multicast(fd,m_multiip);
	cl_net::sock_set_timeout(fd,200);
	cl_net::sock_bind(fd,m_port);

	printf("# umpsearch server open(%s:%d) \n",m_multiip,m_port);
	while(m_brun)
	{
		n = recvfrom(fd,buf,4096,0,(sockaddr*)&from,&fromlen);
		if(n>0)
		{
			buf[n] = '\0';
			printf("ump server recv(%s): %s \n",cl_net::ip_ntoas(from.sin_addr.s_addr,from.sin_port).c_str(),buf);
			sendto(fd,buf,n,0,(const sockaddr*)&from,fromlen);
		}
	}
	closesocket(fd);
	return 0;
}
int cl_umpsearch::find(const char* multiaddr)
{
	sockaddr_in addr,from;
	socklen_t fromlen;
	int n = 0;
	char buf[4096];
	char reqbuf[1024];
	SOCKET fd;

	
	fromlen = sizeof(sockaddr_in);
	memset(&from,0,fromlen);
	memset(&addr,0,fromlen);

	string ip,str;
	unsigned short port;
	str = multiaddr;
	ip = cl_util::get_string_index(str,0,":");
	port = cl_util::atoi(cl_util::get_string_index(str,1,":").c_str());
	if(ip.empty()) ip = "239.0.0.2";
	if(0==port) port = 7770;

	printf("# umpsearch find(%s:%d) \n",ip.c_str(),port);

	fd = socket(AF_INET,SOCK_DGRAM,0);
	if(INVALID_SOCKET == fd)
		return -1;

	cl_net::sock_set_timeout(fd,300);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip.c_str());

	sprintf(reqbuf,"test ump");
	DWORD begtick = GetTickCount();
	int timeo = 5000;
	while(1)
	{
		if(sendto(fd,reqbuf,(int)strlen(reqbuf),0,(const sockaddr*)&addr,sizeof(addr))>0)
		{
			while(1)
			{
				n = recvfrom(fd,buf,1024,0,(sockaddr*)&from,&fromlen);
				if(n>0)
				{
					buf[n] = '\0';
					printf("ump recv(%s): %s \n",cl_net::ip_ntoas(from.sin_addr.s_addr,from.sin_port).c_str(),buf);
				}
				else
					break;
			}
			if(begtick + timeo < GetTickCount())
				break;

		}
		else
		{
			perror("***sendto(req):");
			break;
		}
	}

	closesocket(fd);
	return 0;
}

int cl_umpsarech_main(int argc,char** argv)
{
	if(-1!=cl_util::string_array_find(argc,argv,"-h"))
	{
		printf("version: cl_umpsarech-20171120: \n");
		printf("cl_umpsarech -a multiip:port -f multiip:port \n");
		return 0;
	}
	cl_util::debug_memleak();
	cl_net::socket_init();
	
	cl_umpsearch ump;
	
	bool isclient = false;
	int i=0;
	string addr;
	if(-1!=(i=cl_util::string_array_find(argc,argv,"-a")))
	{
		if(i+1<argc)
			addr = argv[i+1];
	}

	if(-1!=(i=cl_util::string_array_find(argc,argv,"-f")))
	{
		isclient = true;
	}
	
	if(!isclient)
	{
		ump.run(addr.c_str());
		while(1) Sleep(10000);
		ump.end();
	}
	else
	{
		cl_umpsearch::find(addr.c_str());
	}

	cl_net::socket_fini();
	return 0;
}
