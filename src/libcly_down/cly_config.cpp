#include "cly_config.h"
#include "cl_xml.h"
#include "cl_inifile.h"
#include "cl_util.h"
#include "cl_net.h"
#include "cl_crc32.h"
#include "cl_unicode.h"

#ifdef  _MSC_VER
#elif defined(__GNUC__)
#endif

string cly_get_hid_from_app()
{
	string strhid;
	char hid[128];
	memset(hid,0,128);

#ifdef __GNUC__
	char cmdline[256];
	sprintf(cmdline,"/usr/local/bin/hid");
	FILE *fp = popen(cmdline,"r");
	if(fp)
	{
		fread(hid,1,128,fp);
		pclose(fp);
	}
#endif
	strhid = hid;
	cl_util::string_trim_endline(strhid);
	return strhid;
}
string cly_get_default_hid()
{
	//mac + 路径,注意目前tracker,db均控制了32字符长度，需要加长则要全部改。
	char pid[32]={0,};
	unsigned char umac[10]; //只有6位
	unsigned char umac2[10];
	unsigned int cid[4];

	//test: peer_name = "95546900000045bb";

	string strhid = cly_get_hid_from_app();
	if(!strhid.empty())
		return strhid;

	memset(cid, 0, 16);
	cl_util::get_cpuid(cid);

	cl_net::get_umac(umac2);

	//crc32 path
	unsigned int crc32;
	string str = cl_util::get_module_path();
	crc32 = cl_crc32_write(CL_CRC32_FIRST,(unsigned char*)str.c_str(),str.length());

	
#ifdef _WIN32
	unsigned int cid_crc32 = cl_crc32_write(CL_CRC32_FIRST, (unsigned char*)cid, 16);
	memcpy(umac, umac2, 6);
	sprintf(pid, "W%08X%02X%02X%02X%02X%02X%02X%08X",
		cid_crc32,umac[0], umac[1], umac[2], umac[3], umac[4], umac[5], crc32);
#elif defined(ANDROID)
	memcpy(umac, umac2, 6);
	sprintf(pid, "%02X%02X%02X%02X%02X%02X%08X",
		umac[0], umac[1], umac[2], umac[3], umac[4], umac[5], crc32);
#else
	for (int i = 0; i<6; ++i)
	{
		umac[i] = ~(umac2[5 - i] + 77);
	}
	crc32 &= 0x7fffffff;
	sprintf(pid,"%02X%02X%02X%02X%02X%02X%d",umac[0],umac[1],umac[2],umac[3],umac[4],umac[5],crc32);
#endif
	return pid;
}


/*
<conf>
	<main> #主进程配置
		...
		<secondary> </secondary> #副进程
		<secondary> </secondary>
	</main>
</conf>
*/
void cly_process_config_default(cly_config_t& c)
{
	c.lang_utf8 = 0;
	char* lang = getenv("LANG");
	if(lang && strstr(lang,".UTF-8"))
		c.lang_utf8 = 1;

	c.main_process = 1;
	c.peer_name = "";
	c.used = 0xffffffff;
	c.peer_id = 0;
	c.group_id = 0;
	c.rootdir = cl_util::get_module_dir();
	c.server_tracker = "";
	c.server_stun = "";
	c.sn = "";
	c.http_port = 9880;
	c.http_threadnum = 5;

	c.share_path = "";
	c.share_suffix = "";

	c.down_path = "";
	c.down_encrypt = 0;
	c.down_active_num = 0;
	c.down_freespaceG = 20;

	c.upload_hids = "";

	c.limit_share_cnns = 500;
	c.limit_share_threads = 4;

	c.uac_mtu = 0;
	c.uac_port = 9200;
	c.uac_max_sendwin = 0;
	c.uac_sendtimer_ms = 0;
	c.uac_default_addr = "";

	c.cache_vod_disk = 0;
	c.cache_win_num = 100;

	//网络配置
	c.timer_report_progressS = -1;
	c.timer_get_ddlistS = -1;
}


