#include "cly_cTracker.h"
#include "cl_util.h"
#include "cl_net.h"
#include "cl_httpc.h"
#include "cl_httpc2.h"
#include "cl_urlcode.h"
#include "cl_unicode.h"

extern int g_cly_lang_utf8;

cly_cTracker::cly_cTracker(void)
	:m_login_fails(0)
	, m_keepalive_fails(0)
	,m_brun(false)
	,m_blogin(false)
	,m_bchecksum(false)
{
	m_seg_fchsum.ff_num = 0;
	m_seg_fchsum.ff_checksum = 0;
}


cly_cTracker::~cly_cTracker(void)
{
}

void cly_cTracker::_ClearMessageFun(cl_Message* msg)
{
	if(!msg)
	{
		assert(0);
		return;
	}
	switch(msg->cmd)
	{
	case CLY_CT_SEARCH_SOURCE:
	case CLY_CT_REPORT_NATTYPE:
	case CLY_CT_PUT_FINIFILES:
	case CLY_CT_GET_DDLIST:
		delete (string*)msg->data;
		break;
	case CLY_CT_REPORT_ERROR:
		delete (cly_report_error_t*)msg->data;
	default:
		break;
	}
	delete msg;
}

void cly_cTracker::set_checksum(cly_seg_fchsum_t* pseg)
{
	m_seg_fchsum.ff_num = pseg->ff_num;
	m_seg_fchsum.ff_checksum = pseg->ff_checksum;
}
int cly_cTracker::init(const cTrackerConf_t& conf)
{
	if(m_brun) return 1;
	m_brun = true;
	m_conf = conf;
	m_conf.peer_id = 0;
	g_cly_lang_utf8 = m_conf.lang_utf8;
	cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),1,120000);
	//5分钟检查上报
	cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),2,300000);

	cl_util::get_stringlist_from_file(m_conf.workdir + "~report_fdfile.txt",m_fdlist);
	if(!m_fdlist.empty())
		m_msgQueue.AddMessage(new cl_Message(CLY_CT_REPORT_FDFILE,NULL));
	
	return 0;
}
void cly_cTracker::run()
{
	activate();
}
void cly_cTracker::end()
{
	if(!m_brun) return;
	m_brun = false;
	cl_timerSngl::instance()->unregister_all(static_cast<cl_timerHandler*>(this));
	wait();
	m_msgQueue.ClearMessage(_ClearMessageFun);
	m_blogin = false;
}
int cly_cTracker::work(int e)
{

	cl_Message* msg = NULL;
	_login();
	Sleep(500);
	while(m_brun)
	{
		msg=m_msgQueue.GetMessage(0);
		if(!msg)
		{
			Sleep(100);
			continue;
		}
		switch(msg->cmd)
		{
		case CLY_CT_LOGIN:
			_login();
			break;
		case CLY_CT_KEEPALIVE:
			_keepalive();
			break;
		case CLY_CT_SEARCH_SOURCE:
			_search_source(*(string*)msg->data);
			break;
		case CLY_CT_REPORT_ERROR:
			_report_error((cly_report_error_t*)msg->data);
			break;
		case CLY_CT_REPORT_NATTYPE:
			_update_nat(m_conf.uacaddr);
			break;
		case CLY_CT_REPORT_PROGRESS:
			 _report_progress(m_pgls);
			break;
		case CLY_CT_REPORT_FDFILE:
			_report_fdfile();
			break;
		case CLY_CT_GET_DDLIST:
			_get_ddlist(*(string*)msg->data);
			break;
		case CLY_CT_GET_FINIFILES:
			_get_finifiles();
			break;
		case CLY_CT_PUT_FINIFILES:
			_put_finifiles(*(string*)msg->data);
			break;
		default:
			assert(false);
			break;
		}
		_ClearMessageFun(msg);
	}
	return 0;
}
void cly_cTracker::on_timer(int e)
{
	switch(e)
	{
	case 1:
		{
			if(!m_blogin)
				m_msgQueue.AddMessage(new cl_Message(CLY_CT_LOGIN,NULL));
			else
				m_msgQueue.AddMessage(new cl_Message(CLY_CT_KEEPALIVE,NULL));
		}
		break;
	case 2:
		{
			if(!m_fdlist.empty())
				m_msgQueue.AddMessage(new cl_Message(CLY_CT_REPORT_FDFILE,NULL));
		}
		break;
	default:
		break;
	}

}
#define CLY_CT_HTTPREQ_BASE_DEFINE \
	int ret = 0;	\
	char url[2048];	\
	cly_seg_error_t seg_err;	\
	cl_xml xml;

