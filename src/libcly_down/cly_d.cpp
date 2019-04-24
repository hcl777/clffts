#include "cly_d.h"
#include "cly_schedule.h"
#include "cl_result.h"
#include "cly_downloadMgr.h"
#include "cly_filemgr.h"
#include "cly_downAuto.h"
#include "cl_util.h"
#include "cly_cTracker.h"
#include "cly_config.h"
#include "cly_server.h"

void cly_print_help()
{
	printf("%-12s = %s\n","[version]",CLY_DOWN_VERSION);
}

//**********************************************
cly_d::cly_d(void)
	:m_binit(false)
	, m_unused_down_times(0)
{
}

cly_d::~cly_d(void)
{
}
int cly_d::init(cly_config_t* pc)
{
	if(m_binit) return 1;
	if(0==cly_schedule_sngl::instance()->run(pc))
	{
		m_binit = true;
		return 0;
	}
	return -1;
}
void cly_d::fini()
{
	if(!m_binit)
		return;
	m_binit =false;

	cly_schedule_sngl::instance()->end();
	cly_schedule_sngl::destroy();
	m_msgQueue.ClearMessage(_ClearMessageFun);
}

//******************************************
enum {
	CLYCMD_CREATE_DOWNLOAD,
	CLYCMD_LOAD_DDLIST,
	CLYCMD_REFER_FILE,
	CLYCMD_RELEASE_FILE,
	CLYCMD_DELETE_FILE,
	CLYCMD_ADD_SOURCE,
	CLYCMD_CHECK_EXIST_SHARE_BY_PATH,
	CLYCMD_SHARE_FILE,
	CLYCMD_SAVE_READYFILE_UTF8,
	CLYCMD_UPDATE_PROGRESS,
	CLYCMD_GET_STATE,
	CLYCMD_GET_FILEINFO,
	CLYCMD_GET_READYINFO_BY_PATH,

	CLYCMD_DOWNAUTO_ADD,
	CLYCMD_DOWNAUTO_ALLINFO,
	CLYCMD_DOWNAUTO_START,
	CLYCMD_DOWNAUTO_STOP,

	CLYCMD_LOGIN_CONFIG,
	CLYCMD_KEEPALIVE_ACK
};
typedef struct tag_clycmd_fileinfo
{
	string	hash;
	string	subhash;
	string	path;
	string	name;
	int		ftype;
	int		priority;
	int		save_original;
	void*	p;
	int		ret;
}clycmd_fileinfo_t;

//********************************************

