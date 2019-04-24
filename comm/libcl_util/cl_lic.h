#pragma once

#include "cl_basetypes.h"

/*========================================================================
cl_lic:
1. use httpget lic on rsa_buffer,if got ok, save to licd(aes),aeskey save in licd.
2. get svr datatime from time server,if failed use local.
3. load licd to check if ok.
4. one hourse timer to check.
========================================================================*/

typedef struct tag_cl_licinfo
{
	string peer_name;
	string end_time;
	string ntp_server; //时间服务器
}cl_licinfo_t;

//lic与xmlstr
int cllic_lic_to_xmlstr(const cl_licinfo_t *lic,char* xmlbuf,int size);
int cllic_xmlstr_to_lic(const char* xmlbuf, cl_licinfo_t *lic);

//lic与rsa_aes_buf
int cllic_lic_to_rsabuf(const cl_licinfo_t *lic,unsigned int n,unsigned int e,char* rsabuf,int* size); //size传入最大值，返回真实值
int cllic_rsabuf_to_lic(unsigned int n,unsigned int d,char* rsabuf,int size,cl_licinfo_t *lic); 

//lic与file
int cllic_lic_to_file(const cl_licinfo_t *lic,const char* path,bool baes);
int cllic_file_to_lic(const char* path,cl_licinfo_t *lic,bool baes);


//****************************************************************************
//cl_licClient
#include "cl_thread.h"
#include "cl_singleton.h"

typedef void (*func_lic_fail)();

class cl_licClient : public cl_thread
{
public:
	cl_licClient(void);
	virtual ~cl_licClient(void);

public:
	//检查lic失败的回调接口
	int run(const string& peer_name,const string& ver, func_lic_fail func,int check_timer_sec);
	void end();
	virtual int work(int e);
private:
	void check_lic();
private:
	bool			m_binit;
	string			m_server;
	string			m_peer_name;
	string			m_ver;
	func_lic_fail	m_func;
	int				m_check_timer_sec;
};
typedef cl_singleton<cl_licClient> cl_licClientSngl;