void cly_cTracker::_login()
{
	//获取配置
	CLY_CT_HTTPREQ_BASE_DEFINE
	cly_seg_config_t seg_config;

	sprintf(url,"http://%s%s/login.php?peer_name=%s&ver=%s&addr=%s",
		m_conf.tracker.c_str(),CLY_CGI_,
		m_conf.peer_name.c_str(),m_conf.app_version.c_str(),m_conf.uacaddr.c_str());
	//DEBUGMSG("#login ( %s )... \n",url);

	assert(!m_blogin);
	ret = cly_tracker_http_get(url,xml,seg_err,1024);
	if(0==ret)
	{
		if(0 == seg_err.code)
			m_blogin = true;
		if(0==cly_xml_get_seg_config(xml,seg_config))
		{
			DWORD timeo = seg_config.timer_keepaliveS * 1000;
			if(timeo>=1000)
			{
				cl_timerSngl::instance()->unregister_timer(static_cast<cl_timerHandler*>(this),1);
				cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),1,timeo);
			}
			m_conf.peer_id = seg_config.peer_id;
			m_conf.group_id = seg_config.group_id;
			if(m_conf.func)
				m_conf.func(CLY_CTRACKER_MSG_CONF,&seg_config,NULL);
		}
	}
	if (m_blogin)
	{
		DEBUGMSG("$login = ok( pid=%d, group_id=%d )\n", m_conf.peer_id, m_conf.group_id);
		if (!m_bchecksum && m_fdlist.empty())
		{
			_check_fini_chsum();
		}
	}else
	{
		m_login_fails++;
		DEBUGMSG("$login = failed \n");
	}
}
void cly_cTracker::_keepalive()
{

	cly_seg_keepalive_ack_t seg;
	if(!m_blogin)
		return;
	//keepalive ,上报一些状态，如共享连接数
	CLY_CT_HTTPREQ_BASE_DEFINE
	sprintf(url,"http://%s%s/keepalive.php?peer_id=%d&peer_name=%s&down_state=%s&up_state=%s",
		m_conf.tracker.c_str(),CLY_CGI_,m_conf.peer_id,m_conf.peer_name.c_str()
		,m_info.down_state.c_str(),m_info.up_state.c_str());
	ret = cly_tracker_http_get(url,xml,seg_err,1024);
	if(0==ret)
	{
		DEBUGMSG("# keepalive =>(%d,%s) !\n",seg_err.code,seg_err.message.c_str());
		if(CLY_TERR_SUCCEED !=seg_err.code)
		{
			m_blogin = false;
			m_conf.peer_id = 0;
			m_keepalive_fails++;
			DEBUGMSG("# *** keepalive server logout(code=%d) ! \n", seg_err.code);
		}
		if(m_blogin && 0 == cly_xml_get_seg_keepalive_ack(xml, seg))
		{
			if (m_conf.func)
				m_conf.func(CLY_CTRACKER_MSG_KEEPALIVE_ACK, &seg, NULL);
		}
	}
	else
		DEBUGMSG("# *** keepalive http_get()=%d fail! \n",ret);
}
void cly_cTracker::_update_nat(const string& uacaddr)
{
	if(!m_blogin)
		return;
	DEBUGMSG("#cly_cTracker::_update_nat(%s) \n",uacaddr.c_str());
	CLY_CT_HTTPREQ_BASE_DEFINE
	sprintf(url,"http://%s%s/update_nat.php?peer_id=%d&addr=%s",
		m_conf.tracker.c_str(),CLY_CGI_,m_conf.peer_id,
		uacaddr.c_str());
	ret = cly_tracker_http_get(url,xml,seg_err,1024);
	if(0==ret)
		DEBUGMSG("# update_nat =>(%d,%s) !\n",seg_err.code,seg_err.message.c_str());
	else
		DEBUGMSG("# *** update_nat http_get()=%d fail! \n",ret);
}
void cly_cTracker::_search_source(string& hash)
{
	if(!m_blogin)
		return;
	list<string> ls;
	DEBUGMSG("# search_source(%s) \n",hash.c_str());
	CLY_CT_HTTPREQ_BASE_DEFINE
	sprintf(url,"http://%s%s/search_source.php?peer_id=%d&hash=%s",
		m_conf.tracker.c_str(),CLY_CGI_,m_conf.peer_id,
		hash.c_str());
	ret = cly_tracker_http_get(url,xml,seg_err,10240);
	if(0==ret && 0==seg_err.code)
	{
		cly_xml_get_list(xml,ls,"sources");
		if(!ls.empty() && m_conf.func)
		{
			m_conf.func(CLY_CTRACKER_MSG_SOURCE,(void*)&hash,&ls);
		}
	}
	////test:
	//ls.clear();
	//ls.push_back("127.0.0.1:9200:1:2");
	//m_conf.func(CLY_CTRACKER_MSG_SOURCE,(void*)&hash,&ls);
}

