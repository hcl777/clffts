#include "cly_downAuto.h"
#include "cl_util.h"
#include "cly_config.h"
#include "cly_downloadMgr.h"
#include "cly_filemgr.h"
#include "cly_cTracker.h"


#define CLY_ACTIVE_LOOT 1
#define CLY_ACTIVE_MAX_SCORE 2

cly_downAuto::cly_downAuto(void)
: m_iCurrTick(0)
, m_binitactive(false)
, m_haveSpeed(false)
, m_bpause(false)
, m_updateid("0")
{
}

cly_downAuto::~cly_downAuto(void)
{
	clear();
}

int cly_downAuto::Init()
{
	if(0==cly_pc->main_process)
		return 0;

	Load();

	cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),1,5000);
	//10秒后激活任务
	cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),2,10000);

	//每分钟调整一次任务优先级,有速度的时候才调整
	cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),3,120000);

	return 0;
}
int cly_downAuto::Fini()
{
	if(0==cly_pc->main_process)
		return 0;

	cl_timerSngl::instance()->unregister_all(static_cast<cl_timerHandler*>(this));
	//Save();
	clear();
	return 0;
}
void cly_downAuto::update_timer(int timer_getddlistS,int timer_reportprogressS)
{
	//5分钟ddlist
	int ms;

	if(-1==cly_pc->timer_get_ddlistS)
		cly_pc->timer_get_ddlistS = timer_getddlistS;
	if(-1==cly_pc->timer_report_progressS)
		cly_pc->timer_report_progressS = timer_reportprogressS;

	ms = cly_pc->timer_get_ddlistS * 1000;
	if(ms>0 && ms<5000) ms = 5000;
	if(ms>0)
	{
		cly_cTrackerSngl::instance()->get_ddlist(m_updateid);
		cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),4,ms);
	}
	else
		cl_timerSngl::instance()->unregister_timer(static_cast<cl_timerHandler*>(this),4);

	//report_progress
	ms = cly_pc->timer_report_progressS * 1000;
	if(ms>0 && ms<2000) ms = 2000;
	if(ms>0)
		cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),5,ms);
	else
		cl_timerSngl::instance()->unregister_timer(static_cast<cl_timerHandler*>(this),5);

}
int cly_downAuto::AddDown(const string& hash,const string& filename,int ftype,int priority,int save_original)
{
	if(!CLY_IS_USED(cly_pc->used, CLY_USED_DOWNLOAD) || 0==cly_pc->main_process)
		return -1;

	InitActivat();

	//如果文件已经存在则不再添加
	cl_TLock<Mutex> l(m_mt);
	cly_fileinfo *fi = cly_filemgrSngl::instance()->get_readyinfo(hash);
	if (fi)
	{
		return 1;
	}
	string name = filename;
	if(name.empty())
	{
		name = hash;
		name += ".mkv";
	}
	if('/'==name.at(0) || '\\'==name.at(0))
		name.erase(0,1);
	//如果文件存在down_path 或者 share_path，也不再下载
	string fullpath = CLYSET->find_exist_fullpath(name);
	if(!fullpath.empty())
	{
		DEBUGMSG("# ***cly_downAuto::AddDown(%s) fail!, file exist(%s) by not share!!!\n",hash.c_str(),fullpath.c_str());
		return 1;
	}

	cly_downAutoInfo *inf = FindDown(hash);
	if(NULL==inf)
	{
		inf = new cly_downAutoInfo();
		inf->hash = hash;
		inf->size = 0;
		inf->name = name;
		inf->ftype = ftype;
		inf->priority = priority;
		inf->createtime = cl_util::time_to_datetime_string(time(NULL));
		inf->faileds = 0;
		inf->progress = 0;
		inf->speed = 0;
		inf->state = 2;
		inf->save_original = save_original;
		

		m_lsWaiting.push_back(inf);
		DownActive(CLY_ACTIVE_MAX_SCORE);
	}
	else
	{
		inf->priority = priority;
		inf->save_original = save_original;
	}

	DownAdjust();
	Save();
	return 0;
}
cly_downAutoInfo *cly_downAuto::FindDown(const string& hash)
{
	FileIter it;
	for(it=m_lsDowning.begin();it!=m_lsDowning.end();++it)
	{
		if((*it)->hash == hash)
		{
			return *it;
		}
	}
	for(it=m_lsWaiting.begin();it!=m_lsWaiting.end();++it)
	{
		if((*it)->hash == hash)
		{
			return *it;
		}
	}
	for(it=m_lsStop.begin();it!=m_lsStop.end();++it)
	{
		if((*it)->hash == hash)
		{
			return *it;
		}
	}
	return NULL;
}

