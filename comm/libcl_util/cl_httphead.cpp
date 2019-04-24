#include "cl_httphead.h"


cl_httphead::cl_httphead(void)
{
}


cl_httphead::~cl_httphead(void)
{
}
int cl_httphead::get_code(const string& header)
{
	//"HTTP/1.0 200 OK\r\n"; 
	int pos = (int)header.find(" ");
	if(pos<0)
		return -1;
	else
		return atoi(header.substr(pos+1).c_str());
}

int cl_httphead::get_field(const string& header,const string& session, string& text)
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
		return 0;
	}
	else
	{
		return -1;
	}
}