void cly_cTracker::_report_progress(list<string>& ls)
{
	if(!m_blogin)
		return;
	char *body = new char[4096];
	char buf[1024];
	string str;
	sprintf(body,"xml=<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<root>"
			"<seg id=\"client\">"
				"<p n=\"peer_id\">%d</p>"
			"</seg>"
			"<list id=\"progress\" fields=\"%s\">",m_conf.peer_id,m_conf.progress_fields.c_str());
	for(list<string>::iterator it=ls.begin();it!=ls.end();++it)
	{
		str = *it;
		if(0==m_conf.lang_utf8)
			cl_unicode::GB2312ToUTF_8(str,str.c_str(),str.length());
		sprintf(buf,"<p>%s</p>",str.c_str());
		strcat(body,buf);
	}
	strcat(body,"</list></root>");

	CLY_CT_HTTPREQ_BASE_DEFINE
	sprintf(url,"http://%s%s/report_progress.php",m_conf.tracker.c_str(),CLY_CGI_);

	//body用xml=urlcode(xmlbody)的方式
	ret = cly_tracker_http_post(url,body,(int)strlen(body),xml,seg_err);
	if(0==ret && 0==seg_err.code)
		DEBUGMSG("# report_progress =>(%d,%s) !\n",seg_err.code,seg_err.message.c_str());
	else
		DEBUGMSG("# *** report_progress http_get()=%d fail! \n",ret);
	delete[] body;
}
void cly_cTracker::_report_fdfile()
{
	if(!m_blogin)
		return;
	string str;
	bool b = false;
	int ret = 0;	
	char url[2048];	
	cly_seg_error_t seg_err;	
	cl_xml xml;

	while(!m_fdlist.empty())
	{
		{
			Lock l(m_mt);
			str = m_fdlist.front();
		}
		
		if(0==m_conf.lang_utf8)
			cl_unicode::GB2312ToUTF_8(str,str.c_str(),str.length());
		sprintf(url,"http://%s%s/report_fdfile.php?peer_id=%d&fields=%s&val=%s",m_conf.tracker.c_str(),CLY_CGI_
			,m_conf.peer_id,cl_urlencode(m_conf.fdfile_fields).c_str(),cl_urlencode(str).c_str());
		ret = cly_tracker_http_get(url,xml,seg_err,1024);
		if(0==ret && 0==seg_err.code)
		{
			b = true;
			DEBUGMSG("#-- report_fdfile (%s)\n",str.c_str());
			Lock l(m_mt);
			m_fdlist.pop_front();
		}
		else
			break;
	}
	if(b)
	{
		Lock l(m_mt);
		cl_util::put_stringlist_to_file(m_conf.workdir + "~report_fdfile.txt",m_fdlist);
	}
	if(!m_bchecksum && m_fdlist.empty())
	{
		_check_fini_chsum();
	}
}

