#pragma once
#include "cl_basetypes.h"
#include "cl_xml.h"

#define CLY_PEER_TYPE_CLIENT 2

#define CLY_INVALID_VALUE -1


#define CLY_FDTYPE_DOWN 1
#define CLY_FDTYPE_DELETE 9

#define CLY_TERR_SUCCEED 0
#define CLY_TERR_DB_WRONG 1
#define CLY_TERR_NO_PEER 2
#define CLY_TERR_PEER_UNUSED 3
#define CLY_TERR_POSTBODY_SIZEOUT 4
#define CLY_TERR_RECVBODY_WRONG 5
#define CLY_TERR_BODY_WRONG 6
#define CLY_TERR_NO_SOURCE 7
#define CLY_TERR_TASK_NAME_LIST_EMPTY 8
#define CLY_TERR_NO_TASK 9
#define CLY_TERR_WRITE_FILE_FAILED 10
#define CLY_TERR_READ_FILE_FAILED 11

#define CLY_TERR_WRONG_PARAM 12
#define CLY_TERR_NO_FILE 13
#define CLY_TERR_CHSUM_DISTINCT 14
#define CLY_TERR_NO_CHANGE 16


#define CLY_ERR_COREDUMP			1001


//************************
//USED type
#define CLY_USED_ALL			0x01
#define CLY_USED_DOWNLOAD		0x02
#define CLY_USED_SHARE			0x04
#define CLY_USED_read_rdbfile	0x08

#define CLY_IS_USED(v,f) (v&f)

typedef struct tag_cly_account
{
	string name;
	string pass;
}cly_account_t;
typedef struct tag_cly_report_error
{
	string	peer_name;
	string	appname;
	string	systemver;
	string	appver;
	int		err;
	string description;
}cly_report_error_t;

typedef struct tag_cly_seg_error
{
	int			code;
	string		message;
}cly_seg_error_t;
typedef struct tag_cly_seg_config
{
	int			peer_id;
	int			group_id;
	int			peer_type;
	int			used; //>=1:used,0:不可用

	int			limit_share_speediKB;
	int			limit_share_speedKB;
	int			limit_down_speedKB;
	int			timer_keepaliveS;
	int			timer_getddlistS;	//获取任务周期
	int			timer_reportprogressS;	//上报进度周期

	tag_cly_seg_config(void)
	{
		peer_id = 0;
		group_id = 0;
		peer_type = 0;
		used = 0;
		limit_share_speediKB = 0;
		limit_share_speedKB = 0;
		limit_down_speedKB = 0;
		timer_keepaliveS = 0;
		timer_getddlistS = 0;
		timer_reportprogressS = 0;
	}
}cly_seg_config_t;
typedef struct tag_cly_seg_keepalive
{
	int 		peer_id;
	string		peer_name;
	string		upstate;
	string		downstate;
}cly_seg_keepalive_t;

typedef struct tag_cly_seg_keepalive_ack
{
	int			used;
}cly_seg_keepalive_ack_t;

typedef struct tag_cly_seg_fdfile
{
	int			fdtype;
	string		hash;
	int			ftype;
	string		name;
	string		subhash;
	string		rcvB;
}cly_seg_fdfile_t;


typedef struct tag_cly_seg_fchsum
{
	int						ff_num;
	unsigned long long		ff_checksum;
}cly_seg_fchsum_t;


typedef struct tag_cly_login_info
{
	string		peer_name;
	string		ver;
	string		addr;
}cly_login_info_t;

typedef struct tag_clyt_fileinfo
{
	int				file_id;
	string			hash;
	string			name;
	string			subhash;
	int				peer_num;
}clyt_fileinfo_t;


