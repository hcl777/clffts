#include "cl_lic.h"
#include "cl_xml.h"
#include "cl_bstream.h"
#include "cl_aes.h"
#include "cl_rsa.h"
#include "cl_ntp.h"

int cllic_lic_to_xmlstr(const cl_licinfo_t *lic,char* xmlbuf,int size)
{
	cl_xml xml;
	cl_xmlNode *node;
	node = xml.add_node("license");
	if(!node)
		return -1;
	xml.add_child(node,"peer_name",lic->peer_name.c_str());
	xml.add_child(node,"end_time",lic->end_time.c_str());
	xml.add_child(node,"ntp_server",lic->ntp_server.c_str());
	if(size<xml.size())
		return -1;
	return xml.to_string(xmlbuf,size); //0成功
}
int cllic_xmlstr_to_lic(const char* xmlbuf, cl_licinfo_t *lic)
{
	cl_xml xml;
	cl_xmlNode *node;
	char buf[1024];
	if(0!=xml.load_string(xmlbuf))
	{
		return -1;
	}
	node = xml.find_first_node("license");
	if(NULL==node)
	{
		return -1;
	}
	node = node->child();
	lic->peer_name = xml.find_first_node_data(node,"peer_name","",buf,1024);
	lic->end_time = xml.find_first_node_data(node,"end_time","",buf,1024);
	lic->ntp_server = xml.find_first_node_data(node,"ntp_server","",buf,1024);
	return 0;
}

int cllic_lic_to_rsabuf(const cl_licinfo_t *lic,unsigned int n,unsigned int e,char* rsabuf,int* size)
{
	//将license 生成xml,并aes,并将aeskey 用rsa加密保存到头部.
	//头:4B stx + 4B size(all) + 1B aeskey_rsalen + aeskey_rsa
	do
	{
		if(*size < 64) break;
		char *p;
		int pos = 0;
		int realsize = 0;
		int keylen = 1024;
		unsigned char key[16];
		cl_aes aes;

		aes.rand_key(key,16);
		aes.set_key(key,16*8);
		if(0!=cl_rsa32_encrypt(n,e,(char*)key,16,rsabuf+9,&keylen))
			break;
		memcpy(rsabuf,"[CA]",4);
		rsabuf[8] = keylen;
		pos = 9+rsabuf[8];
		p = rsabuf + pos;
		if(cllic_lic_to_xmlstr(lic,p,*size-pos)!=0)
			break;
		realsize = (int)strlen(p) + 1;//串结束符也算.
		aes.encrypt_n((unsigned char*)p,realsize);
		realsize += pos; 
		*size = realsize;
		*(unsigned int*)(rsabuf+4) = cl_bstream::htob32(realsize);
		return 0;
	}while(0);
	return -1;
}
int cllic_rsabuf_to_lic(unsigned int n,unsigned int d,char* rsabuf,int size,cl_licinfo_t *lic)
{
	do
	{
		//头:4B stx + 4B size(all) + 1B aeskey_rsalen + aeskey_rsa
		char* p;
		int realsize = 0;
		cl_aes aes;
		char aeskey[128];
		int aeskey_len = 128;
		if(size<9) break;
		if(0!=memcmp(rsabuf,"[CA]",4)) break;
		realsize = cl_bstream::btoh32(*(unsigned int*)(rsabuf+4));
		if(realsize != size) break;
		size -= (9+rsabuf[8]);
		p = rsabuf + (9+rsabuf[8]);
		if(size<=0) break;

		//解key
		if(0!=cl_rsa32_decrypt(n,d,rsabuf+9,rsabuf[8],aeskey,&aeskey_len))
			break;
		aes.set_key((unsigned char*)aeskey,aeskey_len*8);
		aes.decrypt_n((unsigned char*)p,size);
		//p[size] = '\0'; //加密时已经含结束符.

		//xml load
		if(0!=cllic_xmlstr_to_lic(p,lic))
			break;
		
		return 0;
	}while(0);
	return -1;
}
int cllic_lic_to_file(const cl_licinfo_t *lic,const char* path,bool baes)
{
	FILE* fp;
	char buf[4096];
	char *p;
	int aeskeylen = 16; //取16
	int headlen = aeskeylen+9;
	int size=0;
	int writesize = 0;
	int n = 0;
	p = buf+headlen;
	if(0!=cllic_lic_to_xmlstr(lic,p,4096-headlen))
		return -1;
	size = (int)strlen(p)+1;
	if(baes)
	{
		//
		cl_aes aes;
		unsigned char* aeskey = (unsigned char*)buf+9;
		cl_aes::rand_key(aeskey,aeskeylen);
		aes.set_key(aeskey,aeskeylen*8);
		for(int i=0;i<aeskeylen;++i)
		{
			aeskey[i] = ~aeskey[i];
		}
		memcpy(buf,"[CA]",4);
		buf[8] = aeskeylen;
		aes.encrypt_n((unsigned char*)p,size);
		size += headlen;
		p = buf;
		*(unsigned int*)(buf+4) = cl_bstream::htob32(size);
	}

	fp = fopen(path,"wb+");
	if(fp)
	{
		while(writesize<size)
		{
			n = (int)fwrite(p+writesize,1,size-writesize,fp);
			if(n<=0)
				break;
			writesize += n;
		}
		fclose(fp);

	}
	return 0;
}
int cllic_file_to_lic(const char* path,cl_licinfo_t *lic,bool baes)
{
	FILE* fp;
	char buf[4096];
	char *p;
	int size=0;
	int n = 0;
	fp = fopen(path,"rb");
	if(fp!=NULL)
	{
		while(!feof(fp)&&size<4095)
		{
			n = (int)fread(buf+size,1,4095-size,fp);
			if(n<=0) 
			{
				break;
			}
			size+=n;
		}
		fclose(fp);
	}
	else
	{
		return -1;
	}

	if(size<9)
	{
		printf("#***ca file size faild(%d)!\n",size);
		return -1;
	}
	
	buf[size] = '\0';
	p = buf;
	if(baes)
	{
		//16B加密,前16字节
		cl_aes aes;
		if(0!=memcmp(buf,"[CA]",4))
		{
			printf("#***ca file stx faild!");
			return -1;
		}
		int realsize = cl_bstream::btoh32(*(unsigned int*)(buf+4));
		if(size!=realsize)
		{
			printf("#***ca file realsize faild(%d)!",realsize);
			return -1;
		}
		unsigned char* aeskey = (unsigned char*)buf+9;
		int aeskeylen = buf[8];
		int headlen = 9+aeskeylen;
		p = buf+headlen;
		size -= headlen;
		for(int i=0;i<aeskeylen;++i)
		{
			aeskey[i] = ~aeskey[i];
		}
		aes.set_key(aeskey,aeskeylen*8);
		aes.decrypt_n((unsigned char*)p,size);
	}

	return cllic_xmlstr_to_lic(p,lic);
}