void cly_d::handle_root()
{
	cl_Message* msg = NULL;
	//int ret = 0;
	while((msg=m_msgQueue.GetMessage(0)))
	{
		switch(msg->cmd)
		{
		case CLYCMD_CREATE_DOWNLOAD:
			{
				clycmd_fileinfo_t* inf = (clycmd_fileinfo_t*)msg->data;
				inf->ret = cly_downloadMgrSngl::instance()->create_download(inf->hash,inf->ftype,inf->path,0);
			}
			break;
		case CLYCMD_REFER_FILE:
			{
				cly_downloadMgrSngl::instance()->refer(*((string*)msg->data));
			}
			break;
		case CLYCMD_RELEASE_FILE:
			{
				cly_downloadMgrSngl::instance()->release(*((string*)msg->data));
			}
			break;
		case CLYCMD_DELETE_FILE:
			{
				clycmd_fileinfo_t* inf = (clycmd_fileinfo_t*)msg->data;
				//先删除downauto
				cly_downAutoSngl::instance()->DelDown(inf->hash);
				inf->ret = cly_downloadMgrSngl::instance()->delete_file(inf->hash,inf->ftype?true:false);
			}
			break;
		case CLYCMD_ADD_SOURCE:
			{
				clycmd_fileinfo_t* inf = (clycmd_fileinfo_t*)msg->data;
				inf->ret = cly_downloadMgrSngl::instance()->add_source(inf->hash,*(list<string>*)inf->p);
			}
			break;
		case CLYCMD_CHECK_EXIST_SHARE_BY_PATH:
			{
				clycmd_fileinfo_t* inf = (clycmd_fileinfo_t*)msg->data;
				inf->ret = cly_filemgrSngl::instance()->check_exist_share_bypath(inf->path);
			}
			break;
		case CLYCMD_SHARE_FILE:
			{
				clycmd_fileinfo_t* inf = (clycmd_fileinfo_t*)msg->data;
				inf->ret = cly_filemgrSngl::instance()->create_readyinfo(inf->hash,inf->subhash,inf->path,inf->name,inf->ftype?true:false);
			}
			break;
		case CLYCMD_SAVE_READYFILE_UTF8:
			{
				clycmd_fileinfo_t* inf = (clycmd_fileinfo_t*)msg->data;
				inf->ret = cly_filemgrSngl::instance()->save_readyfile_utf8(inf->path);
			}
			break;
		case CLYCMD_UPDATE_PROGRESS:
			{
				cly_downAutoSngl::instance()->report_progress();
			}
			break;
		case CLYCMD_GET_STATE:
			{
				clycmd_fileinfo_t* inf = (clycmd_fileinfo_t*)msg->data;
				clyd_state_t& s = *(clyd_state_t*)inf->p;
				//
				s.peer_name = cly_pc->peer_name;
				s.peer_id = cly_pc->peer_id;
				s.used = cly_pc->used;
				s.login_state = cly_cTrackerSngl::instance()->is_login()?1:0;
				s.login_fails = cly_cTrackerSngl::instance()->m_login_fails;
				s.keepalive_fails = cly_cTrackerSngl::instance()->m_keepalive_fails;
				s.downauto_pause = cly_downAutoSngl::instance()->is_pause() ? 1 : 0;
				s.unused_down_times = m_unused_down_times;
				s.ready_num = cly_filemgrSngl::instance()->get_ready_num();
				s.free_space_GB = CLYSET->get_freespace_GB();
				s.downconn_num = cly_downloadMgrSngl::instance()->get_conn_num();
				cly_downloadMgrSngl::instance()->get_allspeed(s.down_speed,s.downi_speeds);
				cly_serverSngl::instance()->get_allspeed(s.up_speed,s.upi_speeds);
				
				UAC_statspeed_t ss;
				uac_get_statspeed(ss);
				s.sendspeedB = ss.sendspeedB;
				s.valid_sendspeedB = ss.valid_sendspeedB;
				s.recvspeedB = ss.recvspeedB;
				s.valid_recvspeedB = ss.valid_recvspeedB;
				s.app_recvspeedB = ss.app_recvspeedB;
				s.sendfaild_count = ss.sendfaild_count;
				for (int i = 0; i < 8; ++i)
					s.sendfaild_errs[i] = ss.sendfaild_errs[i];

				inf->ret = 0;
			}
			break;	
		case CLYCMD_GET_FILEINFO:
			{
				clycmd_fileinfo_t* inf = (clycmd_fileinfo_t*)msg->data;
				cly_downloadInfo_t& di = *(cly_downloadInfo_t*)inf->p;
				inf->ret = cly_downloadMgrSngl::instance()->get_fileinfo(inf->hash,di);
			}
			break;	
		case CLYCMD_GET_READYINFO_BY_PATH:
			{
				clycmd_fileinfo_t* inf = (clycmd_fileinfo_t*)msg->data;
				cly_readyInfo_t& ri = *(cly_readyInfo_t*)inf->p;
				cly_fileinfo* fi = cly_filemgrSngl::instance()->get_readyinfo_by_path(inf->path);
				if(NULL!=fi)
				{
					inf->ret = 0;
					ri.hash = fi->fhash;
					ri.subhash = fi->sh.subhash;
					ri.ftype = fi->ftype;
					ri.name = fi->name;
					ri.path = fi->fullpath;
				}
				else
				{
					inf->ret = -1;
				}
			}
			break;	
			
		case CLYCMD_LOAD_DDLIST:
			{
				cly_downAutoSngl::instance()->load_ddlist(*((string*)msg->data));
			}
			break;
		case CLYCMD_DOWNAUTO_ADD:
			{
				clycmd_fileinfo_t* inf = (clycmd_fileinfo_t*)msg->data;
				inf->ret = cly_downAutoSngl::instance()->AddDown(inf->hash,inf->path,inf->ftype,inf->priority,inf->save_original);
			}
			break;
		case CLYCMD_DOWNAUTO_ALLINFO:
			{
				DownAutoInfoList *ls = (DownAutoInfoList*)msg->data;
				cly_downAutoSngl::instance()->GetDownInfo(*ls);
			}
			break;
		case CLYCMD_DOWNAUTO_START:
			{
				clycmd_fileinfo_t* inf = (clycmd_fileinfo_t*)msg->data;
				inf->ret = cly_downAutoSngl::instance()->Start(inf->hash);
			}
			break;
		case CLYCMD_DOWNAUTO_STOP:
			{
				clycmd_fileinfo_t* inf = (clycmd_fileinfo_t*)msg->data;
				inf->ret = cly_downAutoSngl::instance()->Stop(inf->hash);
			}
			break;
		case CLYCMD_LOGIN_CONFIG:
			{
				cly_seg_config_t* conf = (cly_seg_config_t*)msg->data;
				cly_pc->peer_id = conf->peer_id;
				cly_pc->group_id = conf->group_id;
					
				//如果本地未配置，则使用网络配置。（优先使用本地）
				CLYSET->update_speed(conf->limit_share_speediKB,conf->limit_share_speedKB,conf->limit_down_speedKB,true);
				cly_downAutoSngl::instance()->update_timer(conf->timer_getddlistS,conf->timer_reportprogressS);
				update_used(conf->used);
			}
			break;
		case CLYCMD_KEEPALIVE_ACK:
			{
				cly_seg_keepalive_ack_t* seg = (cly_seg_keepalive_ack_t*)msg->data;
				update_used(seg->used);
			}
			break;
		default:
			break;

		}
		_ClearMessageFun(msg);
	}
}
void cly_d::_ClearMessageFun(cl_Message* msg)
{
	if(!msg)
	{
		assert(0);
		return;
	}
	switch(msg->cmd)
	{
	case CLYCMD_LOAD_DDLIST:
	case CLYCMD_REFER_FILE:
	case CLYCMD_RELEASE_FILE:
		{
			delete (string*)msg->data;
		}
		break;
	default:
		break;
	}
	delete msg;
}
void cly_d::update_used(int used)
{
	if (CLY_IS_USED(used, CLY_USED_ALL))
		used = 0xffffffff;

	cly_pc->used = used;
	if (!CLY_IS_USED(cly_pc->used, CLY_USED_DOWNLOAD))
	{
		cly_downAutoSngl::instance()->Stop("all");
		m_unused_down_times++;
	}
	if (!CLY_IS_USED(cly_pc->used, CLY_USED_SHARE))
	{
		cly_serverSngl::instance()->stop_all();
	}
}

