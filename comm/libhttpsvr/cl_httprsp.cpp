#include "cl_httprsp.h"
#include "cl_util.h"
#include "cl_net.h"
#include "cl_urlcode.h"
#include "HttpResponseHeader.h"
#include "HttpContentType.h"
#include "cl_RDBEifFile64.h"

bool g_https_exiting=false;

cl_httprsp::cl_httprsp(cl_httppubconf_t* c)

{
	m_req = new cl_HttpRequest_t();
	m_req->fun_param = c->fun_param;
	m_pubconf = c;
}

cl_httprsp::~cl_httprsp(void)
{
	delete m_req;
}
int cl_httprsp::handle_req(SOCKET sock,sockaddr_in &addr)
{
	cl_HttpRequest_t* req = m_req;
	req->fd = (int)sock;
	req->addr.s_addr = addr.sin_addr.s_addr;
	int timeoutms = 8000;
#ifndef ANDROID
	cl_net::sock_set_timeout(sock, 8000);
#endif
	do
	{
		//只支持Get 方法头，不支持头含数据，也就是收到"\r\n\r\n"后认为是一个完整请求
		req->reset();

		if(0!=recv_head(req,timeoutms))
			break;

		//如果body数据比较少，则收完它
		if(req->Content_Length>req->bodylen && req->Content_Length<=CL_HTTP_MAX_HEADLEN)
		{
			if(0!=cl_net::sock_select_recv_n(req->fd,req->body+req->bodylen,(int)(req->Content_Length-req->bodylen),timeoutms))
				break;
			req->bodylen = (int)req->Content_Length;
		}
		req->body[req->bodylen] = '\0';
		
		if(0!=handle_head(req, m_pubconf->fun_handle_http_req)) //返回0才表示正确完整处理，才可能继续循环处理(如果keepalive)
			break;
	}while(m_pubconf->bsupport_keepalive && is_keeplive(req->header));
	return 0;
}
int cl_httprsp::recv_head(cl_HttpRequest_t* req,int timeoutms)
{
	int n = 0;
	int readsize = 0;
	DWORD begintick = GetTickCount();
	char *p = NULL;
	char* buf = req->header;
	while(readsize<CL_HTTP_MAX_HEADLEN)
	{
		if(1==cl_net::sock_select_readable(req->fd,timeoutms))
		{
			n = recv(req->fd,buf+readsize,CL_HTTP_MAX_HEADLEN-readsize,0);
			if(n<=0)
			{
				break;
			}
			readsize+=n;
			buf[readsize] = '\0';
			p = strstr(buf,"\r\n\r\n");
			if(p)
			{
				req->bodylen = readsize - (int)(p-buf+4);
				assert(req->bodylen>=0);
				if(req->bodylen>0)
				{
					memcpy(req->body,p+4,req->bodylen);
				}
				p[4] = '\0';
				return 0;
			}
		}

		//超时 10秒
		if(_timer_after(GetTickCount(),(begintick + timeoutms)))
			break;
	}
	return -1;
}
int cl_httprsp::handle_head(cl_HttpRequest_t* req,FUN_HANDLE_HTTP_REQ_PTR fun)
{
	//DEBUGMSG("request------- \n %s \n",header);
	string text = "";
	if(0==get_header_field(req->header,"Content-Length",text))
	{
		req->Content_Length = cl_util::atoll(text.c_str());
	}
	char buf[CL_HTTP_MAX_HEADLEN+1];
	char* ptr = strchr(req->header,'\r');
	if(NULL==ptr) return -1;
	memcpy(buf,req->header,(int)(ptr-req->header));
	buf[(int)(ptr-req->header)] = '\0';
	string src = buf;
	string str;
	str = cl_util::get_string_index(src,0," ");
	if(str.empty()||str.length()>7) return -1;
	strcpy(req->method,str.c_str());
	str = cl_util::get_string_index(src,1," ");
	if(str.empty()||str.length()>CL_HTTP_MAX_HEADLEN) return -1;
	str = cl_urldecode(str);
	int pos = (int)str.find("?");
	if(pos>0)
	{
		strcpy(req->cgi,str.substr(0,pos).c_str());
		strcpy(req->params,str.substr(pos).c_str());
	}
	else if(pos==0)
	{
		return -1;
	}
	else
	{
		strcpy(req->cgi,str.c_str());
		req->params[0]='\0';
	}

	if(strstr(req->cgi,"/version") && !m_pubconf->ver.empty())
	{
		char rspbuf[1024];
		sprintf(rspbuf, "ver=%s<br>"
			"begintime=%s<br>"
			"bmulti_thread=%d<br>"
			"bsupport_keepalive=%d<br>"
			"max_client_num=%d<br>"
			"current_clients=%d<br>"
			"request_amount=%d<br>"
			, m_pubconf->ver.c_str(), m_pubconf->begin_time.c_str(), m_pubconf->multi_thread?1:0,
			m_pubconf->bsupport_keepalive?1:0, m_pubconf->max_client_num,
			m_pubconf->client_num, m_pubconf->request_amount);
		response_message(req->fd, rspbuf);
		return 0;
	}
	else if(fun)
	{
		return fun(req);
	}
	else
	{
		response_error((int)req->fd);
		return -1;
	}
}