int cly_config_load(cly_config_t& conf,const char* rootdir/*=NULL*/)
{
	cly_config_t *pc;
	cl_xml xml;
	cl_xmlNode *n;
	string str,xmlpath;
	const char *a;

	pc = &conf;
	cly_process_config_default(*pc);
	pc->rootdir = rootdir;
	if(pc->rootdir.empty())
		pc->rootdir = cl_util::get_module_dir();

	cl_util::dir_add_tail_symbol(pc->rootdir);

	xmlpath = pc->rootdir + "cly_down.xml";
	cl_util::str_replace(xmlpath,".exe","");

	if(0!=xml.load_file(xmlpath.c_str()))
	{
		return -1;
	}
	
	cly_dconf_load(pc->dconf, pc->rootdir + "cly_dconf.ini");

	n=xml.find_first_node("conf/main");
	n=n->child();

#define _XML_SEG_ATTRI_IF_STR(s) if(0==strcmp(a,#s)) pc->s = n->get_data();
#define _XML_SEG_ATTRI_ELSE_IF_STR(s) else if(0==strcmp(a,#s)) pc->s = n->get_data();

#define _XML_SEG_ATTRI_ELSE_IF_INT(s,dv) else if(0==strcmp(a,#s)) pc->s = cl_util::atoi(n->get_data(),dv);

	while(n)
	{
		a = n->attri.get_attri("n");
		if(a)
		{
			_XML_SEG_ATTRI_IF_STR(server_tracker)
			_XML_SEG_ATTRI_ELSE_IF_STR(server_stun)
			_XML_SEG_ATTRI_ELSE_IF_INT(http_port,0)
			_XML_SEG_ATTRI_ELSE_IF_INT(http_threadnum,5)
			_XML_SEG_ATTRI_ELSE_IF_STR(peer_name)
			_XML_SEG_ATTRI_ELSE_IF_STR(sn)

			//share,不指定后辍不共享
			else if(0==strcmp(a,"share_path"))
			{
				pc->share_suffix = n->attri.get_attri("suffix");
				if(!pc->share_suffix.empty())  pc->share_path = n->get_data();
			}
			else if(0==strcmp(a,"down_path"))
			{
				pc->down_path = n->get_data();
				pc->down_encrypt = cl_util::atoi(n->attri.get_attri("encrypt"),1);
				pc->down_active_num = cl_util::atoi(n->attri.get_attri("active"));
				pc->down_freespaceG = cl_util::atoi(n->attri.get_attri("freespace"));
				//if(pc->down_active_num<=0) pc->down_active_num = 3; //0则不下载
				if(pc->down_freespaceG<20) pc->down_freespaceG = 20;
			}

			_XML_SEG_ATTRI_ELSE_IF_STR(upload_hids)
			
			_XML_SEG_ATTRI_ELSE_IF_INT(limit_share_cnns,500)
			_XML_SEG_ATTRI_ELSE_IF_INT(limit_share_threads,4)

			_XML_SEG_ATTRI_ELSE_IF_INT(uac_mtu,0)
			_XML_SEG_ATTRI_ELSE_IF_INT(uac_port,0)
			_XML_SEG_ATTRI_ELSE_IF_INT(uac_max_sendwin, 0)
			_XML_SEG_ATTRI_ELSE_IF_INT(uac_sendtimer_ms, 0)
			_XML_SEG_ATTRI_ELSE_IF_STR(uac_default_addr)

			_XML_SEG_ATTRI_ELSE_IF_INT(timer_report_progressS,-1)
			_XML_SEG_ATTRI_ELSE_IF_INT(timer_get_ddlistS,-1) //0表示一定不取

			_XML_SEG_ATTRI_ELSE_IF_STR(url_oldhash_relation)
		}
		n = n->next();
	}

	xml.clear();

	if(pc->peer_name.empty())
		pc->peer_name = cly_get_default_hid();
	if (pc->http_threadnum < 1) pc->http_threadnum = 1;
	
	return 0;
}


