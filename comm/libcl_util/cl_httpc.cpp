
#include "cl_httpc.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "cl_file64.h"
#include "cl_net.h"
#include "cl_util.h"
#include "cl_membuf.h"

#ifdef _WIN32
#pragma warning(disable:4996)
#else
#include <errno.h>

#include <pthread.h>
#endif

static int httpc_set_blocking(SOCKET s,bool bBlocking);
static int httpc_is_connected( SOCKET s, fd_set *rd, fd_set *wr, fd_set *ex);

cl_httpc::cl_httpc(void)
{
}

cl_httpc::~cl_httpc(void)
{
}
int cl_httpc::request(const string& url,const char* body,int bodylen,string& rspheader,string& rspbody)
{
	int n = 0;
	httpc_response_t rsp;
	SOCKET fd = http_open_request(url,body,bodylen);
	if(INVALID_SOCKET==fd)
		return -1;
	do
	{
		if(0!=http_recv_header((int)fd,&rsp))
			break;
		if(rsp.Content_Length>10240000|| rsp.Content_Length<rsp.bodylen)
			break;
		rsp.pbody = new char[(int)rsp.Content_Length+2];
		if(rsp.bodylen>0)
			memcpy(rsp.pbody,rsp.body,(int)rsp.bodylen);
		while(rsp.bodylen<rsp.Content_Length)
		{
			n = recv(fd,rsp.pbody+(int)rsp.bodylen,(int)(rsp.Content_Length-rsp.bodylen),0);
			if(n<=0)
				break;
			rsp.bodylen += n;
		}
		break;
	}while(0);
	closesocket(fd);
	if (rsp.Content_Length > 0)
	{
		rsp.pbody[rsp.bodylen] = '\0';
		rspbody = rsp.pbody;
	}
	if(rsp.bodylen == rsp.Content_Length && 200==rsp.retcode)
	{
		rspheader = rsp.header;
		return 0;
	}
	return -1;
}
int cl_httpc::download_file(const string& url,const string& filepath)
{
	cl_file64 file;
	int n = 0;
	httpc_response_t rsp;
	SOCKET fd = http_open_request(url,NULL,0);
	if(INVALID_SOCKET==fd)
		return -1;
	do
	{
		if(0!=http_recv_header((int)fd,&rsp))
			break;
		if(200!=rsp.retcode||rsp.Content_Length<=0)
			break;
		
		if(0!=file.open(filepath.c_str(),F64_RDWR|F64_TRUN))
			break;
		if(rsp.bodylen>0)
		{
			if(0!=file.write_n(rsp.body,(int)rsp.bodylen))
				break;
		}
		while(rsp.bodylen<rsp.Content_Length)
		{
			n = HTTPC_MAX_HEADLEN+1;
			n = recv(fd,rsp.body,n,0);
			if(n<=0)
				break;
			if(0!=file.write_n(rsp.body,n))
				break;
			rsp.bodylen += n;
		}
		break;
	}while(0);
	closesocket(fd);

	if(rsp.bodylen == rsp.Content_Length && 200==rsp.retcode)
	{
		return 0;
	}
	return -1;
}

