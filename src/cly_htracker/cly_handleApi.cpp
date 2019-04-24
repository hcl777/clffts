#include "cly_handleApi.h"
#include "cly_cTrackerInfo.h"
#include "cl_util.h"
#include "cly_setting.h"
#include "cl_urlcode.h"
#include "cl_httpc.h"

enum CLY_API_CMD{
	EAPI_END=0,
	EAPI_REPORT_FINIFILE
};

//*****************************************************
cly_handleApi::cly_handleApi(void)
	:m_brun(false)
{
}


cly_handleApi::~cly_handleApi(void)
{
}
void cly_handleApi::_ClearMessageFun(cl_Message* msg)
{
	delete msg;
}
int cly_handleApi::run()
{
	if(m_brun) return 1;
	m_brun = true;

	//load report hids
	string str;
	string& hids = cly_settingSngl::instance()->get_api_report_finifile_hids();
	int n = cl_util::get_string_index_count(hids,",");
	for(int i=0;i<n;++i)
	{
		str = cl_util::get_string_index(hids,i,",");
		cl_util::string_trim(str);
		if(!str.empty())
			m_report_fini_hids_map[str] = 0;
	}

	cl_util::get_stringlist_from_file(cly_settingSngl::instance()->get_datadir() + "~report_finifile.txt",m_finilist);

	activate();
	
	//5分钟检查上报
	cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),1,300000);
	return 0;
}
void cly_handleApi::end()
{
	if(!m_brun) return;
	m_brun = false;
	
	cl_timerSngl::instance()->unregister_all(static_cast<cl_timerHandler*>(this));
	m_queue.AddMessage(new cl_Message(EAPI_END,NULL));
	wait();

	m_queue.ClearMessage(_ClearMessageFun);
}
int cly_handleApi::work(int e)
{
	Sleep(1000);
	_report_finifile();
	cl_Message *msg;
	while(m_brun)
	{
		msg = m_queue.GetMessage();
		if(NULL==msg)
			continue;
		switch(msg->cmd)
		{
		case EAPI_REPORT_FINIFILE:
			_report_finifile();
			break;
		default:
			break;
		}
		_ClearMessageFun(msg);
	}
	return 0;
}
void cly_handleApi::on_timer(int e)
{
	switch(e)
	{
	case 1:
		{
			if(!m_finilist.empty())
				m_queue.AddMessage(new cl_Message(EAPI_REPORT_FINIFILE,NULL));
		}
		break;
	default:
		break;
	}

}

void cly_handleApi::report_finifile(const string& hid,const string& hash,const string& name)
{
	if(!m_report_fini_hids_map.empty() && m_report_fini_hids_map.find(hid)==m_report_fini_hids_map.end())
		return;
	{
		Lock l(m_mt);
		char buf[1024];
		sprintf(buf,"%s|%s|%s",hid.c_str(),hash.c_str(),name.c_str());
		m_finilist.push_back(buf);
		cl_util::put_stringlist_to_file(cly_settingSngl::instance()->get_datadir() + "~report_finifile.txt",m_finilist);
	}
	m_queue.AddMessage(new cl_Message(EAPI_REPORT_FINIFILE,NULL));
}

void cly_handleApi::_report_finifile()
{
	string str;
	bool b = false;
	int ret = 0;	
	char url[2048];	
	cly_seg_error_t seg_err;	
	cl_xml xml;
	string hid,hash,name;

	string& url_pre = cly_settingSngl::instance()->get_api_report_finifile();
	if(url_pre.empty())
		return;
	while(!m_finilist.empty())
	{
		{
			Lock l(m_mt);
			str = m_finilist.front();
		}
		hid = cl_util::get_string_index(str,0,"|");
		hash = cl_util::get_string_index(str,1,"|");
		name = cl_util::get_string_index(str,2,"|");

		//注意这里不带？号，
		sprintf(url,"%shid=%s&hash=%s&name=%s",url_pre.c_str(),
			cl_urlencode(hid).c_str(),cl_urlencode(hash).c_str(),cl_urlencode(name).c_str());
		ret = cly_tracker_http_get(url,xml,seg_err,1024);
		if(0==ret && 0==seg_err.code)
		{
			b = true;
			DEBUGMSG("#-- report_finifile (%s) ok\n",url);
			Lock l(m_mt);
			m_finilist.pop_front();
		}
		else
		{
			DEBUGMSG("#*** report_finifile (%s) fail\n",url);
			break;
		}
	}
	if(b)
	{
		Lock l(m_mt);
		cl_util::put_stringlist_to_file(cly_settingSngl::instance()->get_datadir() + "~report_finifile.txt",m_finilist);
	}
}
int cly_handleApi::get_group_id(const string& peer_name)
{
	//不使用异步获取
	//返回0 表示未获得，
	char buf[4096];
	char url[1024];
	char val[1024];
	int code;
	cl_xml xml;
	if(peer_name.empty())
		return 0;
	string& url_pre = cly_settingSngl::instance()->get_api_get_group_id();
	if(url_pre.empty())
		return 0;
	sprintf(url,"%ssn=%s",url_pre.c_str(),peer_name.c_str());
	if(0==cl_httpc::http_get(url,buf,4095))
	{
		if(0==xml.load_string(buf))
		{
			code = cl_util::atoi(xml.find_first_node_data(NULL,"response/code","-1",val,1024));
			if(0==code)
				return cl_util::atoi(xml.find_first_node_data(NULL,"response/group_id","0",val,1024));
		}
	}
	return 0;
}

