#pragma once

#include "cl_basetypes.h"
#include "cly_trackerProto.h"

enum CLY_CTRACKER_CMD
{
	CLY_CT_LOGIN=1,
	CLY_CT_KEEPALIVE,
	CLY_CT_SEARCH_SOURCE,

	CLY_CT_REPORT_ERROR,
	CLY_CT_REPORT_NATTYPE,
	CLY_CT_REPORT_PROGRESS, //主动定时取
	CLY_CT_REPORT_FDFILE,	//未报记录缓存
	
	CLY_CT_GET_DDLIST,
	//CLY_CT_GET_FINI_CHSUM,
	CLY_CT_GET_FINIFILES,
	CLY_CT_PUT_FINIFILES,
};



int cly_tracker_http_get(const char* url,cl_xml& xml,cly_seg_error_t& seg_err,int max_rsp_size);
int cly_tracker_http_post(const char* url,const char* body,int bodylen,cl_xml& xml,cly_seg_error_t& seg_err);
int cly_tracker_http_post_file(const char* url,const char* path,cl_xml& xml,cly_seg_error_t& seg_err);