int cl_httpc::http_open_request(const string& url,const char* body,int bodylen)
{
	string server,cgi,ip,header;
	unsigned short port;
	SOCKET sock = INVALID_SOCKET;
	//printf("#http request url=%s \n",url.c_str());
	int err = 0;
	do
	{
		cl_httpc::url_element_split(url,server,port,cgi);
		ip = cl_httpc::ip_explain_ex(server.c_str());
		cl_httpc::format_header(header,server,cgi,bodylen);
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
		x.tv_sec = 30;
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

		if(0!=cl_httpc::send_n((int)sock,header.c_str(),(int)header.length()))
		{
			err = 4;
			break;
		}
		if(bodylen>0 && 0!=cl_httpc::send_n((int)sock,body,bodylen))
		{
			err = 5;
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
int cl_httpc::http_recv_header(int fd,cl_httpc::httpc_response_t* rsp)
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
	rsp->retcode = cl_httpc::get_server_response_code(rsp->header);
	if(0==cl_httpc::get_field(rsp->header,"Content-Length",str))
		rsp->Content_Length = httpc_atoll(str.c_str());
	else
		rsp->Content_Length = 0;
	return 0;
}
int cl_httpc::http_recv_data(int fd,char* buf,int size)
{
	return recv(fd,buf,size,0);;
}
int cl_httpc::http_close(int fd)
{
	return closesocket(fd);
}
int cl_httpc::send_n(int sock,const char *buf,int size)
{
	int pos=0;
	int ret=0;
	while(pos<size)
	{
		ret = send(sock,buf+pos,size-pos,0);
		if(ret>0)
			pos += ret;
		else
			return -1;
	}
	return 0;
}


//************************************************************
int cl_httpc::http_get(const string& url,char* sret,int retlen)
{
	int len = retlen-1;
	if(0==http_get_buffer(url,sret,len))
	{
		sret[len] = '\0';
		return 0;
	}
	return -1;
}
int cl_httpc::http_get_buffer(const string& url,char* sret,int& iolen)
{

	string server,cgi,ip,header;
	unsigned short port;
	SOCKET sock = INVALID_SOCKET;
	int ret;

	int err = 0;
	do
	{
		url_element_split(url,server,port,cgi);
		//ip = ip_format(server.c_str());
		ip = ip_explain_ex(server.c_str());
		format_header(header,server,cgi,0);
		
		sock = socket(AF_INET,SOCK_STREAM,0);
		if(INVALID_SOCKET==sock)
		{
			err = 2;
			break;
		}

		httpc_set_blocking(sock,false);
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
		if(0!=(ret=connect(sock,(sockaddr*)&addr,sizeof(addr))))
		{
#ifdef _WIN32
			int ierr = WSAGetLastError();
			if(WSAEWOULDBLOCK != ierr)
#else
			if(EINPROGRESS != errno)
#endif
			{
				err = 3;
				break;
			}
		}

		if (ret==0)
		{
			httpc_set_blocking(sock,true);
		}
		else
		{
			fd_set rdevents;
			fd_set wrevents;
			fd_set exevents;
			struct timeval y;  
			y.tv_sec = 30;
			y.tv_usec = 0;
			FD_ZERO(&rdevents); 
			FD_SET(sock, &rdevents);
			wrevents = rdevents;
			exevents = rdevents;
			ret = select((int)sock+1, &rdevents, &wrevents, &exevents, &y); 
			if (ret<0)
			{
				err = 8;
				break;
			}
			else if (ret==0)
			{
				err = 9;
				break;
			}
			else
			{
				if (httpc_is_connected(sock, &rdevents, &wrevents, &exevents ))
				{
					httpc_set_blocking(sock,true);
				}
				else
				{
					err = 10;
					break;
				}
			}
		
		}

		if(0!=send_n((int)sock,header.c_str(),(int)header.length()))
		{
			err = 4;
			break;
		}

		//接收头
		string str;
		cl_membuf b(2047);
		int ret;
		memset(b.buf,0,b.buflen);
		char *ptr=NULL;
		int data_size=0;
		int file_size=0;
		while(1)
		{
			ret = recv(sock,b.write_ptr(),b.overage(),0);
			if(ret<=0)
			{
				err = 5;
				break;
			}
			b.wpos += ret;

			ptr = strstr(b.buf,"\r\n\r\n");
			if(ptr)
			{
				*(ptr+2) = '\0';
				header = b.buf;
				ptr += 4;
				b.rpos = (int)(ptr-b.buf);
				b.move0();
				break;
			}
			else
			{
				if(0==b.overage())
				{
					err = 6;
					break;
				}
			}
		}
		if(0!=err)
			break;

		//解释头:
		if(200!=get_server_response_code(header))
		{
			err = 7;
			break;
		}
		if(0==get_field(header,"Content-Length",str))
		{
			file_size = atoi(str.c_str());
			if(file_size>iolen||file_size<=0)
				file_size = iolen;
		}
		else
		{
			file_size = iolen;
		}

		//如果返回头有 Transfer-Encoding: chunked 则内容必须分块接收
		bool chunked = false;
		int n;
		if(0==get_field(header,"Transfer-Encoding",str) && str=="chunked")
			chunked = true;
		if(!chunked)
		{
			if(b.len())
			{
				ret = CL_MIN(b.len(),file_size-data_size);
				memcpy(sret+data_size,b.read_ptr(),ret);
				b.rpos += ret;
				data_size += ret;
			}

			//接收数据
			while(data_size<file_size)
			{
				ret = recv(sock,sret+data_size,file_size-data_size,0);
				if(ret<=0)
				{
					err = 8;
					break;
				}
				data_size += ret;
			}
			iolen = data_size;
		}
		else
		{
			//先将数据收到缓冲，再解释缓冲
			int chunk_size,chunk_readsize;
			while(data_size<file_size)
			{
				//收chunk头
				while(b.overage() && !(ptr=strstr(b.read_ptr(),"\r\n")))
				{
					ret = recv(sock,b.write_ptr(),b.overage(),0);
					if(ret<=0) break;
					b.wpos += ret;
				}
				if(!ptr)
				{
					err = 9;
					break;
				}
				
				*ptr='\0';
				if(1!=sscanf(b.read_ptr(),"%x",&chunk_size))
					break;
				b.rpos = (int)(ptr-b.buf)+2;
				
				//收chunk
				chunk_readsize = 0;
				while(chunk_readsize<chunk_size)
				{
					if(b.overage())
					{
						ret = recv(sock,b.write_ptr(),b.overage(),0);
						if(ret>=0)
							b.wpos += ret;
					}
					if(b.len()==0)
						break;
					n = CL_MIN(b.len(),chunk_size-chunk_readsize);
					n = CL_MIN(n,file_size-data_size);
					memcpy(sret+data_size,b.read_ptr(),n);
					b.rpos += n;
					data_size += n;
					chunk_readsize += n;
					b.move0();
					if(data_size>=file_size || chunk_readsize>=chunk_size)
						break;
				}
				if(chunk_readsize<chunk_size)
				{
					err = 10;
					break;
				}

				//去掉两个回车
				while(b.len()<2)
				{
					ret = recv(sock,b.write_ptr(),b.overage(),0);
					if(ret<=0) break;
					b.wpos += ret;
				}
				if(b.len()<2)
				{
					err = 10;
					break;
				}
				b.rpos += 2;
				if(0==chunk_size)
				{
					//收完结束
					break;
				}
			}
			iolen = data_size;
		}

	}while(0);
	if(sock!=INVALID_SOCKET)
	{
		closesocket(sock);
		sock = INVALID_SOCKET;
	}
	if(0!=err)
		return -1;
	return 0;
}

int cl_httpc::url_element_split(const string& url,string& server,unsigned short& port,string& cgi)
{
	//解析url
	string str;
	int pos = 0,pos2 = 0, pos3 = 0;
	port = 80;
	server = "";
	cgi = "";

	pos = (int)url.find("http://",0);
	if(pos >= 0)
		pos += 7;
	else
	{
		pos = (int)url.find("HTTP://",0);
		if(pos >= 0)
			pos += 7;
		else
			pos = 0;
	}

	pos2 = (int)url.find(":",pos);
	pos3 = (int)url.find("/",pos);

	if(pos3 > 0 && pos2 > pos3)
		pos2 = -1;

	if(pos3 > pos)
	{
		if(pos2>pos)
		{
			server = url.substr(pos,pos2-pos);
			str = url.substr(pos2+1,pos3-pos2-1);
			port = atoi(str.c_str());
		}
		else
		{
			server = url.substr(pos,pos3-pos);
		}
		cgi = url.substr(pos3);
	}
	else
	{
		if(pos2>pos)
		{
			server = url.substr(pos,pos2-pos);
			str = url.substr(pos2+1);
			port = atoi(str.c_str());
		}
		else
		{
			server = url;
		}
		cgi = "/";
	}

	return 0;
}

int cl_httpc::format_header(string& header,const string& server,const string& cgi,int bodylen)
{
	char buf[2048];
	header = "";
	sprintf(buf,"%s %s HTTP/1.1\r\n",bodylen>0?"POST":"GET",cgi.c_str());
	sprintf(buf+strlen(buf),"Host: %s\r\n",server.c_str());
	if(bodylen>0)
		sprintf(buf+strlen(buf),"Content-Length: %d\r\n",bodylen);
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
int cl_httpc::get_server_response_code(const string& header)
{
	int pos = (int)header.find(" ");
	if(pos<0)
		return -1;
	else
		return atoi(header.substr(pos+1).c_str());
}
int cl_httpc::get_field(const string& header,const string& session, string& text)
{
	//取得某个域值
	if(header.empty()) 
		return -1;
	int nPos = -1;
	nPos = (int)header.find(session,0);
	if(nPos != -1)
	{
		nPos += (int)session.length();
		nPos += 2;
		int nCr = (int)header.find("\r\n",nPos);
		text = header.substr(nPos,nCr - nPos);
		cl_util::string_trim(text);
		return 0;
	}
	else
	{
		return -1;
	}
}

long long cl_httpc::httpc_atoll(const char* _Str)
{
	long long i=0;
	if(NULL==_Str)
		return 0;
	if(1!=sscanf(_Str,"%lld",&i))
		return 0;
	return i;

}
string cl_httpc::ip_format(const char* ip_or_dns)
{
	if(INADDR_NONE != inet_addr(ip_or_dns))
	{
		return ip_or_dns;//就是ip
	}
	else
	{
		in_addr sin_addr;
		hostent* host;
		host = gethostbyname(ip_or_dns);
		if (host == NULL) {
			return ip_or_dns;
		}
		sin_addr.s_addr = *((unsigned long*)host->h_addr);
		return inet_ntoa(sin_addr);
	}
}

int httpc_set_blocking(SOCKET s,bool bBlocking)
{
#ifdef _WIN32
	//NONBLOCKING=1
	u_long val = bBlocking?0:1;
	if(INVALID_SOCKET!=s)
		return ioctlsocket(s,FIONBIO,&val);
	return -1;
#else
	int opts;
	opts = fcntl(s,F_GETFL);
	if(-1 == opts)
	{
		perror("fcntl(s,GETFL)");
		return -1;
	}
	if(!bBlocking)
		opts |= O_NONBLOCK;
	else
		opts &= ~O_NONBLOCK;
	if(-1 == fcntl(s,F_SETFL,opts))
	{
		printf("***error s=%d ***\n",s);
		perror("fcntl(s,SETFL,opts); ");
		return -1;
	}
	return 0;
#endif
}

#ifdef _WIN32
int httpc_is_connected(SOCKET s,fd_set *rd,fd_set *wr,fd_set *ex)
{
	WSASetLastError(0);
	if ( !FD_ISSET(s, rd) && !FD_ISSET(s, wr) )
		return 0;
	if (FD_ISSET(s, ex))
		return 0;
	return 1;
}
#else
int httpc_is_connected(SOCKET s,fd_set *rd,fd_set *wr,fd_set *ex)
{
	int err;
	socklen_t len = sizeof( err );

	errno = 0;
	if ( !FD_ISSET( s, rd ) && !FD_ISSET( s, wr ) )
		return 0;
	if ( getsockopt( s, SOL_SOCKET, SO_ERROR, &err, &len ) < 0 )
		return 0;
	errno = err;
	return err == 0;
}
#endif


string cl_httpc::ip_explain(const char* s)
{
	if(INADDR_NONE != inet_addr(s))
	{
		return s;
	}
	else
	{
		in_addr sin_addr;
		hostent* host = gethostbyname(s);
		if (host == NULL) 
		{
			return s;
		}
		sin_addr.s_addr = *((unsigned long*)host->h_addr);
		return inet_ntoa(sin_addr);
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
DWORD WINAPI cl_httpc::ip_explain_ex_t1(void *p)
#else
void* cl_httpc::ip_explain_ex_t1(void *p)
#endif
{
	IPFD *ipf = (IPFD*)p;
	ipf->ip = ip_explain(ipf->dns.c_str());
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
string cl_httpc::ip_explain_ex(const char* s,int maxTick/*=5000*/)
{
	string ip;
	if(INADDR_NONE != inet_addr(s))
	{
		return s;//就是ip
	}
	else
	{
		IPFD *ipf = new IPFD();
		if(!ipf)
			return s;
		ipf->dns = s;
#ifdef _WIN32
		DWORD thid=0;
		HANDLE h = CreateThread(NULL,0,cl_httpc::ip_explain_ex_t1,(void*)ipf,0,&thid);
		if(INVALID_HANDLE_VALUE==h)
			return s;
		else
			CloseHandle(h);
#else
		pthread_t hthread;
		if(0==pthread_create(&hthread,NULL,cl_httpc::ip_explain_ex_t1,(void*)ipf))
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
		return s;
	}
}