void cly_config_sprint(cly_config_t* pc,char* buf,int size)
{
#define pi(name) sprintf(buf+strlen(buf),"%-22s = %d \n",#name,(int)pc->name)
#define ps(name) sprintf(buf+strlen(buf),"%-22s = %s \n",#name,pc->name.c_str())

//#ifndef ANDROID
//	string testcode = "中文编码测试!"; //android不支持中文字符串编译
//	if(pc->lang_utf8)
//		cl_unicode::GB2312ToUTF_8(testcode,testcode.c_str(),testcode.length());
//	printf("\n#[LANG %s]:: %s\n",testcode.c_str(),getenv("LANG"));
//#endif

	sprintf(buf,"[config]\n");
	pi(lang_utf8);

	ps(peer_name);
	ps(rootdir);
	ps(server_tracker);
	ps(server_stun);
	ps(sn);
	pi(http_port);
	pi(http_threadnum);

	ps(share_path);
	ps(share_suffix);

	ps(down_path);
	pi(down_encrypt);
	pi(down_active_num);
	pi(down_freespaceG);

	ps(upload_hids);

	pi(limit_share_cnns);
	pi(limit_share_threads);

	pi(uac_mtu);
	pi(uac_port);
	pi(uac_max_sendwin);
	pi(uac_sendtimer_ms);
	ps(uac_default_addr);

	cly_dconf_sprint(pc->dconf,buf);
		
	pi(timer_report_progressS);
	pi(timer_get_ddlistS);

}

//****************************************************************************
int cly_dconf_load(cly_dynamic_conf_t& dc, const string& path)
{
	cl_inifile ini;
	dc.use_local_limit = 0;
	if(0==ini.open(path.c_str()))
	{
		dc.use_local_limit = ini.read_int("limit","use_local_limit",0);
		dc.limit_share_speediKB = ini.read_int("limit","limit_share_speediKB",0);
		dc.limit_share_min_speediKB = ini.read_int("limit", "limit_share_min_speediKB", 0);
		dc.limit_share_speedKB = ini.read_int("limit","limit_share_speedKB",0);
		dc.limit_down_speedKB = ini.read_int("limit","limit_down_speedKB",0);
		ini.close();
	}
	return 0;
}

void cly_dconf_sprint(cly_dynamic_conf_t& dc,char* buf)
{
#define pi1(name) sprintf(buf+strlen(buf),"%-22s = %d\n",#name,(int)dc.name)
		pi1(limit_share_speediKB);
		pi1(limit_share_min_speediKB);
		pi1(limit_share_speedKB);
		pi1(limit_down_speedKB);
}

//*********************************************************

int cly_setting::init(cly_config_t* pc)
{
	string str;
	int i, n;
	m_pc = pc;
	m_info_path = m_pc->rootdir + "data/info/";
	m_log_path = m_pc->rootdir + "data/log/";
	m_share_path = m_pc->rootdir + "data/share/";
	m_auto_path = m_pc->rootdir + "data/auto/";
	m_tracker_path = m_pc->rootdir + "data/tracker/";
	
	cl_util::my_create_directory(m_info_path);
	cl_util::my_create_directory(m_log_path);
	cl_util::my_create_directory(m_share_path);
	cl_util::my_create_directory(m_auto_path);
	cl_util::my_create_directory(m_tracker_path);

	m_uacaddr.nattype = 6;
	m_uacaddr.ip = 0;
	m_uacaddr.port = pc->uac_port;
	m_local_ip = cl_net::get_local_private_ip_ex();

	//down_path list
	format_down_path(m_pc->down_path);

	//share_path list
	n = cl_util::get_string_index_count(m_pc->share_path,"|");
	for(i=0;i<n;++i)
	{
		str = cl_util::get_string_index(m_pc->share_path,i,"|");
		cl_util::string_trim(str);
		if(!str.empty())
		{
			cl_util::dir_add_tail_symbol(str);
			m_share_path_list.push_back(str);
		}
	}

	//sn
	n = cl_util::get_string_index_count(m_pc->sn,"|");
	for(i=0;i<n;++i)
	{
		str = cl_util::get_string_index(m_pc->sn,i,"|");
		cl_util::string_trim(str);
		if(!str.empty())
			m_sn_list.push_back(str);
	}
	return 0;
}