void cly_cTracker::_report_error(cly_report_error_t* errinf)
{
	CLY_CT_HTTPREQ_BASE_DEFINE
	//如果有peer_id，就改用peer_id易观看
	string peer_name = m_conf.peer_name;
	if(m_conf.peer_id>0)
		peer_name = cl_util::itoa(m_conf.peer_id);
	sprintf(url,"http://%s%s/report_error.php?peer_name=%s&err=%d&appname=%s&appver=%s&systemver=%s&description=%s",
		m_conf.tracker.c_str(),CLY_CGI_,peer_name.c_str(),errinf->err,
		errinf->appname.c_str(),errinf->appver.c_str(),cl_urlencode(errinf->systemver).c_str(),cl_urlencode(errinf->description).c_str());
	ret = cly_tracker_http_get(url,xml,seg_err,1024);
	if(0==ret)
		DEBUGMSG("# report_error =>(%d,%s) !\n",seg_err.code,seg_err.message.c_str());
	else
		DEBUGMSG("# *** report_error http_get()=%d fail! \n",ret);
}
void cly_cTracker::_get_ddlist(const string& taskid)
{
	if(!m_blogin)
		return;
	DEBUGMSG("#cly_cTracker::_get_ddlist(%s) \n",taskid.c_str());
	list<string> ls;
	CLY_CT_HTTPREQ_BASE_DEFINE
		sprintf(url,"http://%s%s/get_ddlist.php?peer_id=%d&taskid=%s", m_conf.tracker.c_str(),CLY_CGI_,m_conf.peer_id,taskid.c_str());
	ret = cly_tracker_http_get(url,xml,seg_err,102400);
	if(0==ret && 0==seg_err.code)
	{
		if(0==cly_xml_get_seg_ddlist(xml,ls))
		{
			m_conf.func(CLY_CTRACKER_MSG_DDLIST,&ls,NULL);
		}
	}
	//////fdtype|hash|priority|name
	////test
	//ls.clear();
	//ls.push_back("2");
	//ls.push_back("fdtype|hash|priority|name");
	//ls.push_back("1|7f4588f235ddbea3db0cad7d95396640|10|变形金刚.mkv");
	//ls.push_back("1|478738ab42b8b2a2b43fdb7d94f18b4d|10|");
	//ls.push_back("1|b9355088313d6ac334e6cf11e7e789e0|10|");
	//ls.push_back("1|95795a16f1c646d987c0cf74842e49b0|10|");
	//m_conf.func(CLY_CTRACKER_MSG_DDLIST,&ls,NULL);
}
void cly_cTracker::_check_fini_chsum()
{
	if(!m_blogin||m_bchecksum)
		return;

	CLY_CT_HTTPREQ_BASE_DEFINE
	sprintf(url,"http://%s%s/check_fini_chsum.php?peer_id=%d&ff_num=%d&ff_checksum=%lld",
	m_conf.tracker.c_str(),CLY_CGI_,m_conf.peer_id,
	m_seg_fchsum.ff_num,m_seg_fchsum.ff_checksum);
	ret = cly_tracker_http_get(url,xml,seg_err,1024);
	if(0==ret)
	{
		m_bchecksum = true;
		if(CLY_TERR_CHSUM_DISTINCT==seg_err.code)
		{
			DEBUGMSG("#*** check sum DISTINCT !!! \n");
			m_conf.func(CLY_CTRACKER_CHSUM_DISTINCT,NULL,NULL);
		}
		else
			DEBUGMSG("#--- check sum OK !!! \n");
	}
}
void cly_cTracker::_get_finifiles()
{
	if(!m_blogin)
		return;
	char url[1024];
	string path = m_conf.workdir + "~get_finifiles.txt";
	sprintf(url,"http://%s%s/get_finifiles.php?peer_id=%d",
		m_conf.tracker.c_str(),CLY_CGI_,m_conf.peer_id);
	if(0==cl_httpc::download_file(url,path))
	{
		if(m_conf.func)
		{
			m_conf.func(CLY_CTRACKER_MSG_GET_FINIFILES,&path,NULL);
		}
	}
}
void cly_cTracker::_put_finifiles(const string& path)
{
	if(!m_blogin)
		return;
	CLY_CT_HTTPREQ_BASE_DEFINE
	sprintf(url,"http://%s%s/put_finifiles.php?peer_id=%d",m_conf.tracker.c_str(),CLY_CGI_,m_conf.peer_id);
	ret = cly_tracker_http_post_file(url,path.c_str(),xml,seg_err);
	if(0==ret)
	{
		DEBUGMSG("# put_finifiles(err=%d,msg=%s)\n" ,seg_err.code,seg_err.message.c_str());
	}
	else
	{
		DEBUGMSG("# ***put_finifiles() http post file fail!\n" );
	}
}
//***********************************************************************
void cly_cTracker::update_nat(const string& uacaddr)
{
	m_conf.uacaddr = uacaddr;
	m_msgQueue.AddMessage(new cl_Message(CLY_CT_REPORT_NATTYPE,NULL));
}
void cly_cTracker::search_source(const string& hash)
{
	string *data = new string(hash);
	m_msgQueue.AddMessage(new cl_Message(CLY_CT_SEARCH_SOURCE,data));
}
void cly_cTracker::report_fdfile(const string& fdfile,cly_seg_fchsum_t* pseg)
{
	{
		Lock l(m_mt);
		m_fdlist.push_back(fdfile);
		cl_util::put_stringlist_to_file(m_conf.workdir + "~report_fdfile.txt",m_fdlist);
		if(pseg)
			set_checksum(pseg);
	}
	m_msgQueue.AddMessage(new cl_Message(CLY_CT_REPORT_FDFILE,NULL));
}
void cly_cTracker::report_progress(list<string>& ls)
{
	m_pgls = ls;
	m_msgQueue.AddMessage(new cl_Message(CLY_CT_REPORT_PROGRESS,NULL));
}

