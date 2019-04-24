#pragma once
#include "cl_basetypes.h"

class cl_httpc2
{
private:
	cl_httpc2(void);
	~cl_httpc2(void);

	
public:
	enum {CONTENT_TYPE_URLCODE=0,	//application/x-www-form-urlencoded ；表示普通表单格式上传
		CONTENT_TYPE_DATA,			// multipart/form-data  ；表示上传文件的规格
		CONTENT_TYPE_STREAM			//application/octet-stream ； 表示上传一个文件流数据
	};
	
#define HTTPC_MAX_HEADLEN 2047
	typedef struct tag_httpc2_response
	{
		char				header[HTTPC_MAX_HEADLEN+1];
		int					retcode;
		unsigned long long	Content_Length;
		char				body[HTTPC_MAX_HEADLEN+1]; //记录收头时多收的数据
		unsigned long long	bodylen;
		char				*pbody;

		tag_httpc2_response(void)
			:retcode(0)
			,Content_Length(0)
			,bodylen(0)
			,pbody(NULL)
		{
		}
		~tag_httpc2_response(void)
		{
			if(pbody)
				delete[] pbody;
		}
	}httpc2_response_t;

	//
	static int format_header(string& header,const string& server,const string& cgi,unsigned long long bodylen,int content_type);
	static int http_open_send_head(const string& url,unsigned long long bodylen,int content_type);
	static int http_recv_head(int fd,httpc2_response_t* rsp);
public:
	static int post_file(string& rspbody,const string& path,const string& url,int content_type=CONTENT_TYPE_URLCODE);

};

