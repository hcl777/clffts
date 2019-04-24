#pragma once

#ifdef _WIN32

#else
#endif

class cl_socketbase
{
public:
	cl_socketbase(void);
	~cl_socketbase(void);

public:
	static int socket_tcp();
	static int socket_udp();
	static int open_udp_sock(int& fd,unsigned short nport,unsigned int nip,int rcvbuf,int sndbuf);
	static void close_sock(int& fd);

	static int set_nonblock(int fd,int nonblock=1); //inoblock=1 ±íÊ¾·Ç×èÈû
	static int set_timeout(int fd,int timeo_ms);
	static int set_udp_broadcast(int fd);
	static int set_udp_multicast(int fd,const char* multi_ip);
	static int bind_device(int fd,const char* device);

	static int sendto(int fd,const char* buf,int size,unsigned int ip,unsigned short port);
	static int recvfrom(int fd,char* buf,int size,unsigned int* ip,unsigned short* port);
};