void cly_cTracker::report_error(const cly_report_error_t& errinf)
{
	cly_report_error_t *data = new cly_report_error_t();
	*data = errinf;
	m_msgQueue.AddMessage(new cl_Message(CLY_CT_REPORT_ERROR,data));
}
void cly_cTracker::get_ddlist(const string& taskid)
{
	m_msgQueue.AddMessage(new cl_Message(CLY_CT_GET_DDLIST,(void*)new string(taskid)));
}
void cly_cTracker::get_finifiles()
{
	m_msgQueue.AddMessage(new cl_Message(CLY_CT_GET_FINIFILES,NULL));
}

void cly_cTracker::put_finifiles(const string& path)
{
	//先删除掉所有上报完成或者删除任务，再上传文件
	{
		Lock l(m_mt);
		m_fdlist.clear();
		cl_util::file_delete(m_conf.workdir + "~report_fdfile.txt");
	}
	string *data = new string(path);
	m_msgQueue.AddMessage(new cl_Message(CLY_CT_PUT_FINIFILES,data));
}
int cly_cTracker::_update_filename(const string& hash,const string& name)
{
	CLY_CT_HTTPREQ_BASE_DEFINE
	string str = name;
	if(0==m_conf.lang_utf8)
		cl_unicode::GB2312ToUTF_8(str,str.c_str(),str.length());
	sprintf(url,"http://%s%s/update_filename.php?peer_name=%s&hash=%s&name=%s",
	m_conf.tracker.c_str(),CLY_CGI_,m_conf.peer_name.c_str(),hash.c_str(),str.c_str());
	ret = cly_tracker_http_get(url,xml,seg_err,1024);
	return ret;
}
int cly_cTracker::_get_fileinfo(const string& hash,clyt_fileinfo_t& fi)
{
	CLY_CT_HTTPREQ_BASE_DEFINE
	sprintf(url,"http://%s%s/get_fileinfo.php?hash=%s",
	m_conf.tracker.c_str(),CLY_CGI_,hash.c_str());
	ret = cly_tracker_http_get(url,xml,seg_err,1024);
	if(0==ret && 0==seg_err.code)
		cly_xml_get_seg_clyt_fileinfo(xml,fi);
	return 0==ret?seg_err.code:-1;
}
int cly_cTracker::_upload_task(const string& hids,const string& hash)
{
	char task_name[1024];
	list<string> ls_hids, ls_hash;
	sprintf(task_name, "upload_%s_%s", m_conf.peer_name.c_str(), cl_util::time_to_datetime_string2(time(NULL)).c_str());
	int n;
	string str;
	char xmlbuf[4096];
	sprintf(xmlbuf, "xml=<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<root>"
		"<p n=\"task_name\">%s</p>\r\n", task_name);

	n = cl_util::get_string_index_count(hids, "|");
	for (int i = 0; i < n; ++i)
	{
		str = cl_util::get_string_index(hids, i, "|");
		cl_util::string_trim(str);
		if (!str.empty())
			ls_hids.push_back(str);
	}
	ls_hash.push_back(hash);
	if (ls_hids.empty())
		return -1;

	cly_tp_rspxml_add_seg_list(xmlbuf, "hids", ls_hids);
	cly_tp_rspxml_add_seg_list(xmlbuf, "files", ls_hash);
	
	strcat(xmlbuf, "</root>");

	CLY_CT_HTTPREQ_BASE_DEFINE
		sprintf(url, "http://%s%s/new_task.php", m_conf.tracker.c_str(), CLY_CGI_);

	//body用xml=urlcode(xmlbody)的方式
	ret = cly_tracker_http_post(url, xmlbuf, (int)strlen(xmlbuf), xml, seg_err);
	if (0!=ret)
		return -1;
	return seg_err.code;
}