int cly_downAuto::DelDown(const string& hash)
{
	//second_peer 不支持下载
	if(0==cly_pc->main_process)
		return -1;

	cl_TLock<Mutex> l(m_mt);
	FileIter it;
	bool bdel = false;
	
	for(it=m_lsStop.begin();!bdel && it!=m_lsStop.end();++it)
	{
		if((*it)->hash == hash)
		{
			delete *it;
			m_lsStop.erase(it);
			bdel = true;
			break;
		}
	}
	for(it=m_lsWaiting.begin();!bdel && it!=m_lsWaiting.end();++it)
	{
		if((*it)->hash == hash)
		{
			delete *it;
			m_lsWaiting.erase(it);
			bdel = true;
			break;
		}
	}
	for(it=m_lsDowning.begin();!bdel && it!=m_lsDowning.end();++it)
	{
		if((*it)->hash == hash)
		{
			cly_downloadMgrSngl::instance()->release(hash);
			delete *it;
			m_lsDowning.erase(it);
			bdel = true;
			break;
		}
	}
	if(bdel)
	{
		Save();
		if(NULL==cly_filemgrSngl::instance()->get_fileinfo(hash))
		{
			//未创建的任务，要报告，让数据库清记录
			char buf[1024];
			sprintf(buf,"%d|%s|%d",CLY_FDTYPE_DELETE,hash.c_str(),FTYPE_DISTRIBUTION);
			cly_cTrackerSngl::instance()->report_fdfile(buf,NULL);
		}
	}

	return 0;
}

