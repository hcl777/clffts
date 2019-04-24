#include "cly_filemgr.h"
#include "cly_config.h"
#include "cl_util.h"
#include "cl_file32.h"
#include "cl_bstream.h"
#include "cl_crc32.h"
#include "cl_unicode.h"
#include "cly_jni.h"
#include "cly_server.h"
#include "cl_fhash.h"

cly_filemgr::cly_filemgr(void)
	:m_binit(false)
	,m_max_no(0)
{
	m_seg_fchsum.ff_num = 0;
	m_seg_fchsum.ff_checksum = 0;
}


cly_filemgr::~cly_filemgr(void)
{
}

int cly_filemgr::init()
{
	if(m_binit)
		return 1;
	m_binit = true;
	//信息保存目录
	m_downinfo_path = CLY_DIR_INFO + "downinfo.dat";
	m_readyinfo_path = CLY_DIR_INFO + "readyinfo.dat";

	load_downinfo();
	load_readyinfo();

	m_readyinfo_mtime = cl_util::get_file_modifytime(m_readyinfo_path.c_str());
	
	return 0;
}
void cly_filemgr::fini()
{
	if(!m_binit)
		return;
	m_binit = false;

	fileiter it;
	for(it=m_downInfo.begin();it!=m_downInfo.end();++it)
		delete it->second;
	m_downInfo.clear();
	for(it=m_readyInfo.begin();it!=m_readyInfo.end();++it)
		delete it->second;
	m_readyInfo.clear();
}

void cly_filemgr::load_downinfo()
{
	if(!cly_pc->main_process)
		return;
	cly_fileinfo* fi;
	list<string> ls;
	string str,name,fullpath;
	int wrong_di_num=0;
	cl_util::get_stringlist_from_file(m_downinfo_path,ls);
	for(list<string>::iterator it=ls.begin();it!=ls.end(); ++it)
	{
		str = *it;
		name = cl_util::get_string_index(str,0,"|");
		fullpath = cl_util::get_string_index(str,1,"|");
		fi = cly_fileinfo::load_downinfo_i(name,fullpath);
		if(fi)
		{
			fi->http_url = cl_util::get_string_index(str, 2, "|");
			m_downInfo[fi->fhash] = fi;
		}
		else
		{
			//尝试将临时文件也一齐删除
			cl_file64::remove_file(fullpath.c_str());
			wrong_di_num++;
		}
	}
	if(wrong_di_num>0)
	{
		save_downinfo_main();
	}
	DEBUGMSG("$load_downinfo = %d / %d \n",m_downInfo.size(),ls.size());
}
void cly_filemgr::save_downinfo_main()
{
	assert(cly_pc->main_process);
	list<string> ls;
	char buf[1024];
	for(fileiter it=m_downInfo.begin();it!=m_downInfo.end();++it)
	{
		sprintf(buf,"%s|%s|%s",it->second->name.c_str(),it->second->fullpath.c_str(),it->second->http_url.c_str());
		ls.push_back(buf);
	}
	cl_util::put_stringlist_to_file(m_downinfo_path,ls);
}