void cly_setting::format_down_path(const string& paths)
{
	//检查path_down_auto的空间情况，可能硬盘挂载不上的情况
	// /data1>50G|data2>60G
	string str,tmppath;
	int n=0;
	unsigned long long size = 0;
	string log_path = m_log_path + "conf_error.log";

	n = cl_util::get_string_index_count(paths,"|");
	m_down_path_list.clear();

	for(int i=0;i<n;++i)
	{
		str = cl_util::get_string_index(paths,i,"|");
		tmppath = cl_util::get_string_index(str,0,">");
		cl_util::string_trim(tmppath);
		if(tmppath.empty())
			continue;

		cl_util::my_create_directory(tmppath);
		//检查硬盘是否挂上
		size = cl_util::atoi(cl_util::get_string_index(str,1,">").c_str());
		if(size>0)
		{
			ULONGLONG total=0,used=0,free=0;
			cl_util::get_volume_size(tmppath,total,used,free);
			total = total >> 30;
			if(total<=size)
			{
				cl_util::write_tlog(log_path.c_str(),1024,"down_path:%s not mount!(total=%dG)",tmppath.c_str(),(int)total);
				continue;
			}
		}

		//检查目录是否可用
		cl_util::my_create_directory(tmppath);
		//GetStatus()
		char c = tmppath.at(tmppath.length()-1);
		if('/' != c && '\\' != c)
			tmppath += "/";
		str = tmppath + "test.d!!";

		FILE *fp = fopen(str.c_str(),"wb+");
		if(fp)
		{
			fclose(fp);
			cl_util::file_delete(str);
		}
		else
		{
			cl_util::write_tlog(log_path.c_str(),1024,"down_path:%s not unused!",tmppath.c_str());
			tmppath = "";
		}
		if(!tmppath.empty())
			m_down_path_list.push_back(tmppath);
	}
}
string cly_setting::find_exist_fullpath(const string& path)
{
	if(path.empty()) return "";
	if((path.length()>1&&path.at(1)==':') || (!path.empty()&&path.at(0)=='/'))
	{
		if(1==cl_util::file_state(path.c_str()))
			return path;
		return "";
	}
	list<string>::iterator it;
	string str;
	for(it=m_share_path_list.begin();it!=m_share_path_list.end();++it)
	{
		str = (*it)+path;
		if(1==cl_util::file_state(str.c_str()))
			return str;
	}
	for(it=m_down_path_list.begin();it!=m_down_path_list.end();++it)
	{
		str = (*it)+path;
		if(1==cl_util::file_state(str.c_str()))
			return str;
	}
	
	return "";
}
string cly_setting::find_new_fullpath(const string& path)
{
	//
	if((path.length()>1&&path.at(1)==':') || (!path.empty()&&path.at(0)=='/'))
		return path;

	string str,dir;
	ULONGLONG size=0,total=0,used=0,free=0;
	//找最大空间的dir
	for(list<string>::iterator it = m_down_path_list.begin();it!=m_down_path_list.end();++it)
	{
		total=0;used=0;free=0;
		cl_util::get_volume_size(*it,total,used,free);
		if(free>size)
		{
			dir = *it;
			size = free;
		}
	}
	if(dir.empty())
		return "";
	str = dir+path;
	//去掉/../
	//cl_util::filepath_format(str);
	return str;
}
string cly_setting::find_path_name(const string& path)
{
	string strret;
	if((path.length()>1&&path.at(1)==':') || (!path.empty()&&path.at(0)=='/'))
	{
		list<string>::iterator it;
		
		for(it=m_share_path_list.begin();it!=m_share_path_list.end();++it)
		{
			string& str = *it;
			if(0==path.find(str))
			{
				strret = path.substr(str.length());
				return strret;
			}
		}
		for(it=m_down_path_list.begin();it!=m_down_path_list.end();++it)
		{
			string& str = *it;
			if(0==path.find(str))
			{
				strret = path.substr(str.length());
				return strret;
			}
		}
		return cl_util::get_filename(path);
	}
	else
	{
		//已经是相对路径，直接返回
		return path;
	}
}
int cly_setting::get_freespace_GB()
{
	ULONGLONG all=0,total=0,used=0,free=0;
	for(list<string>::iterator it = m_down_path_list.begin();it!=m_down_path_list.end();++it)
	{
		total=0;used=0;free=0;
		if(cl_util::get_volume_size(*it,total,used,free))
			all += free;
	}
	all = all >> 30;
	return (int)all;
}
int cly_setting::update_speed(int share_speediKB,int share_speedKB,int down_speedKB,bool bnetset)
{
	//如果本地配置生效，则不再使用网络配置
	if(bnetset && m_pc->dconf.use_local_limit)
		return 0;
	if (share_speediKB >= 0) m_pc->dconf.limit_share_speediKB = share_speediKB;
	if (share_speedKB >= 0) m_pc->dconf.limit_share_speedKB = share_speedKB;
	if (down_speedKB >= 0) m_pc->dconf.limit_down_speedKB = down_speedKB;
	//网络配置不保存
	if(!bnetset)
	{
		m_pc->dconf.use_local_limit = 1;
		const string path = m_pc->rootdir + "cly_dconf.ini";
		cl_inifile ini;
		if(0==ini.open(path.c_str()))
		{
			ini.write_int("limit","use_local_limit",m_pc->dconf.use_local_limit);
			ini.write_int("limit","limit_share_speediKB",m_pc->dconf.limit_share_speediKB);
			ini.write_int("limit","limit_share_speedKB",m_pc->dconf.limit_share_speedKB);
			ini.write_int("limit","limit_down_speedKB",m_pc->dconf.limit_down_speedKB);
			ini.close();
		}
	}
	uac_config_t uac_conf;
	if(0==uac_get_conf(&uac_conf))
	{
		uac_conf.limit_sendspeed_i = cly_pc->dconf.limit_share_speediKB<<10;
		uac_conf.limit_sendspeed = cly_pc->dconf.limit_share_speedKB<<10;
		uac_conf.limit_recvspeed = cly_pc->dconf.limit_down_speedKB<<10;
		uac_set_conf(&uac_conf);
	}
	return 0;
}

