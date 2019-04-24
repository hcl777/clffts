#pragma once
#include "cl_basetypes.h"
#include "cl_ntypes.h"
#include "cl_util.h"
#include "cl_net.h"

typedef struct tag_cly_source
{
	string			straddr;
	uint64			id;
	int				user_type;
	uint32			ip;
	uint16			port;
	int				ntype;
	unsigned int	last_use_tick;
	unsigned int	failed_times;

	tag_cly_source(void)
		:id(0)
		,user_type(0)
		,ip(0)
		,port(0)
		,ntype(0)
		,last_use_tick(0)
		,failed_times(0)
	{}
}cly_source_t;
int cly_soucre_str_section(const string& s,uint32& ip,uint16& port,int& ntype,uint32& private_ip,uint16& private_port,int& user_type);

typedef struct tag_cly_readyInfo
{
	string hash;
	int  ftype;  //下载类型
	string path;
	string name;
	string subhash;
}cly_readyInfo_t;

typedef struct tag_cly_downloadInfo
{
	string hash;
	int  ftype;  //下载类型
	string path;
	uint64 size;
	int progress;//完成的千分比
	int srcNum;
	int connNum;
	int speedB;
	int state;   //0:停止,1:下载,2:下载完整

	tag_cly_downloadInfo(void)
	{
		ftype = 0;
		size = 0;
		progress = 0;
		srcNum = 0;
		connNum = 0;
		speedB = 0;
		state = 0;
	}
}cly_downloadInfo_t;

typedef struct tag_clyd_state
{
	string	peer_name;
	int		peer_id;
	int		used;
	int		login_state;
	int		login_fails;
	int		keepalive_fails;
	int		downauto_pause;
	int		unused_down_times; //
	int		ready_num;
	int		free_space_GB;//下载目录的剩余空间
	int		downconn_num; //所有下载共建立的连接数
	int		down_speed;
	int		up_speed;

	//uac
	int sendspeedB;
	int valid_sendspeedB;
	int recvspeedB;
	int valid_recvspeedB;
	int app_recvspeedB;
	int sendfaild_count;
	int	sendfaild_errs[8];

	list<string> downi_speeds;
	list<string> upi_speeds;
}clyd_state_t;