int cly_d::create_download(const string& hash,int ftype,const string& path)
{
	//
	string filepath = path;
	if(filepath.empty())
	{
		clyt_fileinfo_t f;
		if(0==cly_cTrackerSngl::instance()->_get_fileinfo(hash,f))
			filepath = f.name;
		if(filepath.empty())
			return CLY_TERR_WRONG_PARAM; //没有名字不下载
	}
	//filepath = hash + ".mkv";
	clycmd_fileinfo_t inf;
	inf.path = filepath;
	inf.hash = hash;
	inf.ftype = ftype;
	this->m_msgQueue.SendMessage(new cl_Message(CLYCMD_CREATE_DOWNLOAD,&inf));
	return inf.ret;
}
void cly_d::load_ddlist(const string& path)
{
	this->m_msgQueue.AddMessage(new cl_Message(CLYCMD_LOAD_DDLIST,new string(path)));
}
void cly_d::refer_file(const string& hash)
{
	this->m_msgQueue.AddMessage(new cl_Message(CLYCMD_REFER_FILE,new string(hash)));
}
int cly_d::delete_file(const string& hash,bool isDelPhy)
{
	clycmd_fileinfo_t inf;
	inf.hash = hash;
	inf.ftype = isDelPhy?1:0;
	this->m_msgQueue.SendMessage(new cl_Message(CLYCMD_DELETE_FILE,&inf));
	return inf.ret;
}
void cly_d::release_file(const string& hash)
{
	this->m_msgQueue.AddMessage(new cl_Message(CLYCMD_RELEASE_FILE,new string(hash)));
}
int cly_d::add_source(const string& hash,list<string>& ls)
{
	clycmd_fileinfo_t inf;
	inf.hash = hash;
	inf.p = &ls;
	this->m_msgQueue.SendMessage(new cl_Message(CLYCMD_ADD_SOURCE,&inf));
	return inf.ret;
}
int cly_d::check_exist_share_by_path(const string& path)
{
	clycmd_fileinfo_t inf;
	inf.path = path;
	this->m_msgQueue.SendMessage(new cl_Message(CLYCMD_CHECK_EXIST_SHARE_BY_PATH,&inf));
	return inf.ret;
}
int cly_d::share_file(const string& hash,const string& subhash,const string& path,const string& name, bool report)
{
	clycmd_fileinfo_t inf;
	inf.path = path;
	inf.name = name;
	inf.hash = hash;
	inf.subhash = subhash;
	inf.ftype = report ? 1 : 0;
	this->m_msgQueue.SendMessage(new cl_Message(CLYCMD_SHARE_FILE,&inf));
	return inf.ret;
}
int cly_d::save_readyfile_utf8(const string& path)
{
	clycmd_fileinfo_t inf;
	inf.path = path;
	this->m_msgQueue.SendMessage(new cl_Message(CLYCMD_SAVE_READYFILE_UTF8,&inf));
	return inf.ret;
}
int cly_d::update_progress()
{
	this->m_msgQueue.AddMessage(new cl_Message(CLYCMD_UPDATE_PROGRESS,NULL));
	return 0;
}
int cly_d::get_state(clyd_state_t& s)
{
	clycmd_fileinfo_t inf;
	inf.p = &s;
	this->m_msgQueue.SendMessage(new cl_Message(CLYCMD_GET_STATE,&inf));
	return inf.ret;
}
int cly_d::get_fileinfo(const string& hash,cly_downloadInfo_t& di)
{
	clycmd_fileinfo_t inf;
	inf.hash = hash;
	inf.p = &di;
	this->m_msgQueue.SendMessage(new cl_Message(CLYCMD_GET_FILEINFO,&inf));
	return inf.ret;
}
int cly_d::get_readyinfo_by_path(const string& path,cly_readyInfo_t& ri)
{
	clycmd_fileinfo_t inf;
	inf.path = path;
	inf.p = &ri;
	this->m_msgQueue.SendMessage(new cl_Message(CLYCMD_GET_READYINFO_BY_PATH,&inf));
	return inf.ret;
}