int cly_downAuto::Start(const string& hash)
{
	//如果在waiting队列，则直接启动
	cl_TLock<Mutex> l(m_mt);

	if(hash == "all" || hash == "ALL")
	{
		this->resume();
		return 0;
	}
	if(m_bpause || cly_pc->down_active_num<=0)
		return -1;
	cly_downAutoInfo* inf = NULL;
	inf = DetachInfo(m_lsWaiting,hash);
	if(NULL==inf)
		inf = DetachInfo(m_lsStop,hash);
	if(NULL==inf)
		return -1;

	string queuehash;
	//如果并发已满，则删除一个旧下载
	if((int)m_lsDowning.size() >= cly_pc->down_active_num)
	{
		FileIter min_it = GetMinScoreInfo(m_lsDowning);
		cly_downAutoInfo* p = *min_it;
		queuehash = p->hash;
		cly_downloadMgrSngl::instance()->release(p->hash);
		m_lsDowning.erase(min_it);
		m_lsWaiting.push_back(p);
	}
	ActiveI(inf);
	DEBUGMSG("# DownAuto::Start(%s),queue(%s) \n",hash.c_str(),queuehash.c_str());
	Save();
	return 0;
}
int cly_downAuto::Stop(const string& hash)
{
	cl_TLock<Mutex> l(m_mt);
	if(hash == "all" || hash == "ALL")
	{
		this->pause();
		return 0;
	}

	if(m_bpause || cly_pc->down_active_num<=0)
		return -1;
	
	cly_downAutoInfo* inf = NULL;
	inf = DetachInfo(m_lsWaiting,hash);
	if(NULL==inf)
	{
		inf = DetachInfo(m_lsDowning,hash);
		if(inf)
		{
			cly_downloadMgrSngl::instance()->release(inf->hash);
			DownActive(CLY_ACTIVE_MAX_SCORE);
		}
	}
	if(inf)
	{
		m_lsStop.push_back(inf);
		Save();
		DEBUGMSG("# DownAuto::Stop(%s) \n",hash.c_str());
	}
	
	return 0;
}
int cly_downAuto::GetDownInfo(FileList& ls)
{
	cl_TLock<Mutex> l(m_mt);
	cly_downAutoInfo *inf;
	FileIter it;
	for(it=m_lsDowning.begin();it!=m_lsDowning.end();++it)
	{
		inf = new cly_downAutoInfo();
		*inf = **it;
		inf->state = 1;
		ls.push_back(inf);
	}
	for(it=m_lsWaiting.begin();it!=m_lsWaiting.end();++it)
	{
		inf = new cly_downAutoInfo();
		*inf = **it;
		inf->state = 2;
		ls.push_back(inf);
	}
	for(it=m_lsStop.begin();it!=m_lsStop.end();++it)
	{
		inf = new cly_downAutoInfo();
		*inf = **it;
		inf->state = 0;
		ls.push_back(inf);
	}
	return 0;
}
void cly_downAuto::report_progress()
{
	//report progress
	list<string> ls;
	char buf[1024];
	cly_downAutoInfo* inf;
	for(FileIter it=m_lsDowning.begin();it!=m_lsDowning.end();++it)
	{
		inf = *it;
		sprintf(buf,"%s|%d|%s|%d|%d|%d",inf->hash.c_str(),inf->ftype,
			inf->name.c_str(),(inf->speed>>10),inf->progress,inf->faileds);
		ls.push_back(buf);
	}
	if(!ls.empty())
		cly_cTrackerSngl::instance()->report_progress(ls);
}
void cly_downAuto::on_timer(int e)
{
	switch(e)
	{
	case 1:
		{
			//定时更新速度信息
			UpdateDownInfo();
		}
		break;
	case 2:
		{
			cl_timerSngl::instance()->unregister_timer(static_cast<cl_timerHandler*>(this),2);
			InitActivat();
		}
		break;
	case 3:
		{
			if(m_haveSpeed)
				DownAdjust();
		}
		break;
	case 4:
		cly_cTrackerSngl::instance()->get_ddlist(m_updateid);
		break;
	case 5:
		report_progress();
		break;
	default:
		assert(0);
		break;
	}
}
void cly_downAuto::pause()
{
	DEBUGMSG("# cly_downAuto::pause() \n");
	if(m_bpause)
		return;
	m_bpause = true;
	FileIter it;
	for(it=m_lsDowning.begin();it!=m_lsDowning.end();++it)
	{
		//停止
		cly_downloadMgrSngl::instance()->release((*it)->hash);
		m_lsWaiting.push_back((*it));
	}
	m_lsDowning.clear();
}
void cly_downAuto::resume()
{
	DEBUGMSG("# cly_downAuto::resume() \n");
	if(!m_bpause)
		return;
	m_bpause = false;
	DownActive(CLY_ACTIVE_MAX_SCORE);
}

bool cly_downAuto::ActiveI(cly_downAutoInfo *inf)
{
	if(NULL==inf)
		return false;

	inf->last_changed_time = (unsigned int)time(NULL);

	if(-1!=cly_downloadMgrSngl::instance()->create_download(inf->hash,inf->ftype,inf->name,inf->save_original>0?true:false))
	{
		cly_downloadMgrSngl::instance()->refer(inf->hash);
		inf->downtick = 0;
		inf->zerospeedtick = 0;
		m_lsDowning.push_back(inf);
		DEBUGMSG("# DownAuto::Active (p=%d) \n",inf->priority);
	}
	else
	{
		//assert(0);
		inf->faileds++;
		m_lsWaiting.push_back(inf);
		DEBUGMSG("#*** DownAuto::ActiveI(%s) fail! \n",inf->name.c_str());
		return false;
	}
	return true;
}