int cly_xml_get_seg_error(cl_xml& xml,cly_seg_error_t& seg);
int cly_xml_get_seg_config(cl_xml& xml,cly_seg_config_t& seg);
int cly_xml_get_seg_keepalive_ack(cl_xml& xml, cly_seg_keepalive_ack_t& seg);
int cly_xml_get_seg_clyt_fileinfo(cl_xml& xml,clyt_fileinfo_t& seg);
int cly_xml_get_list(cl_xml& xml,list<string>& ls,const char* idval);
int cly_xml_get_seg_fchsum(cl_xml& xml,cly_seg_fchsum_t& seg);
int cly_xml_get_seg_ddlist(cl_xml& xml,list<string>& ls);
int cly_xml_get_seg_task(const char* xmlstr,string& task_name,list<string>& ls_hids,list<string>& ls_files);

int cly_xml_get_report_progress(const char* xmlstr,int& peer_id,list<string>& ls);



#define cly_tp_xml_begin(xmlbuf) sprintf(xmlbuf,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<root>\r\n")
#define cly_tp_xml_end(xmlbuf) sprintf(xmlbuf+strlen(xmlbuf),"</root>")

#define cly_tp_xml_add_seg_error(xmlbuf,seg) sprintf(xmlbuf+strlen(xmlbuf),\
"	<seg id=\"error\">\r\n \
		<p n=\"code\">%d</p>\r\n \
		<p n=\"message\">%s</p>\r\n \
	</seg>\r\n",seg.code,seg.message.c_str())

#define cly_tp_rspxml_add_seg_keepalive_ack(xmlbuf,seg) sprintf(xmlbuf+strlen(xmlbuf),\
"	<seg id=\"keepalive_ack\">\r\n \
		<p n=\"used\">%d</p>\r\n \
	</seg>\r\n",seg.used)

#define  cly_tp_rspxml_add_seg_config(xmlbuf,seg) sprintf(xmlbuf+strlen(xmlbuf),\
"	<seg id=\"config\">\r\n \
		 <p n=\"peer_id\">%d</p>\r\n \
		 <p n=\"group_id\">%d</p>\r\n \
		 <p n=\"peer_type\">%d</p>\r\n \
		 <p n=\"used\">%d</p>\r\n \
		 <p n=\"limit_share_speediKB\">%d</p>\r\n \
		 <p n=\"limit_share_speedKB\">%d</p>\r\n \
		 <p n=\"limit_down_speedKB\">%d</p>\r\n \
		 <p n=\"timer_keepaliveS\">%d</p>\r\n \
		 <p n=\"timer_getddlistS\">%d</p>\r\n \
		 <p n=\"timer_reportprogressS\">%d</p>\r\n \
	</seg>\r\n",seg.peer_id,seg.group_id,seg.peer_type,seg.used,seg.limit_share_speediKB,seg.limit_share_speedKB,seg.limit_down_speedKB,seg.timer_keepaliveS,seg.timer_getddlistS,seg.timer_reportprogressS)

#define cly_tp_xml_add_seg_fchsum(xmlbuf,seg) sprintf(xmlbuf+strlen(xmlbuf),\
"	<seg id=\"fini_chsum\">\r\n \
		<p n=\"ff_num\">%d</p>\r\n \
		<p n=\"ff_checksum\">%lld</p>\r\n \
	</seg>\r\n",seg.ff_num,seg.ff_checksum)

#define cly_tp_xml_add_seg_clyt_fileinfo(xmlbuf,seg) sprintf(xmlbuf+strlen(xmlbuf),\
"	<seg id=\"fileinfo\">\r\n \
		<p n=\"file_id\">%d</p>\r\n \
		<p n=\"hash\">%s</p>\r\n \
		<p n=\"name\">%s</p>\r\n \
		<p n=\"subhash\">%s</p>\r\n \
		<p n=\"peer_num\">%d</p>\r\n \
	</seg>\r\n",seg.file_id,seg.hash.c_str(),seg.name.c_str(),seg.subhash.c_str(),seg.peer_num)


void cly_tp_rspxml_add_seg_list(char* xmlbuf,const char* idname,list<string>& ls);