//************************************* downauto ***************************
int cly_d::downauto_add(const string& hash,const string& name,int priority,int save_original)
{
	string filepath = name;
	if(filepath.empty())
	{
		clyt_fileinfo_t f;
		if(0==cly_cTrackerSngl::instance()->_get_fileinfo(hash,f))
			filepath = f.name;
		if(filepath.empty())
			return CLY_TERR_WRONG_PARAM; //没有名字不下载
	}

	clycmd_fileinfo_t inf;
	inf.path = filepath;
	inf.hash = hash;
	inf.ftype = FTYPE_DOWNLOAD;
	inf.priority = priority;
	inf.save_original = save_original;
	this->m_msgQueue.SendMessage(new cl_Message(CLYCMD_DOWNAUTO_ADD,&inf));
	return inf.ret;
}
int cly_d::downauto_allinfo(DownAutoInfoList& ls)
{
	this->m_msgQueue.SendMessage(new cl_Message(CLYCMD_DOWNAUTO_ALLINFO,&ls));
	return 0;
}
int cly_d::downauto_start(const string& hash)
{
	clycmd_fileinfo_t inf;
	inf.hash = hash;
	this->m_msgQueue.SendMessage(new cl_Message(CLYCMD_DOWNAUTO_START,&inf));
	return inf.ret;
}
int cly_d::downauto_stop(const string& hash)
{
	clycmd_fileinfo_t inf;
	inf.hash = hash;
	this->m_msgQueue.SendMessage(new cl_Message(CLYCMD_DOWNAUTO_STOP,&inf));
	return inf.ret;
}
int cly_d::update_login_config(cly_seg_config_t *conf)
{
	this->m_msgQueue.SendMessage(new cl_Message(CLYCMD_LOGIN_CONFIG,conf));
	return 0;
}

int cly_d::update_keepalive_ack(cly_seg_keepalive_ack_t* seg)
{
	this->m_msgQueue.SendMessage(new cl_Message(CLYCMD_KEEPALIVE_ACK, seg));
	return 0;
}