////*****************************************************************
//bool cly_get_hash_from_shafile(const string& path,string& hash,string& subhash)
//{
//	char buf[2048]={0,};
//	string str;
//	FILE *fp = fopen(path.c_str(),"rb");
//	if(fp)
//	{
//		str = fgets(buf,2048,fp);
//		fclose(fp);
//	}
//	if(!str.empty())
//	{
//		hash = cl_util::get_string_index(str,0,"|");
//		subhash = cl_util::get_string_index(str,1,"|");
//		unsigned int ncrc = atoi(cl_util::get_string_index(str,2,"|").c_str());
//		unsigned int nhash = CL_CRC32_FIRST;
//		nhash = cl_crc32_write(nhash,(unsigned char*)hash.c_str(),hash.length());
//		nhash = cl_crc32_write(nhash,(unsigned char*)subhash.c_str(),subhash.length());
//		nhash &= 0x7fffffff;
//		if(!hash.empty() && !subhash.empty() && nhash==ncrc)
//			return true;
//
//	}
//	return false;
//}
//bool cly_put_hash_to_shafile(const string& path,const string& hash,const string& subhash)
//{
//	char buf[2048];
//	unsigned int nhash = CL_CRC32_FIRST;
//	nhash = cl_crc32_write(nhash,(unsigned char*)hash.c_str(),hash.length());
//	nhash = cl_crc32_write(nhash,(unsigned char*)subhash.c_str(),subhash.length());
//	nhash &= 0x7fffffff;
//	sprintf(buf,"%s|%s|%d",hash.c_str(),subhash.c_str(),nhash);
//	FILE *fp = fopen(path.c_str(),"wb+");
//	if(fp)
//	{
//		fputs(buf,fp);
//		fclose(fp);
//		return true;
//	}
//	return false;
//}