cly_downAuto::FileIter cly_downAuto::GetMaxScoreInfo(cly_downAuto::FileList& ls)
{
	unsigned int t = (unsigned int)time(NULL);
	FileIter it,it2;
	it = it2 = ls.begin();
	int score2 = (*it2)->score(t);
	int score = 0;
	for(++it; it!=ls.end();++it)
	{
		score = (*it)->score(t);
		if(score>score2)
		{
			score2 = score;
			it2 = it;
		}
	}
	return it2;
}
cly_downAuto::FileIter cly_downAuto::GetMinScoreInfo(cly_downAuto::FileList& ls)
{
	unsigned int t = (unsigned int)time(NULL);
	FileIter it,it2;
	it = it2 = ls.begin();
	int score2 = (*it2)->score(t);
	int score = 0;
	for(++it; it!=ls.end();++it)
	{
		score = (*it)->score(t);
		if(score<score2)
		{
			score2 = score;
			it2 = it;
		}
	}
	return it2;
}

cly_downAuto::FileIter cly_downAuto::GetInfo(cly_downAuto::FileList& ls,const string& hash)
{
	FileIter it=ls.begin();
	for(;it!=ls.end();++it)
	{
		if((*it)->hash==hash)
			break;
	}
	return it;
}

cly_downAutoInfo* cly_downAuto::DetachInfo(FileList& ls,const string& hash)
{
	cly_downAutoInfo* inf=NULL;
	for(FileIter it=ls.begin();it!=ls.end();++it)
	{
		if((*it)->hash==hash)
		{
			inf = *it;
			ls.erase(it);
			return inf;
		}
	}
	return NULL;
}
void cly_downAuto::InitActivat()
{
	if(!m_binitactive)
	{
		cl_TLock<Mutex> l(m_mt);

		if(m_bpause)
			return;
		m_binitactive = true;
		cly_downAutoInfo *inf = NULL;
		FileIter it = m_lsWaiting.begin();
		for(;it!=m_lsWaiting.end() && (int)m_lsDowning.size() < cly_pc->down_active_num;)
		{
			inf = *it;
			if(inf->state==1)
			{
				m_lsWaiting.erase(it++);
				if(!ActiveI(inf))
					break;
			}
			else
				++it;
		}
	}
	DownActive(CLY_ACTIVE_MAX_SCORE);
}
void cly_downAuto::DownActive(int tactics)
{
	cl_TLock<Mutex> l(m_mt);
	if(m_bpause)
		return;
	//任务不够的先加够，再检查是否有更高级的任务在等待队列
	cly_downAutoInfo *inf = NULL;
	while(!m_lsWaiting.empty() && (int)m_lsDowning.size() < cly_pc->down_active_num)
	{
		if(CLY_ACTIVE_LOOT==tactics)
		{
			//无网速时轮循
			inf = m_lsWaiting.front();
			m_lsWaiting.pop_front();
		}
		else
		{
			//找最高分的任务下载
			FileIter it = GetMaxScoreInfo(m_lsWaiting);
			inf = *it;
			m_lsWaiting.erase(it);
		}
		if(!ActiveI(inf))
			break;
	}
}

