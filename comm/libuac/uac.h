#pragma once
#include "uac_UDPConfig.h"
//
//Aspera 专业做高速文件传输,2015年被IBM 2亿美元收购.
//http://asperasoft.com/zh/site/

//最大支持连接数
#define UAC_FD_SIZE 2048
#define UAC_SOCK_SENDBUF 1024000

typedef int UAC_SOCKET;

//后面有函数使用返回引用，不是C标志，所以不再使用C标准接口
//#ifdef __cplusplus
//extern "C"
//{
//#endif

//UAC_sockaddr使用主机序，不使用网络序
typedef struct tag_UAC_sockaddr
{
	unsigned int	ip;
	unsigned short	port;
	char			nattype; //0~6
}UAC_sockaddr;

typedef struct tag_UAC_sendspeed
{
	unsigned int	speedB;		 //实发总速度，包括重发包
	unsigned int	setspeedB;	 //算法速度
	unsigned int	wind_pks; //发送窗.
	unsigned int	other_wind_pks; //对方要求发送窗
	unsigned long long		allsendB;		//一共发了多少数据
	unsigned long long		resendB;

	//累计packets
	unsigned int	allsend_pks; //总发包量
	unsigned int	resend_pks;
	unsigned int	resend_timeo_pks;
	unsigned int	other_rerecv_pks; //对方真实重收包量
	unsigned int	second_maxttl_ms; //ms
	unsigned int	second_rerecv_pks;
	unsigned int	sendfaild_count;

	unsigned int	max_seg_num; //缓冲区最大号
	unsigned int	low_line; //下界

	tag_UAC_sendspeed()
	{
		speedB = 0;
		setspeedB = 0;
		wind_pks = 0;
		other_wind_pks = 0;
		allsendB = 0;
		resendB = 0;
		allsend_pks = 0;
		resend_pks = 0;
		resend_timeo_pks = 0;
		other_rerecv_pks = 0;
		second_maxttl_ms = 0;
		second_rerecv_pks = 0;
		sendfaild_count = 0;

		max_seg_num = 0;
		low_line = 0;
	}
}UAC_sendspeed_t;

typedef struct tag_UAC_recvspeed
{
	unsigned int speedB;
	unsigned int rerecv_rate; //%分比
	unsigned int seq_range; //最大seq与最小seq的差.
	unsigned int ttlms; //ms
}UAC_recvspeed_t;

typedef struct tag_UAC_statspeed
{
	int sendspeedB;
	int valid_sendspeedB;
	int recvspeedB;
	int valid_recvspeedB;
	int app_recvspeedB;
	int sendfaild_count;
	int	sendfaild_errs[8];
}UAC_statspeed_t;
//**********************************
//nattype(0~5): nat类型可连接通性匹配
//
//			nat0	nat1	nat2	nat3	nat4
// nat0		1		1		1		1		1
// nat1		1	   	1		1		1		1
// nat2		1		1		1		1		1
// nat3		1		1		1		1		0
// nat4		1		1		1		0		0
//
//**********************************


typedef struct tag_UAC_fd_set
{
	int				fd_count;
	UAC_SOCKET		fd_array[UAC_FD_SIZE];
}UAC_fd_set;

//setting
void uac_setcallback_onnatok(UAC_CALLBACK_ONNATOK fun);
void uac_setcallback_onipportchanged(UAC_CALLBACK_ONIPPORTCHANGED fun);
//get
int uac_get_nattype();
//UAC 只开一个物理端口，其它连接在此端口基础上模拟出来
int uac_init(unsigned short bindport,const char* stunsvr,unsigned short stunport,uac_config_t* conf,bool runthread=true); //指定UDP端口
int uac_fini();
void uac_loop();
int uac_set_mtu(unsigned int m);
int uac_get_conf(uac_config_t* conf);
int uac_set_conf(uac_config_t* conf);
int uac_get_statspeed(UAC_statspeed_t& ss);

//UAC socket 接口
UAC_SOCKET uac_accept(UAC_sockaddr* sa_client);
UAC_SOCKET uac_connect(const UAC_sockaddr* sa_client);
int uac_closesocket(UAC_SOCKET fd);
int uac_setbandwidth(UAC_SOCKET fd,int size);
int uac_setsendbuf(UAC_SOCKET fd,int size);
int uac_setrecvbuf(UAC_SOCKET fd,int size);
UAC_sendspeed_t& uac_getsendspeed(UAC_SOCKET fd);
UAC_recvspeed_t& uac_getrecvspeed(UAC_SOCKET fd);
bool uac_is_read(UAC_SOCKET fd);
bool uac_is_write(UAC_SOCKET fd);
bool uac_is_connected(UAC_SOCKET fd);
int uac_select(UAC_fd_set* rset,UAC_fd_set* wset);
int uac_send(UAC_SOCKET fd,const char* buf,int len);//返回:-1 关闭
int uac_recv(UAC_SOCKET fd,char* buf,int len); //返回:-1 关闭


//#ifdef __cplusplus
//}
//#endif

