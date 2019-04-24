#include "cly_downloadMgr.h"
#include "cly_filemgr.h"
#include "cly_config.h"

cly_downloadMgr::cly_downloadMgr(void)
{
}


cly_downloadMgr::~cly_downloadMgr(void)
{
}

int cly_downloadMgr::init()
{
	cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),1,1000);
	return 0;
}
void cly_downloadMgr::fini()
{
	cl_timerSngl::instance()->unregister_all(static_cast<cl_timerHandler*>(this));
	for(DownloadIter it=m_downloads.begin();it!=m_downloads.end();++it)
	{
		it->second.dl->close();
		delete it->second.dl;
	}
}
void cly_downloadMgr::on_timer(int e)
{
	DWORD tick = GetTickCount();

	while(!m_release.empty())
	{
		releaseInfo_t ri = m_release.front();
		if(_timer_after(tick,ri.beginTick+3000))
		{
			m_release.pop_front();
			cly_filemgrSngl::instance()->release_file(ri.hash);
			DownloadIter it = m_downloads.find(ri.hash);
			if(it!=m_downloads.end())
			{
				task_t& t = it->second;
				if((--t.ref)<=0)
				{
					t.dl->close();
					delete t.dl;
					m_downloads.erase(it);
				}
			}
		}
		else
			break;
	}

	//检查下载完成
	for(DownloadIter it=m_downloads.begin();it!=m_downloads.end();)
	{
		if(it->second.dl->is_finished())
		{
			it->second.dl->close();
			delete it->second.dl;
			m_downloads.erase(it++);
			DEBUGMSG(" cly_downloadMgr::download file fini !!!\n");
		}
		else
			++it;
	}
}

int cly_downloadMgr::create_download(const string& hash,int ftype,const string& path, bool bsave_original)
{
	if(!CLY_IS_USED(cly_pc->used, CLY_USED_DOWNLOAD) || !cly_pc->main_process || hash.empty()||path.empty())
		return -1;
	////test
	cly_fileinfo* fi = cly_filemgrSngl::instance()->get_readyinfo(hash);
	if(fi) 
		return 1;
	if(m_downloads.find(hash)!=m_downloads.end())
		return 1;
	task_t t;
	t.ref = 0;
	t.dl = new cly_download();
	if(0!=t.dl->create(hash,ftype,path, bsave_original))
	{
		delete t.dl;
		return -1;
	}
	m_downloads[hash] = t;
	return 0;
}
int cly_downloadMgr::delete_file(const string& hash,bool isDelPhy)
{
	cly_filemgrSngl::instance()->release_file(hash);
	DownloadIter it = m_downloads.find(hash);
	if(it!=m_downloads.end())
	{
		task_t& t = it->second;
		t.dl->close();
		delete t.dl;
		m_downloads.erase(it);
	}
	return cly_filemgrSngl::instance()->delete_info(hash,isDelPhy);
}
void cly_downloadMgr::refer(const string& hash)
{
	cly_filemgrSngl::instance()->refer_file(hash);
	DownloadIter it = m_downloads.find(hash);
	if(it!=m_downloads.end())
		it->second.ref++;
}
void cly_downloadMgr::release(const string& hash)
{
	m_release.push_back(releaseInfo_t(hash,GetTickCount()));
}
int cly_downloadMgr::add_source(const string& hash,list<string>& ls)
{
	DownloadIter it = m_downloads.find(hash);
	if(it!=m_downloads.end())
		it->second.dl->add_source_list(ls);
	return 0;
}
int cly_downloadMgr::get_fileinfo(const string& hash,cly_downloadInfo_t& inf)
{
	DownloadIter it = m_downloads.find(hash);
	if(it!=m_downloads.end())
		return it->second.dl->get_downloadinfo(inf);

	cly_fileinfo *fi = cly_filemgrSngl::instance()->get_readyinfo(hash);
	if(fi)
	{
		inf.hash = hash;
		inf.state = 2; //完成
		inf.ftype = fi->ftype;
		inf.size = fi->sh.size;
		inf.path = fi->fullpath;
		inf.progress = 1000;
		inf.speedB = 0;
		inf.connNum = 0;
		inf.srcNum = 0;
		return 0;
	}
	return -1;
}
int cly_downloadMgr::get_allspeed(int& allspeed,list<string>& ls)
{
	allspeed = 0;
	cly_downloadInfo_t inf;
	char buf[1024];
	for(DownloadIter it=m_downloads.begin();it!=m_downloads.end();++it)
	{
		it->second.dl->get_downloadinfo(inf);
		allspeed += inf.speedB;
		sprintf(buf,"%s|%5d kb|%5.1f%%|%s|%lld",inf.hash.c_str(),(int)(inf.speedB>>10),inf.progress/(double)10,inf.path.c_str(),inf.size);
		ls.push_back(buf);
	}
	return 0;
}
int cly_downloadMgr::get_conn_num()
{
	int n = 0;
	for (DownloadIter it = m_downloads.begin(); it != m_downloads.end(); ++it)
		n += it->second.dl->get_conn_num();
	return n;
}