//****************************************************************************
#include "cl_httpc.h"
#include "cl_util.h"
#include "cl_net.h"

cl_licClient::cl_licClient(void)
	:m_binit(false)
{
}


cl_licClient::~cl_licClient(void)
{
}

int cl_licClient::run(const string& peer_name,const string& ver,func_lic_fail func,int check_timer_sec)
{
	if(m_binit)
		return 1;
	m_server = "htracker.imovie.com.cn:9980";
	m_peer_name = peer_name;
	m_ver = ver;
	m_func = func;
	m_check_timer_sec = check_timer_sec;
	m_binit = true;
	activate();
	return 0;
}
void cl_licClient::end()
{
	if(!m_binit)
		return;
	m_binit = false;
	wait();
}
int cl_licClient::work(int e)
{
	int n = 0;
	Sleep(2000);
	check_lic();
	while(m_binit)
	{
		Sleep(1000);
		if(++n >= m_check_timer_sec)
		{
			n = 0;
			check_lic();
		}
	}
	return 0;
}
void cl_licClient::check_lic()
{
	cl_licinfo_t lic;
	char url[1024];
	char sret[2048];
	int size = 2048;
	string strtime;
	unsigned int rsa_n,rsa_e,rsa_d;
	string path = cl_util::get_module_dir() + ".core";
	time_t t=0;

	//try get http lic
	cl_rsa32_randkey(rsa_n,rsa_e,rsa_d);
	sprintf(url,"http://%s/lic/licd.php?peer_name=%s&ver=%s&n=%d&e=%d"
		,m_server.c_str(),m_peer_name.c_str(),m_ver.c_str()
		,(int)rsa_n-rsa_e,(int)rsa_e);
	if(0==cl_httpc::http_get_buffer(url,sret,size))
	{
		if(0==cllic_rsabuf_to_lic(rsa_n,rsa_d,sret,size,&lic))
			cllic_lic_to_file(&lic,path.c_str(),true);
	}

	//try load from file
	if(lic.peer_name.empty())
		cllic_file_to_lic(path.c_str(),&lic,true);

	//DEBUGMSG("#LICD(%s,%s,%s)\n",lic.peer_name.c_str(),lic.end_time.c_str(),lic.ntp_server.c_str());
	// end_time = 空时表示没有授权
	if(lic.peer_name != m_peer_name || lic.end_time.empty())
		goto fail;

	//尝试获取时间
	t = (time_t)cl_ntp_get_sec1970(lic.ntp_server.c_str());
	if(t>0)
		strtime = cl_util::time_to_date_string(t);
	else
		strtime = cl_util::time_to_date_string(time(NULL));
	if(strcmp(strtime.c_str(),lic.end_time.c_str())>0)
		goto fail;
	//DEBUGMSG("#LICDok\n");
	return;
fail:
	//DEBUGMSG("#LICDFAIL\n");
	m_func();
}