void cly_filemgr::load_readyinfo()
{
	cl_TLock<Mutex> l(m_mt); //second_peer版会多线程运行
	list<string> ls;
	int wrong_num=0;
	string logpath = CLY_DIR_LOG+"readyloss.log";
	if(!cl_util::get_stringlist_from_file(m_readyinfo_path,ls))
	{
		//cl_util::write_tlog(logpath.c_str(),1024,"load_readyinfo();open(%s) fail",m_readyinfo_path.c_str());
		return;
	}
	//清理源列表,注意，引用会被重置
	if(!m_readyInfo.empty())
	{
		for(fileiter it=m_readyInfo.begin();it!=m_readyInfo.end();++it)
			delete it->second;
		m_readyInfo.clear();
	}
	m_seg_fchsum.ff_checksum = 0;
	if(ls.size()>1)
	{

		cly_fileinfo *fi = NULL;
		string str;

		ls.pop_front();
		ls.pop_front();

		for(list<string>::iterator it=ls.begin();it!=ls.end(); ++it)
		{
			str = *it;
			cl_util::string_trim(str);
			if(str.empty())
				continue;
			fi = new cly_fileinfo();
			fi->bready = true;
			//0为序号
			fi->fhash = cl_util::get_string_index(str,1,"|");
			fi->ftype = atoi(cl_util::get_string_index(str,2,"|").c_str());
			fi->mtime = cl_util::get_string_index(str,3,"|");
			fi->ctime = cl_util::get_string_index(str,4,"|");
			fi->name = cl_util::get_string_index(str,5,"|");
			fi->fullpath = cl_util::get_string_index(str,6,"|");
			fi->sh.load_strhash(cl_util::get_string_index(str,7,"|"));
			if(0==cl_util::file_state(fi->fullpath.c_str()))
				fi->fullpath = CLYSET->find_exist_fullpath(fi->name);
			if(fi->fullpath.empty() || fi->sh.size==0)
			{
				report_delefile(fi);
				cl_util::write_tlog(logpath.c_str(),1024,"load_readyinfo();file loss: %s - %s ",fi->fhash.c_str(),fi->name.c_str());
				delete fi;
				wrong_num++;
				continue;
			}

			fi->no = m_max_no++;
			m_readyInfo[fi->fhash] = fi;
			m_seg_fchsum.ff_checksum += cl_crc32_write(CL_CRC32_FIRST,(const unsigned char*)fi->fhash.c_str(),fi->fhash.length());
		}
	}
	m_seg_fchsum.ff_num = m_readyInfo.size();
	cly_cTrackerSngl::instance()->set_checksum(&m_seg_fchsum);

	cl_util::write_tlog(logpath.c_str(),1024,"load_readyinfo[%s]::file num/all num=%d / %d ;wrong=%d "
		,cly_pc->main_process?"main":"",m_readyInfo.size(),ls.size(),wrong_num);
	if(wrong_num>0)
		save_readyinfo();
	
	DEBUGMSG("$load_readyinfo = %d / %d \n",m_readyInfo.size(),ls.size());
}
void cly_filemgr::save_readyinfo()
{
	if(!cly_pc->main_process)
		return;
	list<string> ls;
	cly_fileinfo *fi = NULL;
	char buf[2048];
	string logpath = CLY_DIR_LOG+"save_readyinfo.log";

	//保存考虑以mtime时间排序
	void** pfi = new void*[m_max_no+1];
	memset(pfi,0,(m_max_no+1)*sizeof(void*));
	
	//按序号顺序保存
	for(fileiter it=m_readyInfo.begin();it!=m_readyInfo.end();++it)
		pfi[it->second->no] = it->second;
	string tmppath = m_readyinfo_path + ".tmp";
	cl_util::file_delete(tmppath);
	FILE *fp = fopen(tmppath.c_str(),"wb+");
	if(fp)
	{
		//先保存chsum
		sprintf(buf,"%lld\r\n"
			"id|hash|ftype|mtime|ctime|path|fullpath|subhash\r\n",m_seg_fchsum.ff_checksum);
		fwrite(buf,strlen(buf),1,fp);

		for(int i=0;i<m_max_no;++i)
		{
			fi = (cly_fileinfo*)pfi[i];
			//id|hash|ftype|mtime|ctime|path|fullpath|subhash
			sprintf(buf,"%5d|%s|%d|%s|%s|%s|%s|%s\r\n",fi->no,fi->fhash.c_str(),fi->ftype
				,fi->mtime.c_str(),fi->ctime.c_str(),fi->name.c_str(),fi->fullpath.c_str(),fi->sh.subhash.c_str());
			fwrite(buf,strlen(buf),1,fp);
		}
		fclose(fp);
	}
	delete[] pfi;
	
	//备份
	string bakpath = CLY_DIR_INFO + "readybak/readyinfo_"+ cl_util::time_to_datetime_string2(time(NULL)) + ".dat";
	cl_util::create_directory_by_filepath(bakpath);
	cl_util::file_delete(bakpath);
	cl_util::file_rename(m_readyinfo_path,bakpath);
	
	//更新
	cl_util::file_delete(m_readyinfo_path);
	cl_util::file_rename(tmppath,m_readyinfo_path);

	//清理旧备份
	{
		string bakdir = CLY_DIR_INFO + "readybak";
		list<string> ls_path;
		list<int> ls_ino;
		cl_util::get_folder_files(bakdir,ls_path,ls_ino,"dat",1,false);
		if(ls_path.size()>110)
		{
			//删除前10个
			int i=0;
			for(list<string>::iterator it=ls_path.begin();it!=ls_path.end()&&i<10; ++it,i++)
			{
				cl_util::file_delete(*it);
			}
		}
	}
}
int cly_filemgr::save_readyfile_utf8(const string& path)
{
	list<string> ls,ls2;
	string str;
	if(!cl_util::get_stringlist_from_file(m_readyinfo_path,ls))
	{
		//文件不存在，此为也上报空记录
		ls.push_back("0");
	}
	for(list<string>::iterator it=ls.begin();it!=ls.end();++it)
	{
		if(cly_pc->lang_utf8)
			ls2.push_back(*it);
		else
		{
			cl_unicode::GB2312ToUTF_8(str,(*it).c_str(),(*it).length());
			ls2.push_back(str);
		}
	}
	if(cl_util::put_stringlist_to_file(path,ls2))
		return 0;
	return -1;
}
cly_fileinfo* cly_filemgr::create_downinfo(const string& hash,int ftype,const string& path, bool bsave_original)
{
	cl_TLock<Mutex> l(m_mt);
	if(hash.empty())
		return NULL;
	fileiter it=m_downInfo.find(hash);
	if(it!=m_downInfo.end())
	{
		if(it->second->ftype==FTYPE_VOD)
			it->second->ftype = ftype;
		return it->second;
	}
	
	cly_fileinfo *fi = new cly_fileinfo();
	fi->fhash = hash;
	fi->ftype = ftype;
	fi->name = path;
	fi->bsave_original = bsave_original;
	fi->name += ".d!";
	fi->fullpath = CLYSET->find_new_fullpath(fi->name);
	fi->name = CLYSET->find_path_name(fi->name);
	cl_util::create_directory_by_filepath(fi->fullpath);
	fi->info_path = fi->fullpath + ".cli";
	//尝试创建文件，但删除，因为未保存下载信息
	if(!fi->open_file(F64_RDWR|F64_TRUN,RDBF_BASE))
	{
		if(FTYPE_VOD!=fi->ftype)
		{
			string logpath = CLYSET->get_log_path() + "create_downinfo.log";
			cl_util::write_tlog(logpath.c_str(),1024,"create_downinfo(%s,%d,%s) open file fail",hash.c_str(),ftype,path.c_str());
			delete fi;
			return NULL;
		}
	}
	fi->close_file();
	cl_util::file_delete(fi->fullpath);
	fi->ctime = cl_util::time_to_datetime_string(time(NULL));
	m_downInfo[fi->fhash] = fi;
	//未获得大小不保存信息
	save_downinfo_main();
	return fi;
}
int cly_filemgr::create_readyinfo(const string& hash,const string& subhash,const string& path,const string& name,bool report)
{
	cl_TLock<Mutex> l(m_mt);
	fileiter it;
	cly_fileinfo *fi = NULL;
	string logpath = CLY_DIR_LOG+"localshare.log";
	////检查删除下载列表中的节目,可能任务正在下载中
	//it1 = m_downInfo.find(hash);
	//if(it1!=m_downInfo.end())
	//{
	//	cl_util::write_tlog(logpath.c_str(),1024,"share tth=%s(%s),delete download file info(%s)",hash.c_str(),path.c_str(),it1->second->path.c_str());
	//	delete_downinfo(hash);
	//}
	it = m_readyInfo.find(hash);
	if(it!=m_readyInfo.end())
	{
		//更新内容
		fi = it->second;
		if(!name.empty() && name!=fi->name)
		{
			fi->name = name; //改名
			save_readyinfo();
		}
		if(CLYSET->find_exist_fullpath(path) == fi->fullpath)
		{
			//fi->ftype = FTYPE_SHARE;
			cl_util::write_tlog(logpath.c_str(),1024,"share the same path file(%s - %s)",hash.c_str(),path.c_str());
			return 0;
		}

		if (/*fi->ftype != FTYPE_SHARE ||*/ !cl_util::file_exist(fi->fullpath.c_str()))
		{
			cl_util::write_tlog(logpath.c_str(),1024,"share tth=%s(%s),delete ready fileinfo(%s)",hash.c_str(),path.c_str(),fi->name.c_str());
			delete_readyinfo(hash,true);
		}
		else
		{
			cl_util::write_tlog(logpath.c_str(),1024,"share (%s-%s) fail, another file(%s) is shared!",hash.c_str(),path.c_str(),fi->name.c_str());
			return 1;
		}
	}
	//
	fi = new cly_fileinfo();
	fi->fhash = hash;
	fi->sh.load_strhash(subhash);
	fi->ftype = FTYPE_SHARE;
	fi->bready = true;
	fi->fullpath = CLYSET->find_exist_fullpath(path);
	fi->name = name;
	if(fi->name.empty())fi->name = CLYSET->find_path_name(path);
	if(fi->fullpath.empty() || fi->sh.size<=0)
	{
		delete fi;
		return -1;
	}
	fi->mtime = cl_util::time_to_datetime_string(time(NULL));
	fi->ctime = "----:--:-- --:--:--";
	fi->no = m_max_no++;
	m_readyInfo[fi->fhash] = fi;
	m_seg_fchsum.ff_checksum += cl_crc32_write(CL_CRC32_FIRST,(const unsigned char*)fi->fhash.c_str(),fi->fhash.length());
	m_seg_fchsum.ff_num = m_readyInfo.size();
	save_readyinfo();

	//如果从数据库同步过来，则不需要上报
	if(report)
		report_finifile(fi);
	return 0;
}
int cly_filemgr::delete_downinfo(const string& hash)
{
	cl_TLock<Mutex> l(m_mt);
	cly_serverSngl::instance()->on_delete_file(hash);
	fileiter it=m_downInfo.find(hash);
	if(it==m_downInfo.end())
		return -1;
	cly_fileinfo *fi = it->second;
	assert(0==fi->ref);
	fi->close_file();
	fi->delete_infofile();
	cl_file64::remove_file(fi->fullpath.c_str());

	report_delefile(fi);
	delete fi;
	m_downInfo.erase(it);
	save_downinfo_main();
	return 0;
}
int cly_filemgr::delete_readyinfo(const string& hash,bool delPhyFile)
{
	cl_TLock<Mutex> l(m_mt);
	cly_serverSngl::instance()->on_delete_file(hash);
	fileiter it=m_readyInfo.find(hash);
	if(it==m_readyInfo.end())
		return -1;
	cly_fileinfo *fi = it->second;
	int no = fi->no;
	string logpath = CLY_DIR_LOG + "delete_readyinfo.log";
	cl_util::write_tlog(logpath.c_str(),1024,":delPhyFile=%d (%s,%s)",delPhyFile,hash.c_str(),fi->name.c_str());
	
	//assert(0==fi->ref);
	fi->close_file();
	if(delPhyFile)
	{
		cl_file64::remove_file(fi->fullpath.c_str());
	}
	m_readyInfo.erase(it);
	
	m_seg_fchsum.ff_checksum -= cl_crc32_write(CL_CRC32_FIRST,(const unsigned char*)fi->fhash.c_str(),fi->fhash.length());
	m_seg_fchsum.ff_num = m_readyInfo.size();
	report_delefile(fi);
	delete fi;

	//变号.
	for(it=m_readyInfo.begin();it!=m_readyInfo.end();++it)
	{
		if(it->second->no > no)
			it->second->no--;
	}
	m_max_no--;
	save_readyinfo();
	return 0;
}
int cly_filemgr::delete_info(const string& hash,bool delPhyFile)
{
	if(0!=delete_downinfo(hash))
		return delete_readyinfo(hash,delPhyFile);
	return -1;
}
cly_fileinfo* cly_filemgr::get_downinfo(const string& hash)
{
	fileiter it=m_downInfo.find(hash);
	if(it!=m_downInfo.end())
		return it->second;
	return NULL;
}
cly_fileinfo* cly_filemgr::get_readyinfo(const string& hash)
{
	fileiter it=m_readyInfo.find(hash);
	if(it!=m_readyInfo.end())
		return it->second;
	return NULL;
}
cly_fileinfo* cly_filemgr::get_readyinfo_by_path(const string& path)
{
	for(fileiter it=m_readyInfo.begin();it!=m_readyInfo.end();++it)
	{
		if(path==it->second->fullpath || path == it->second->name)
		{
			return it->second;
		}
	}
	return NULL;
}
cly_fileinfo* cly_filemgr::get_fileinfo(const string& hash)
{
	cly_fileinfo* inf = get_readyinfo(hash);
	if(NULL==inf)
		inf = get_downinfo(hash);
	return inf;
}
int cly_filemgr::check_exist_share_bypath(const string& path)
{
	for(fileiter it=m_readyInfo.begin();it!=m_readyInfo.end();++it)
	{
		if(path==it->second->fullpath || path == it->second->name)
			return 0;
	}
	return -1;
}
cl_memblock* cly_filemgr::read_block(const string& hash,unsigned int index)
{
	cly_fileinfo* fi;
	cl_memblock* b;
	if(NULL==(fi=get_readyinfo(hash)))
		fi = get_downinfo(hash);
	if(NULL==fi)
		return NULL;
	b = fi->read_block(index);
	if(NULL==b && fi->bready)
	{
		string logpath = CLY_DIR_LOG+"readyloss.log";
		cl_util::write_tlog(logpath.c_str(),1024,"read_block();file loss: %s - %s ",fi->fhash.c_str(),fi->name.c_str());
		delete_readyinfo(hash,false);
	}
	return b;
}