int cl_httprsp::get_header_field(const string& header,const string& session, string& text)
{
	//取得某个域值,session 不带":"号
	if(header.empty()) 
		return -1;
	int nPos = -1;
	nPos = (int)header.find(session,0);
	if(nPos != -1)
	{
		nPos += (int)session.length();
		nPos += 1; //加1忽略:号
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
int cl_httprsp::get_header_range(const string& header,ULONGLONG& ibegin,ULONGLONG& iend)
{
	string str;
	if(0==get_header_field(header,"Range",str))
	{
		str = cl_util::get_string_index(str,1,"=");
		ibegin = cl_util::atoll(cl_util::get_string_index(str,0,"-").c_str());
		iend = cl_util::atoll(cl_util::get_string_index(str,1,"-").c_str());
		return 0;
	}
	else
	{
		return -1;
	}
}
bool cl_httprsp::is_keeplive(const string& header)
{
	//是否keep-alive
	string str;
	if(0==get_header_field(header,"Connection",str))
	{
		if(0==stricmp(str.c_str(),"keep-alive"))
		{
			DEBUGMSG("# cl_httprsp::req \"keep-alive\" \n");
			return true;
		}
	}
	return false;
}

void cl_httprsp::response_error(int fd,const char* msg/*=NULL*/,int code/*=404*/,int timeoutms/*=10000*/)
{
	printf("response %d",code);

	HttpResponseHeader responseHdr;

	string str = "error";
	if(msg)
		str = msg;
    
    SYSTEMTIME st;
	GetLocalTime(&st);

	responseHdr.AddStatusCode(code);
    responseHdr.AddDate(st);
    responseHdr.AddServer("sphttpsvr");
    responseHdr.AddMyAllowFields();
	responseHdr.AddContentLength((int)str.length());
    responseHdr.AddContentType("text/html");

	unsigned int tick = GetTickCount();
	int distance = 0;
    if(responseHdr.Send(fd,timeoutms))
	{
		distance = _timer_distance(GetTickCount(),tick);
		if(timeoutms>= distance)
			cl_net::sock_select_send_n(fd,str.c_str(),(int)str.length(),timeoutms-distance);
	}
	cl_net::sock_select_readable(fd, 1000);
}
void cl_httprsp::response_message(int fd,const char* msg,int len/*=-1*/,int code/*=200*/,int timeoutms/*=10000*/)
{
	HttpResponseHeader responseHdr;
	if(len<0)
	{
		if(msg)
			len = (int)strlen(msg);
		else
			len = 0;
	}
	if(0==len) code = 204;
    
    SYSTEMTIME st;
	GetLocalTime(&st);

	responseHdr.AddStatusCode(code);
    responseHdr.AddDate(st);
    responseHdr.AddServer("sphttpsvr");
    responseHdr.AddMyAllowFields();
	responseHdr.AddContentLength(len);
    responseHdr.AddContentType("text/html");

	unsigned int tick = GetTickCount();
	int distance = 0;
    if(responseHdr.Send(fd,timeoutms))
	{
		if(len>0)
		{
			distance = _timer_distance(GetTickCount(),tick);
			if(timeoutms>= distance)
				cl_net::sock_select_send_n(fd,msg,len,timeoutms-distance);
		}
	}
	cl_net::sock_select_readable(fd, 2000);
}
void cl_httprsp::response_message(int fd,const string& msg,int code/*=200*/,int timeoutms/*=10000*/)
{
	response_message(fd,msg.c_str(),(int)msg.length(),code,timeoutms);
}

void cl_httprsp::response_file(int fd,const string& path)
{
	cl_file64 file;
	if(0!=file.open(path.c_str(),F64_READ))
	{
		printf("#***httpsvr no file (%s) ;\n",path.c_str());
		response_error(fd);
		return;
	}
	size64_t size = file.seek(0,SEEK_END);
	if(size<=0)
	{
		response_error(fd);
		return;
	}
	file.seek(0,SEEK_SET);

	HttpResponseHeader responseHdr;
	SYSTEMTIME tm;
	GetLocalTime(&tm);

	responseHdr.AddStatusCode(200);
	responseHdr.AddString("Access-Control-Allow-Origin: *\r\n");
    responseHdr.AddDate(tm);
    responseHdr.AddServer("sphttpsvr");
    responseHdr.AddMyAllowFields();
	responseHdr.AddContentLength((long long)size);
	responseHdr.AddContentType(http_content_type[cl_util::get_filename_extension(path)]);
	responseHdr.Send(fd,10000);

	const int READLEN = 64 * 1024;
	char *buf=new char[READLEN];
	size64_t rdsize = 0;
	//
	int n = 0;
	while(rdsize<size)
	{
		n = file.read(buf, READLEN);
		if(n<=0)
			break;
		if(0!=cl_net::sock_send_n(fd,buf,n))
			break;
		rdsize += n;
	}
	delete[] buf;
	file.close();
	cl_net::sock_select_readable(fd,3000);//最多等10秒钟，当对方收完关闭时，会可读。
}

void cl_httprsp::response_rdbfile(cl_HttpRequest_t* req,const string& path)
{
	cl_ERDBFile64 file;
	int fd = req->fd;
	size64_t size = rdbeif_get_mainfile_size(path.c_str());

	if(0==size || 0!=file.open(path.c_str(),F64_READ,RDBF_AUTO))
	{
		response_error(fd);
		DEBUGMSG("#*** cl_httprsp::response_rdbfile(%s) file open fail! \n",path.c_str());
		return;
	}

	//content_type
	string content_type = http_content_type[cl_util::get_filename_extension(req->cgi)];
	//range
	bool brange = false;
	size64_t ibegin = 0,iend = size-1;
	size64_t rsp_size = 0,pos=0;
	string str;
	if(0==get_header_field(req->header,"Range",str))
	{
		brange = true;
		str = cl_util::get_string_index(str,1,"=");
		ibegin = cl_util::atoll(cl_util::get_string_index(str,0,"-").c_str());
		iend = cl_util::atoll(cl_util::get_string_index(str,1,"-").c_str());
	}
	if(0==iend)
		iend = size-1;
	rsp_size = iend+1-ibegin;

	//response header
	HttpResponseHeader responseHdr;
	SYSTEMTIME tm;
	GetLocalTime(&tm);

	if (brange)
		responseHdr.AddStatusCode(206);
	else
		responseHdr.AddStatusCode(200);
	responseHdr.AddString("Access-Control-Allow-Origin: *\r\n");
    responseHdr.AddDate(tm);
	responseHdr.AddString("Accept-Ranges: bytes\r\n");
    responseHdr.AddServer("fileserver");
	responseHdr.AddContentLength((long long)rsp_size);
	if(brange)
	{
		char s[256];
		sprintf(s,"Content-Range: bytes %lld-%lld/%lld\r\n", ibegin, iend, size);
		responseHdr.AddString(s);
	}
	responseHdr.AddContentType(content_type.c_str());
	if(is_keeplive(req->header))
		responseHdr.AddString("Keep-Alive: timeout=15, max=100\r\nConnection: Keep-Alive\r\n");
	else
		responseHdr.AddString("Connection: close\r\n");
	responseHdr.Send(fd,10000);

	//send data:
	const int READLEN = 64 * 1024;
	char *buf=new char[READLEN];
	int n = 0;
	pos = ibegin;
	DEBUGMSG("# cl_httprsp::response_rdbfile::(beg=%lld,end=%lld,size=%lld) \n"
		,ibegin,iend,size);
	file.seek(pos,SEEK_SET);
	
	while(pos <= iend && !g_https_exiting)
	{
		n = READLEN;
		if((size64_t)n>(iend-pos+1))
			n = (int)(iend-pos+1);
		n = file.read(buf,n);
		if(n<=0)
			break;
		if(0!=cl_net::sock_send_n(fd,buf,n))
			break;
		pos += n;
	}
	delete[] buf;
	
	file.close();
	Sleep(100);
	DEBUGMSG("# cl_httprsp::response_rdbfile:: send end(pos=%lld, iend=%lld) \n",
		pos,iend);
	cl_net::sock_select_readable(fd,3000);
}

