#pragma once
#include "cl_basetypes.h"

class cl_httpc
{
private:
	cl_httpc(void);
	~cl_httpc(void);
public:
#define HTTPC_MAX_HEADLEN 2047
	typedef struct tag_httpc_response
	{
		char header[HTTPC_MAX_HEADLEN+1];
		int retcode;
		unsigned long long Content_Length;
		char body[HTTPC_MAX_HEADLEN+1]; //记录多数的数据
		unsigned long long bodylen;
		char *pbody;
		tag_httpc_response(void)
			:retcode(0)
			,Content_Length(0)
			,bodylen(0)
			,pbody(NULL)
		{
		}
		~tag_httpc_response(void)
		{
			if(pbody)
				delete[] pbody;
		}
	}httpc_response_t;

public:
	//有data时使用post,
	static int request(const string& url,const char* body,int bodylen,string& rspheader,string& rspbody);
	static int download_file(const string& url,const string& filepath);
	static int http_get(const string& url,char* sret,int retlen);
	//返回大小
	static int http_get_buffer(const string& url,char* sret,int& iolen);

public:
	static int http_open_request(const string& url,const char* data,int datalen);
	static int http_recv_header(int fd,httpc_response_t* rsp);
	static int http_recv_data(int fd,char* buf,int size);
	static int http_close(int fd);
	static int send_n(int fd,const char *buf,int size);

	static int get_field(const string& header,const string& session, string& text);
	static int url_element_split(const string& url,string& server,unsigned short& port,string& cgi);
	static int format_header(string& header,const string& server,const string& cgi,int bodylen);
	static int get_server_response_code(const string& header);

	static long long httpc_atoll(const char* _Str);
	static string ip_format(const char* ip_or_dns);

#ifdef _WIN32
	static DWORD WINAPI ip_explain_ex_t1(void *p);
#else
	static void* ip_explain_ex_t1(void *p);
#endif
	static string ip_explain(const char* s);
	static string ip_explain_ex(const char* s,int maxTick=10000);
	
};
