#include "cly_usermgr.h"
#include "cly_db.h"
#include "cl_util.h"
#include "cly_setting.h"
#include "cly_handleApi.h"
#include "cl_crc32.h"

cly_usermgr::cly_usermgr(void)
{
}


cly_usermgr::~cly_usermgr(void)
{
}

int cly_usermgr::init()
{
	if (0 != cly_dbmgrSngl::instance()->init())
		return -1;
	return 0;
}
void cly_usermgr::fini()
{
	cly_dbmgrSngl::instance()->fini();
	cly_dbmgrSngl::destroy();
	cly_sourceSngl::destroy();
}
void cly_usermgr::on_timer(int e)
{
}
int cly_usermgr::login(const cly_login_info_t& inf,cly_seg_config_t& seg_conf)
{
	int err=0;
	{
		Lock l(m_mt);
		memset((void*)&seg_conf, 0, sizeof(seg_conf));
		seg_conf = cly_settingSngl::instance()->m_userconf;
		err = cly_dbmgrSngl::instance()->db_login(inf, seg_conf);

		//更新addr
		if (0 == err && !inf.addr.empty())
			cly_dbmgrSngl::instance()->db_update_nat(seg_conf.peer_id, inf.addr);
	}
	
	if(0==seg_conf.group_id && 0==err)
	{
		//实时更新uid可能引起阻塞：TODO
		seg_conf.group_id = cly_handleApiSngl::instance()->get_group_id(inf.peer_name);
		if(0!=seg_conf.group_id)
		{
			Lock l(m_mt);
			cly_dbmgrSngl::instance()->db_update_group_id(seg_conf.peer_id,seg_conf.group_id);
		}
	}
	return err;
}
int cly_usermgr::keepalive(cly_seg_keepalive_t& sk, cly_seg_keepalive_ack_t& seg)
{
	Lock l(m_mt);
	int err=0;
	err = cly_dbmgrSngl::instance()->db_keepalive(sk,seg);
	return err;
}
int cly_usermgr::update_nat(int peer_id,const string& addr)
{
	Lock l(m_mt);
	int err=0;
	err = cly_dbmgrSngl::instance()->db_update_nat(peer_id,addr);
	return err;
}
int cly_usermgr::report_fdfile(int peer_id,const string& fields,const string& vals)
{
	Lock l(m_mt);
	int err=0;
	cly_seg_fdfile_t seg;
	clyt_peer_t* peer = cly_sourceSngl::instance()->get_peer(peer_id);
	if(!peer)
		return CLY_TERR_NO_PEER;
	seg.fdtype = cl_util::atoi(cl_util::get_string_index(vals,0,"|").c_str());
	seg.hash = cl_util::get_string_index(vals,1,"|");
	seg.ftype = cl_util::atoi(cl_util::get_string_index(vals,2,"|").c_str());
	seg.name = cl_util::get_string_index(vals,3,"|");
	seg.subhash = cl_util::get_string_index(vals,4,"|");
	seg.rcvB = cl_util::get_string_index(vals,5,"|");
	cl_util::string_trim(seg.hash);
	if(seg.hash.empty())
	{
		DEBUGMSG("#*** report_fdfile() hash empty() !\n");
		return CLY_TERR_WRONG_PARAM;
	}
	if(CLY_FDTYPE_DELETE!=seg.fdtype)
	{
		err = cly_dbmgrSngl::instance()->db_report_finifile(peer,seg,true);
		cly_handleApiSngl::instance()->report_finifile(peer->peer_name,seg.hash,seg.name);
	}
	else
		err = cly_dbmgrSngl::instance()->db_report_delfile(peer_id,seg);
	return err;
}
int cly_usermgr::report_progress(int peer_id,list<string>& ls)
{
	Lock l(m_mt);
	return cly_dbmgrSngl::instance()->db_report_progress(peer_id,ls);
}
int cly_usermgr::search_source(int peer_id,const string& hash,list<string>& ls)
{
	Lock l(m_mt);
	return cly_sourceSngl::instance()->src_search_source(peer_id,hash,ls);
}
int cly_usermgr::update_filename(const string& hash,const string& name)
{
	Lock l(m_mt);
	return cly_dbmgrSngl::instance()->db_update_filename(hash,name);
}
int cly_usermgr::get_fileinfo(const string& hash,clyt_fileinfo_t& fi)
{
	Lock l(m_mt);
	return cly_sourceSngl::instance()->src_get_fileinfo(hash,fi);
}
int cly_usermgr::report_error(const cly_report_error_t& inf)
{
	Lock l(m_mt);
	return cly_dbmgrSngl::instance()->db_report_error(inf);
}
int cly_usermgr::new_task(const string& task_name,list<string>& ls_hids,list<string>& ls_files,int& task_id)
{
	Lock l(m_mt);
	list<int> ls_peerid;
	list<int> ls_fileid;

	task_id = 0;
	cly_sourceSngl::instance()->get_peerids_by_hids(ls_hids,ls_peerid);
	cly_sourceSngl::instance()->get_fileids_by_hashs(ls_files,ls_fileid);
	if(task_name.empty() || ls_peerid.empty() || ls_fileid.empty())
		return CLY_TERR_TASK_NAME_LIST_EMPTY;
	return cly_dbmgrSngl::instance()->db_new_task(task_name,ls_peerid,ls_fileid,task_id);
}
int cly_usermgr::set_task_state(const string& task_name,int state)
{
	Lock l(m_mt);
	return cly_dbmgrSngl::instance()->db_set_task_state(task_name,state);
}
int cly_usermgr::get_task_info(const string& task_name,string& path)
{
	Lock l(m_mt);
	char buf[1024];
	int retcode;
	list<cly_task_node_t> ls;
	list<string> ls_str;
	clyt_file_t* pfile;
	clyt_peer_t* ppeer;

	sprintf(buf,"%staskinfo/taski_%s.txt",cly_settingSngl::instance()->get_datadir().c_str(),task_name.c_str());
	path = buf;
	retcode = cly_dbmgrSngl::instance()->db_get_task_info(task_name,ls);
	if(0!=retcode)
		return retcode;

	ls_str.push_back("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	ls_str.push_back("<root>");
	ls_str.push_back("  <seg id=\"error\">");
	ls_str.push_back("    <p n=\"code\">0</p>");
	ls_str.push_back("    <p n=\"message\">succeed</p>");
	ls_str.push_back("  </seg>");
	sprintf(buf,"  <list id=\"info\" task_name=\"%s\" fields=\"hash|hid|progress\">",task_name.c_str());
	ls_str.push_back(buf);

	list<cly_task_node_t>::iterator it;
	for(it=ls.begin();it!=ls.end();++it)
	{
		cly_task_node_t& tn = *it;
		pfile = cly_sourceSngl::instance()->get_file(tn.file_id);
		ppeer = cly_sourceSngl::instance()->get_peer(tn.peer_id);
		if(!pfile || !ppeer)
			continue;
		sprintf(buf,"    <p>%s|%s|%d</p>",pfile->hash.c_str(),ppeer->peer_name.c_str(),tn.progress);
		ls_str.push_back(buf);
	}
	ls_str.push_back("  </list>");
	ls_str.push_back("</root>");

	if(cl_util::put_stringlist_to_file(path,ls_str))
		return 0;
	else
		return CLY_TERR_WRITE_FILE_FAILED;
}
int cly_usermgr::get_ddlist(int peer_id,int update_id,string& path)
{
	Lock l(m_mt);
	int ret;
	list<cly_ddlist_node_t> ls;
	list<string> ls_str;
	clyt_file_t* pfile;
	char buf[512];

	sprintf(buf,"%sddlist/ddlist_%d.txt",cly_settingSngl::instance()->get_datadir().c_str(),peer_id);
	path = buf;
	
	ret = cly_dbmgrSngl::instance()->db_get_ddlist(peer_id,update_id,ls);
	if(0!=ret)
		return ret;

	ls_str.push_back("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	ls_str.push_back("<root>");
	ls_str.push_back("  <seg id=\"error\">");
	ls_str.push_back("    <p n=\"code\">0</p>");
	ls_str.push_back("    <p n=\"message\">succeed</p>");
	ls_str.push_back("  </seg>");
	sprintf(buf,"  <list id=\"ddlist\" taskid=\"%d\" fields=\"state|hash|priority|name\">",update_id);
	ls_str.push_back(buf);

	list<cly_ddlist_node_t>::iterator it;
	for(it=ls.begin();it!=ls.end();++it)
	{
		cly_ddlist_node_t& tn = *it;
		pfile = cly_sourceSngl::instance()->get_file(tn.file_id);
		if(!pfile)
			continue;
		sprintf(buf,"    <p>%d|%s|%d|%s</p>",tn.state,pfile->hash.c_str(),tn.priority,pfile->name.c_str());
		ls_str.push_back(buf);
	}
	ls_str.push_back("  </list>");
	ls_str.push_back("</root>");

	if(cl_util::put_stringlist_to_file(path,ls_str))
		return 0;
	else
		return CLY_TERR_WRITE_FILE_FAILED;
}
int cly_usermgr::check_fini_chsum(int peer_id,const cly_seg_fchsum_t& seg)
{
	Lock l(m_mt);
	return cly_dbmgrSngl::instance()->db_check_fini_chsum(peer_id,seg);
}
int cly_usermgr::get_finifiles(int peer_id,string& path)
{
	Lock l(m_mt);
	//从source中获取
	char buf[512];
	sprintf(buf,"%sfflist/get_finifiles_%d.txt",cly_settingSngl::instance()->get_datadir().c_str(),peer_id);
	path = buf;
	return cly_sourceSngl::instance()->src_save_peer_finifiles(peer_id,path);
}
int cly_usermgr::put_finifiles(int peer_id,const char* path)
{
	/*
	说明：
	1. 两列表匹配去掉重叠
	2。删除DB已有但客户端没有的
	3。添加DB没有但客户端有的
	4。重置checksum 值。
	*/
	Lock l(m_mt);
	clyt_peer_t *peer;
	int file_id;
	list<string> ls_hash;
	list<string> ls_del;
	list<string> ls_all;
	string hash;
	int ret;
	if(!cl_util::get_stringlist_from_file(path,ls_all))
		return CLY_TERR_READ_FILE_FAILED;

	{
		peer = cly_sourceSngl::instance()->get_peer(peer_id);
		if(peer)
		{
			for(map<int,clyt_file_t*>::iterator it=peer->fini_files.begin();it!=peer->fini_files.end();++it)
			{
				ls_hash.push_back(it->second->hash);
			}
		}
	}
	if(NULL==peer)
		return CLY_TERR_NO_PEER;

	ls_all.pop_front(); //去掉checksum
	ls_all.pop_front(); //去掉 id|hash|ftype|mtime|ctime|path|fullpath|subhash
	//1。去掉重叠
	bool bfind;
	if(!ls_all.empty())
	{
		for(list<string>::iterator it1=ls_hash.begin();it1!=ls_hash.end();++it1)
		{
			bfind = false;
			string& h1 = *it1;
			for(list<string>::iterator it2=ls_all.begin();it2!=ls_all.end();++it2)
			{
				hash = cl_util::get_string_index(*it2,1,"|");
				if(h1==hash)
				{
					bfind = true;
					ls_all.erase(it2);
					break;
				}
			}
			if(!bfind)
				ls_del.push_back(h1);
		}
	}
	else
	{
		//全删除
		ls_del.swap(ls_hash);
	}
	DEBUGMSG("# put_finifiles(PID=%d, DB_NEED_DEL=%d, DB_NEED_ADD=%d) \n",peer_id,ls_del.size(),ls_all.size());

	//2。删除服务器数据
	for(list<string>::iterator it=ls_del.begin();it!=ls_del.end();++it)
	{
		if(0==cly_sourceSngl::instance()->src_delfile(peer,*it,file_id))
			cly_dbmgrSngl::instance()->db_delsource(peer_id,file_id);
	}
	//3。添加客户端数据
	cly_seg_fdfile_t seg;
	for(list<string>::iterator it=ls_all.begin();it!=ls_all.end();++it)
	{
		string& str = *it;
		seg.fdtype = 1;
		seg.hash = cl_util::get_string_index(str,1,"|");
		seg.ftype = cl_util::atoi(cl_util::get_string_index(str,2,"|").c_str());
		seg.name = cl_util::get_string_index(str,5,"|");
		seg.subhash = cl_util::get_string_index(str,7,"|");
		cl_util::string_trim(seg.hash);
		cly_dbmgrSngl::instance()->db_report_finifile(peer,seg,false);
	}

	//4。重置checksum
	//使用内存数据
	int ff_num = 0;
	unsigned long long ff_checksum = 0;
	clyt_file_t *file;
	ff_num = peer->fini_files.size();
	{
		for(map<int,clyt_file_t*>::iterator it=peer->fini_files.begin();it!=peer->fini_files.end();++it)
		{
			file = it->second;
			ff_checksum += cl_crc32_write(CL_CRC32_FIRST,(const unsigned char*)file->hash.c_str(),file->hash.length());
		}
	}
	ret = cly_dbmgrSngl::instance()->db_reset_chsum(peer_id, ff_num, ff_checksum);
	return ret;
}

int cly_usermgr::get_state(clyt_state_t& s)
{
	Lock l(m_mt);
	return cly_sourceSngl::instance()->src_get_state(s);
}
int cly_usermgr::get_peer_state(const string& peer_name,clyt_peer_t& s)
{
	Lock l(m_mt);
	return cly_sourceSngl::instance()->src_get_peer_state(peer_name,s);
}

int cly_usermgr::get_server(list<string>& ls)
{
	Lock l(m_mt);
	return cly_sourceSngl::instance()->src_get_server(ls);
}