void cly_downAuto::DownAdjust()
{
	cl_TLock<Mutex> l(m_mt);
	if(m_bpause)
		return;
	if(m_lsDowning.empty() || m_lsWaiting.empty())
		return;

	cly_downAutoInfo *inf = NULL,*infnew=NULL;
	unsigned int t = (unsigned int)time(NULL);
	FileIter min_it = GetMinScoreInfo(m_lsDowning);
	FileIter max_it = GetMaxScoreInfo(m_lsWaiting);
	inf = *min_it;
	infnew = *max_it;
	if( inf->score(t) < infnew->score(t) )
	{
		//停止
		cly_downloadMgrSngl::instance()->release(inf->hash);
		m_lsDowning.erase(min_it);
		m_lsWaiting.erase(max_it);
		m_lsWaiting.push_back(inf);
		ActiveI(infnew);
		DEBUGMSG("# DownAuto::DownAdjust 1 \n");
	}
}
void cly_downAuto::UpdateDownInfo()
{
	cl_TLock<Mutex> l(m_mt);
	//每5秒执行一次
	FileIter it;
	cly_downloadInfo_t di;
	cly_downAutoInfo *inf=NULL;
	bool bEliminate = false;
	if(m_lsDowning.empty())
		return;
	for(it=m_lsDowning.begin();it!=m_lsDowning.end();++it)
	{
		inf = *it;
		inf->downtick += 5;
		if(0==cly_downloadMgrSngl::instance()->get_fileinfo(inf->hash,di))
		{
			if(0==inf->speed)
			{
				inf->zerospeedtick += 5;
				DEBUGMSG("# zerospeedtick = %d \n",inf->zerospeedtick);
			}
			else
			{
				inf->zerospeedtick = 0;
				inf->streak_faileds = 0;
				m_haveSpeed = true;
			}
			inf->size = di.size;
			inf->speed = di.speedB;
			inf->progress = di.progress;
			inf->path = di.path;
			if(2==di.state)
			{
				DEBUGMSG("# DownAuto:: finished download 1 \n");
				cly_downloadMgrSngl::instance()->release((*it)->hash);
				delete *it;
				m_lsDowning.erase(it);
				DownActive(CLY_ACTIVE_MAX_SCORE);
				Save();
				break;
			}
			if(inf->zerospeedtick>=30)
			{
				DEBUGMSG("# DownAuto::speed=0 Eliminate 1 \n");
				cly_downloadMgrSngl::instance()->release((*it)->hash);
				m_lsDowning.erase(it);
				bEliminate = true;
				m_haveSpeed = false;
				break;
			}
		}
		else
		{
			//下载错误已经删除任务,放到等待队列再尝试重下
			m_lsDowning.erase(it);
			bEliminate = true;
			break;
		}
	}
	if(bEliminate)
	{
		inf->faileds++;
		inf->streak_faileds++;
		//if(inf->streak_faileds>200)
		if(0)
		{
			delete inf;
			DEBUGMSG("# *****DownAuto::speed=0 delete 1 \n");
		}
		else
		{
			m_lsWaiting.push_back(inf);
		}
		DownActive(CLY_ACTIVE_LOOT);
		Save();
	}
}

void cly_downAuto::Load()
{
	//tth|size|name|path|priority|createtime|faileds|progress|speed|state|ftype|save_original
	cl_TLock<Mutex> l(m_mt);
	list<string> ls;
	string str;
	cly_downAutoInfo *inf=NULL;
	string path = CLY_DIR_AUTO + "downauto.dat";
	cl_util::get_stringlist_from_file(path,ls);
	//DEBUGMSG("cly_downAuto::Load() get_stringlist_from_file() ok \n");
	if(!ls.empty())
	{
		m_updateid = ls.front();
		if(m_updateid.empty()) m_updateid = "0";
		ls.pop_front();
		if(!ls.empty()) ls.pop_front();
		for(list<string>::iterator it=ls.begin();it!=ls.end();++it)
		{
			str = *it;
			cl_util::string_trim(str);
			if(str.empty())
				continue;
			inf = new cly_downAutoInfo();
			inf->hash = cl_util::get_string_index(str,0,"|");
			inf->size = cl_util::atoll(cl_util::get_string_index(str,1,"|").c_str());
			inf->name = cl_util::get_string_index(str,2,"|");
			inf->path = cl_util::get_string_index(str,3,"|");
			inf->priority = atoi(cl_util::get_string_index(str,4,"|").c_str());
			inf->createtime = cl_util::get_string_index(str,5,"|");
			inf->faileds = atoi(cl_util::get_string_index(str,6,"|").c_str());
			inf->progress = atoi(cl_util::get_string_index(str,7,"|").c_str());
			inf->state = atoi(cl_util::get_string_index(str,9,"|").c_str());
			inf->ftype = atoi(cl_util::get_string_index(str,10,"|").c_str());
			inf->save_original = atoi(cl_util::get_string_index(str, 11, "|").c_str());
			inf->speed = 0;
			if(FindDown(inf->hash))
				delete inf;
			else
			{
				if(0==inf->state)
					m_lsStop.push_back(inf);
				else
					m_lsWaiting.push_back(inf);
			}
		}
	}
}

