#include "cly_localCmd.h"
#include "cl_util.h"
#include "cl_fhash.h"
#include "cly_d.h"
#include "cly_fileinfo.h"
#include "cl_net.h"
#include "cl_crc32.h"
#include "cl_md5_2.h"
#include "cly_cTracker.h"
#include "cl_httpc.h"
#include "cly_config.h"
#include "cl_sha1.h"
#include "cl_urlcode.h"



cly_localCmd::cly_localCmd(void)
	:m_brun(false)
	,m_scan_tick(0)
{
}


cly_localCmd::~cly_localCmd(void)
{
}

int cly_localCmd::run()
{
	if(m_brun || 0==cly_pc->main_process)
		return 0;
	m_brun = true;
	this->activate(1);
	return 0;
}

void cly_localCmd::end()
{
	if(!m_brun)
		return;
	m_brun = false;
	wait();
}

int cly_localCmd::work(int e)
{
	Sleep(3000);
	if(0==e)
	{
		string path;
		int n1=0;
		path = CLY_DIR_SHARE + "sharefile.txt";

		down_test();
		
		scan_share();
		while(m_brun)
		{
			Sleep(1000);
			if(++n1>10)
			{
				n1 = 0;
			
				share_byfile(path+".tmp");
				cl_util::file_rename(path,path+".tmp");
				share_byfile(path+".tmp");
			}
			if(++m_scan_tick>3600)
			{
				m_scan_tick = 0;
				scan_share();
			}
		}
	}
	else if(1==e)
	{
	}
	return 0;
}
void cly_localCmd::down_test()
{
	////test:
	//clyd::instance()->create_download("ee32d54c73596f3c6803da04a9f8ea71",FTYPE_DOWNLOAD,"天地.mp4");

	list<string> ls;
	string name,hash,delfile;
	cl_util::get_stringlist_from_file(cl_util::get_module_dir()+"download_test.txt",ls);
	for(list<string>::iterator it=ls.begin();it!=ls.end();++it)
	{
		string& str = *it;
		hash = cl_util::get_string_index(str,0,"|");
		name = cl_util::get_string_index(str,1,"|");
		delfile = cl_util::get_string_index(str,2,"|");
		if(hash.empty()) continue;
		if(delfile=="1")
			clyd::instance()->delete_file(hash,true);
		clyd::instance()->create_download(hash,FTYPE_DOWNLOAD,name);
	}
}
int cly_localCmd::share_list(list<string>& ls)
{
	int n = 0;
	string path,name;
	string outHash;
	for(list<string>::iterator it=ls.begin();m_brun && it!=ls.end();++it,++n)
	{
		path = cl_util::get_string_index(*it,0,"|");
		name = cl_util::get_string_index(*it,1,"|");
		share_file(path,name,false,outHash,false);
	}
	return n;
}
void cly_localCmd::share_byfile(const string& path)
{
	list<string> ls;
	string bakpath = CLY_DIR_SHARE + "sharefile.txt.bak";

	cl_util::get_stringlist_from_file(path,ls);
	if((int)ls.size()==share_list(ls))
	{
		cl_util::file_delete(bakpath);
		cl_util::file_rename(path,bakpath);
	}
}
void cly_localCmd::scan_share()
{
	//DEBUGMSG("# cly_localCmd::scan_share() !\n");
	string path = CLY_DIR_SHARE + "sharefile.txt.tmp";
	share_byfile(path);
	string str;
	string& suffix = cly_pc->share_suffix;

	list<string> ls_path;
	list<int> ls_ino;
	list<string>& ls = CLYSET->m_share_path_list;
	for(list<string>::iterator it=ls.begin();it!=ls.end();++it)
		cl_util::get_folder_files(*it,ls_path,ls_ino,suffix,1); //不同硬盘之间是有可能两个独立文件inode相同而不是软连接

	if(!ls_path.empty())
	{
		cl_util::put_stringlist_to_file(path,ls_path);
		share_byfile(path);
	}
}

