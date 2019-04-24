#include "cly_cTrackerInfo.h"
#include "cl_httpc.h"
#include "cl_httpc2.h"



int cly_tracker_http_get(const char* url,cl_xml& xml,cly_seg_error_t& seg_err,int max_rsp_size)
{
	char *buf = new char[max_rsp_size];
	int ret = 0;
	if(0==cl_httpc::http_get(url,buf,max_rsp_size))
	{
		if(0==xml.load_string(buf))
		{
			if(0!=cly_xml_get_seg_error(xml,seg_err))
				ret = -3;
		}
		else
			ret = -2;
	}
	else
		ret = -1;
	delete[] buf;
	return ret;
}
int cly_tracker_http_post(const char* url,const char* body,int bodylen,cl_xml& xml,cly_seg_error_t& seg_err)
{
	string head,rspbody;
	int ret = 0;
	if(0==cl_httpc::request(url,body,bodylen,head,rspbody))
	{
		if(0==xml.load_string(rspbody.c_str()))
		{
			if(0!=cly_xml_get_seg_error(xml,seg_err))
				ret = -3;
		}
		else
			ret = -2;
	}
	else
		ret = -1;
	return ret;
}

int cly_tracker_http_post_file(const char* url,const char* path,cl_xml& xml,cly_seg_error_t& seg_err)
{
	string head,rspbody;
	int ret = 0;
	if(0==cl_httpc2::post_file(rspbody,path,url,cl_httpc2::CONTENT_TYPE_STREAM))
	{
		if(0==xml.load_string(rspbody.c_str()))
		{
			if(0!=cly_xml_get_seg_error(xml,seg_err))
				ret = -3;
		}
		else
			ret = -2;
	}
	else
		ret = -1;
	return ret;
}