void cly_downAuto::Save()
{
	//hash|size|name|path|priority|createtime|faileds|progress|speed|state|ftype|save_original
	cl_TLock<Mutex> l(m_mt);
	list<string> ls;
	string str;
	cly_downAutoInfo *inf=NULL;
	string path =  CLY_DIR_AUTO + "downauto.dat";
	char buf[2048];
	FileIter it;

	ls.push_back(m_updateid);
	ls.push_back("hash|size|name|path|priority|createtime|faileds|progress|speed|state|ftype|save_original");
	for(it=m_lsDowning.begin();it!=m_lsDowning.end();++it)
	{
		inf = *it;
		sprintf(buf,"%s|%lld|%s|%s|%d|%s|%d|%d|%d|1|%d|%d",
			inf->hash.c_str(),inf->size,inf->name.c_str(),inf->path.c_str(),inf->priority,inf->createtime.c_str(),inf->faileds,inf->progress,inf->speed,inf->ftype,inf->save_original);
		ls.push_back(buf);
	}
	for(it=m_lsWaiting.begin();it!=m_lsWaiting.end();++it)
	{
		inf = *it;
		sprintf(buf,"%s|%lld|%s|%s|%d|%s|%d|%d|0|2|%d|%d",
			inf->hash.c_str(),inf->size,inf->name.c_str(),inf->path.c_str(),inf->priority,inf->createtime.c_str(),inf->faileds,inf->progress,inf->ftype,inf->save_original);
		ls.push_back(buf);
	}
	for(it=m_lsStop.begin();it!=m_lsStop.end();++it)
	{
		inf = *it;
		sprintf(buf,"%s|%lld|%s|%s|%d|%s|%d|%d|0|0|%d|%d",
			inf->hash.c_str(),inf->size,inf->name.c_str(),inf->path.c_str(),inf->priority,inf->createtime.c_str(),inf->faileds,inf->progress,inf->ftype, inf->save_original);
		ls.push_back(buf);
	}

	if(!ls.empty())
		cl_util::put_stringlist_to_file(path,ls);
	else
		cl_util::file_delete(path);
}

void cly_downAuto::clear()
{
	FileIter it;
	for(it=m_lsWaiting.begin();it!=m_lsWaiting.end();++it)
		delete (cly_downAutoInfo*)(*it);
	for(it=m_lsDowning.begin();it!=m_lsDowning.end();++it)
		delete (cly_downAutoInfo*)(*it);

	m_lsWaiting.clear();
	m_lsDowning.clear();
}
void cly_downAuto::load_ddlist(const string& path)
{
	string hash,name;
	int ftype = FTYPE_DISTRIBUTION;
	int priority;
	int fdtype = 0;
	//fdtype|hash|priority|name
	list<string> ls;
	cl_util::get_stringlist_from_file(path,ls);
	if(ls.size()<2)
		return;
	m_updateid = ls.front();
	ls.pop_front();
	ls.pop_front();//暂忽略strformat
	if(m_updateid.empty()) m_updateid = "0";
	for(list<string>::iterator it=ls.begin();it!=ls.end();++it)
	{
		string& str = *it;
		fdtype = atoi(cl_util::get_string_index(str,0,"|").c_str());
		hash = cl_util::get_string_index(str,1,"|");
		priority = atoi(cl_util::get_string_index(str,2,"|").c_str());
		name = cl_util::get_string_index(str,3,"|");
		if(CLY_FDTYPE_DOWN==fdtype)
			AddDown(hash,name,ftype,priority,0);
		else if(CLY_FDTYPE_DELETE==fdtype)
		{
			DelDown(hash);
			cly_downloadMgrSngl::instance()->delete_file(hash,priority?true:false);
		}
	}
}
