#include "cl_httpc2.h"
#include "cl_incnet.h"
#include "cl_util.h"
#include "cl_net.h"
#include "cl_httphead.h"
#include "cl_file64.h"

cl_httpc2::cl_httpc2(void)
{
}


cl_httpc2::~cl_httpc2(void)
{
}
int cl_httpc2::format_header(string& header,const string& server,const string& cgi,unsigned long long bodylen,int content_type)
{
	char buf[2048];
	sprintf(buf,"%s %s HTTP/1.1\r\n",bodylen>0?"POST":"GET",cgi.c_str());
	sprintf(buf+strlen(buf),"Host: %s\r\n",server.c_str());
	if(bodylen>0)
		sprintf(buf+strlen(buf),"Content-Length: %lld\r\n",bodylen);
	//strcat(buf,"Accept: */*\r\n");
	//Accept-Encoding: gzip, deflate
	strcat(buf,"User-Agent: Mozilla/4.0 (compatible; httpc 1.0;)\r\n");
	strcat(buf,"Pragma: no-cache\r\n");
	strcat(buf,"Cache-Control: no-cache\r\n");
	//Content-Type: application/x-www-form-urlencoded ；表示普通表单格式上传
	//Content-Type: multipart/form-data  ；表示上传文件的规格
	//Content-Type: application/octet-stream ； 表示上传一个文件流数据
	strcat(buf,"Content-Type: application/x-www-form-urlencoded\r\n");
	//strcat(buf,"Connection: Close \r\n");
	strcat(buf,"\r\n");
	header = buf;
	return 0;
}
int cl_httpc2::http_open_send_head(const string& url,unsigned long long bodylen,int content_type)
{
	
	string server,cgi,ip,header;
	unsigned short port;
	SOCKET sock = INVALID_SOCKET;
	//printf("#http request url=%s \n",url.c_str());
	int err = 0;
	do
	{
		cl_util::url_element_split(url,server,port,cgi);
		ip = cl_net::ip_explain_ex(server.c_str());
		cl_httpc2::format_header(header,server,cgi,bodylen,content_type);
		sock = socket(AF_INET,SOCK_STREAM,0);
		if(INVALID_SOCKET==sock)
		{
			err = 2;
			break;
		}

		//设置超时:
#ifdef _WIN32
		int x = 20000;
#else
		struct timeval x;  
		x.tv_sec = 20;
		x.tv_usec = 0;
#endif
		if(-1==setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char*)&x,sizeof(x)))
			perror("setsockopt SO_RCVTIMEO");
		if(-1==setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,(char*)&x,sizeof(x)))
			perror("setsockopt SO_SNDTIMEO");

		sockaddr_in addr;
		memset(&addr,0,sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = inet_addr(ip.c_str());
		if(SOCKET_ERROR==connect(sock,(sockaddr*)&addr,sizeof(addr)))
		{
			err = 3;
			break;
		}

		if(0!=cl_net::sock_send_n((int)sock,header.c_str(),(int)header.length()))
		{
			err = 4;
			break;
		}

	}while(0);
	if(0!=err)
	{
		if(sock!=INVALID_SOCKET)
		{
			closesocket(sock);
			sock = INVALID_SOCKET;
		}
		printf("***http request faild! url=%s \n",url.c_str());
	}
	return (int)sock;
}
int cl_httpc2::http_recv_head(int fd,httpc2_response_t* rsp)
{
	int n = 0;
	int readsize = 0;
	char *p = NULL;
	char* buf = rsp->header;
	bool bok = false;
	string str;
	rsp->bodylen = 0;
	while(readsize<HTTPC_MAX_HEADLEN)
	{
			n = recv(fd,buf+readsize,HTTPC_MAX_HEADLEN-readsize,0);
			if(n<=0)
			{
				return -1;
			}
			readsize+=n;
			buf[readsize] = '\0';
			p = strstr(buf,"\r\n\r\n");
			if(p)
			{
				rsp->bodylen = readsize - (int)(p-buf+4);
				assert(rsp->bodylen>=0);
				if(rsp->bodylen>0)
				{
					memcpy(rsp->body,p+4,(int)rsp->bodylen);
				}
				p[4] = '\0';
				bok = true;
				break;
			}
	}
	if(!bok)
		return -1;
	rsp->retcode = cl_httphead::get_code(rsp->header);
	if(0==cl_httphead::get_field(rsp->header,"Content-Length",str))
		rsp->Content_Length = cl_util::atoll(str.c_str());
	else
		rsp->Content_Length = 0;
	return 0;
}
int cl_httpc2::post_file(string& rspbody,const string& path,const string& url,int content_type/*=CONTENT_TYPE_URLCODE*/)
{
	cl_file64 file;
	int fd = INVALID_SOCKET;
	size64_t bodylen = 0,sendlen = 0;
	httpc2_response_t rsphead;
	int retcode = -1;
	int n=0;
	const int BUFSIZE = 64*1024;
	char* buf = new char[BUFSIZE];
	
	do
	{
		if(0!=file.open(path.c_str(),F64_READ))
			break;
		bodylen = file.get_file_size();

		//发送头
		fd = http_open_send_head(url,bodylen,content_type);
		if(INVALID_SOCKET==fd)
			break;

		//发送文件
		while(sendlen<bodylen)
		{
			n = file.read(buf,BUFSIZE);
			if(n<=0)
				break;
			if(0!=cl_net::sock_send_n(fd,buf,n))
				break;
			sendlen += n;
		}
		if(sendlen<bodylen)
			break;

		//接收头
		if(0!=http_recv_head((int)fd,&rsphead))
			break;

		//接收数据
		if(rsphead.Content_Length>10240000)
			break;
		rsphead.pbody = new char[(int)rsphead.Content_Length+2];
		if(rsphead.bodylen>0)
			memcpy(rsphead.pbody,rsphead.body,(int)rsphead.bodylen);
		while(rsphead.bodylen<rsphead.Content_Length)
		{
			n = recv(fd,rsphead.pbody+(int)rsphead.bodylen,(int)(rsphead.Content_Length-rsphead.bodylen),0);
			if(n<=0)
				break;
			rsphead.bodylen += n;
		}
		if(rsphead.bodylen>=rsphead.Content_Length)
		{
			rsphead.pbody[rsphead.bodylen] = '\0';
			rspbody = rsphead.pbody;
			retcode = 0;
		}

	}while(0);
	if(INVALID_SOCKET!=fd)
		closesocket(fd);
	file.close();
	delete[] buf;
	return retcode;
}