int cly_filemgr::refer_file(const string& hash)
{
	cly_fileinfo* fi;
	if(NULL==(fi=get_readyinfo(hash)))
		fi = get_downinfo(hash);
	if(fi) fi->ref++;
	return 0;
}
int cly_filemgr::release_file(const string& hash)
{
	cly_fileinfo* fi;
	if(NULL==(fi=get_readyinfo(hash)))
		fi = get_downinfo(hash);
	if(fi)
	{
		if(fi->ref > 0)
			fi->ref--;
		if(0==fi->ref && fi->bready)
			fi->close_file();
	}
	return 0;
}
int cly_filemgr::read_down_data(const string& hash,size64_t pos,char* buf,int size)
{
	cl_TLock<Mutex> l(m_mt);
	cly_fileinfo* fi;
	if(NULL!=(fi=get_downinfo(hash)))
		return fi->read_data(pos,buf,size);
	return 0;
}

int cly_filemgr::on_file_done(const string& hash)
{
	//必须校验完成后才调用
	cl_TLock<Mutex> l(m_mt);
	DEBUGMSG("# cly_filemgr::on_file_done(%s) \n",hash.c_str());
	delete_readyinfo(hash,false);
	fileiter it = m_downInfo.find(hash);
	if(it==m_downInfo.end())
		return -1;
	cly_fileinfo *fi = it->second;
	fi->close_file();

	string desfullpath;
	int pos1 = (int)fi->name.rfind('.');
	int pos2 = (int)fi->fullpath.rfind('.');
	if(pos2>0)
	{
		desfullpath = fi->fullpath.substr(0,pos2);
		cl_file64::remove_file(desfullpath.c_str());
		if(0!=cl_util::file_rename(fi->fullpath,desfullpath))
		{
			jni_fire_download_fini(fi->fhash,desfullpath,-2);
			return -1;
		}
		if(pos1>0)
			fi->name.erase(pos1); //可能为空
		fi->fullpath = desfullpath;
	}
	//保存sha3文件
	cl_put_fhash_to_hashfile(fi->fullpath+".sha3",fi->fhash,fi->sh.subhash);
	
	fi->mtime = cl_util::time_to_datetime_string(time(NULL));
	fi->delete_infofile();
	m_downInfo.erase(it);
	fi->no = m_max_no++;
	fi->bready = true;
	m_readyInfo[fi->fhash] = fi;
	m_seg_fchsum.ff_checksum += cl_crc32_write(CL_CRC32_FIRST,(const unsigned char*)fi->fhash.c_str(),fi->fhash.length());
	m_seg_fchsum.ff_num = m_readyInfo.size();

	save_downinfo_main();
	save_readyinfo();
	report_finifile(fi);
	jni_fire_download_fini(fi->fhash,fi->fullpath,0);
	return 0;
}

void cly_filemgr::report_finifile(cly_fileinfo* fi)
{
	char buf[1024];
	sprintf(buf,"1|%s|%d|%s|%s|%lld,%lld,%lld,%lld,%lld",fi->fhash.c_str(),fi->ftype
		,fi->name.c_str(),fi->sh.subhash.c_str(),fi->rcvB[0],fi->rcvB[1],fi->rcvB[2],fi->rcvB[3],fi->rcvB[4]);
	DEBUGMSG("#report_finifile:: %s\n",buf);
	cly_cTrackerSngl::instance()->report_fdfile(buf,&m_seg_fchsum);
}
void cly_filemgr::report_delefile(cly_fileinfo* fi)
{
	char buf[1024];
	sprintf(buf,"%d|%s|%d",CLY_FDTYPE_DELETE,fi->fhash.c_str(),fi->ftype);
	cly_cTrackerSngl::instance()->report_fdfile(buf,&m_seg_fchsum);
}