int cly_localCmd::share_file(const string& path,const string& name,bool try_update_name,string& outHash,bool up_task)
{
	int ret=-1;
	char buf[1024];
	string hash,subhash;
	cly_readyInfo_t ri;
	outHash = "";
	if(0!=clyd::instance()->get_readyinfo_by_path(path,ri))
	{
		if(!cl_get_fhash_from_hashfile(path+".sha3",hash,subhash))
		{	
#ifdef ANDROID
			return -1; //android 不运算HASH
#endif
			cl_util::write_tlog((CLY_DIR_SHARE + "hash.log").c_str(), 1024, "+++ : %s", path.c_str());
			if(0==cl_fhash_file_sbhash(path.c_str(),CLY_FBLOCK_SIZE,buf))
			{
				subhash = buf;
				hash = cl_fhash_sbhash_to_mainhash(subhash);
				cl_put_fhash_to_hashfile(path+".sha3",hash,subhash);
				cl_util::write_tlog((CLY_DIR_SHARE + "hash.log").c_str(), 2048, "--- : %s | %s", hash.c_str(),buf);
			}
			
		}
		if(!hash.empty() && !subhash.empty())
		{
			outHash = hash;
			//先尝试改名
			if(try_update_name && !name.empty())
				cly_cTrackerSngl::instance()->_update_filename(hash,name);
			if(-1!=(ret=clyd::instance()->share_file(hash,subhash,path,name,true)))
			{
				ret = 0;
				cl_util::write_tlog((CLY_DIR_SHARE+"share.log").c_str(),2048,"%s|%s|%s",hash.c_str(),path.c_str(),subhash.c_str());
				oldhash_relation(path, hash);
			}
		}
	}
	else
	{
		//已存在
		ret = 1;
		outHash = ri.hash;
		if(try_update_name && !name.empty() && name!=ri.name)
		{
			cly_cTrackerSngl::instance()->_update_filename(ri.hash,name);
			clyd::instance()->share_file(ri.hash,ri.subhash,path,name,true);//共享的改名
		}
		//DEBUGMSG("#cly_localCmd::share =>(%s) is already shared! \n",path.c_str());
	}

	//检查upload_task 任务
	if (-1 != ret && !outHash.empty() && up_task && !cly_pc->upload_hids.empty())
	{
		//此时未一定共享完成，即tracker未一定有HASH信息
		int n = 0;
		bool bfind = false;
		clyt_fileinfo_t fi;
		while (++n < 5)
		{
			Sleep(800);
			if (0 == cly_cTrackerSngl::instance()->_get_fileinfo(outHash, fi))
			{
				bfind = true;
				break;
			}
		}

		if (bfind)
			ret = cly_cTrackerSngl::instance()->_upload_task(cly_pc->upload_hids, outHash);
		else
			ret = CLY_TERR_NO_SOURCE;
	}
	return ret;
}

string get_oldhash_from_shafile(const string& path)
{
	list<string> ls;
	cl_util::get_stringlist_from_file(path, ls);
	if (!ls.empty())
	{
		string& s = ls.front();
		if (s.length() == 40)
			return s;
	}
	return "";
}
/*
先尝试读旧HASH，如果没有旧HASH，则运算一次，运算完再缓存
*/
void cly_localCmd::oldhash_relation(const string& srcpath, const string& newhash)
{
	string oldpath, newpath;
	string sha3buf;
	string oldhash;
	char shash[41];
	memset(shash, 0, 41);
	if (cly_pc->url_oldhash_relation.empty())
		return;
	oldpath = srcpath + ".sha";
	newpath = srcpath + ".sha3";

	cl_get_buffer_from_hashfile(newpath, sha3buf);
	if (sha3buf.empty())
		return;
	sha3buf = cl_urlencode(sha3buf);
	oldhash = get_oldhash_from_shafile(oldpath);
	if (oldhash.empty())
	{
		//运算旧hash
		cl_util::write_tlog((CLY_DIR_SHARE + "hash.log").c_str(), 1024, "+++sha1 : %s", srcpath.c_str());
		Sha1_BuildFile(srcpath.c_str(), shash, NULL);
		cl_util::write_tlog((CLY_DIR_SHARE + "hash.log").c_str(), 2048, "---sha1 : %s", shash);
		oldhash = shash;
		if (oldhash.empty())
			return;

		list<string> ls;
		char buf[128];
		sprintf(buf, "%lld", cl_ERDBFile64::get_filesize(srcpath.c_str()));
		ls.push_back(shash);
		ls.push_back(buf);
		cl_util::put_stringlist_to_file(oldpath, ls);
	}
	//
	char body[2048];
	string head;
	string rspbody;
	sprintf(body, "hash=%s&new_hash=%s&hash_file_text=%s", oldhash.c_str(),
		newhash.c_str(), sha3buf.c_str());
	cl_httpc::request(cly_pc->url_oldhash_relation.c_str(), body, strlen(body), head, rspbody);
}



