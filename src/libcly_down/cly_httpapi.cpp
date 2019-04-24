#include "cly_httpapi.h"
#include "cl_net.h"
#include "cl_util.h"
#include "cly_config.h"
#include "cly_localCmd.h"
#include "cl_unicode.h"

#include "cly_cTracker.h"
#include "cly_downAuto.h"
#include "cly_jni.h"
#include "cl_httpc.h"

cly_httpapi::cly_httpapi(void)
	:m_readnum(0)
{
}


cly_httpapi::~cly_httpapi(void)
{
}
int cly_httpapi::open()
{
	if(m_svr.is_open())
		return 1;

	bool keepalive = false; //如果使用很容易成死连接占满线程
	if(0!=m_svr.open(cly_pc->http_port,NULL,on_request,this,true, cly_pc->http_threadnum,CLY_DOWN_VERSION, keepalive))
	{
		close();
		return -1;
	}

	return 0;
}
void cly_httpapi::close()
{
	m_svr.stop();
}


int cly_httpapi::on_request(cl_HttpRequest_t* req)
{
	printf("# http_req: %s \n",req->cgi);
	cly_httpapi* ph = (cly_httpapi*)req->fun_param;

#define EIF(func) else if(strstr(req->cgi,"/clyun/"#func".php")) ph->func(req)

	if(0==strcmp(req->cgi,"/") || strstr(req->cgi,"/index."))
		ph->index(req);
	EIF(get_state);
	EIF(get_setting);
	EIF(set_speed);

	EIF(share_file);
	EIF(delete_file);
	EIF(get_fileinfo);
	EIF(get_allfileinfo);
	EIF(read_rdbfile);
	EIF(get_readnum);

	EIF(update_ddlist);
	EIF(update_scan);
	EIF(update_progress);
	
	EIF(downauto_add);
	EIF(downauto_start);
	EIF(downauto_stop);
	EIF(downauto_allinfo);
	else
	{
		//目前只当他为文件
		string path = cly_pc->rootdir;
		path += (req->cgi+1);
		cl_httprsp::response_file(req->fd,path);
	}

	return 0;
}
void cly_httpapi::index(cl_HttpRequest_t* req)
{
	char buf[2048];
#define CAT_TITLE(s) sprintf(buf+strlen(buf),"<br><strong><font>%s</font></strong><br>",s)
#define CAT(cgi) sprintf(buf+strlen(buf),"<a target=\"_blank\" href=\"%s\">%s</a><br>",cgi,cgi)
	
	sprintf(buf,"<h3>HTTP API:</h3>");
	CAT_TITLE("system:");
	CAT("/version");
	CAT("/clyun/get_state.php");
	CAT("/clyun/get_setting.php");
	CAT("/clyun/set_speed.php?down=&up=&upi=");
	
	CAT_TITLE("file:");
	CAT("/clyun/share_file.php?path=&name=");
	CAT("/clyun/delete_file.php?hash=&delphy=");
	CAT("/clyun/get_fileinfo.php?hash=");
	CAT("/clyun/get_allfileinfo.php");
	CAT("/clyun/read_rdbfile.php?path=");
	CAT("/clyun/get_readnum.php");
	
	CAT_TITLE("realtime:");
	CAT("/clyun/update_ddlist.php");
	CAT("/clyun/update_scan.php");
	CAT("/clyun/update_progress.php");
	
	CAT_TITLE("downauto:");
	CAT("/clyun/downauto_add.php?hash=&name=&priority=");
	CAT("/clyun/downauto_start.php?hash=all");
	CAT("/clyun/downauto_stop.php?hash=all");
	CAT("/clyun/downauto_allinfo.php");
	cl_httprsp::response_message(req->fd,buf);
}
void cly_httpapi::rsp_error(int fd,const cly_seg_error_t& seg)
{
	char buf[1024];
	sprintf(buf,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<root>"
			"<seg id=\"error\">"
				"<p n=\"code\">%d</p>"
				"<p n=\"message\">%s</p>"
			"</seg>"
			"</root>",seg.code,seg.message.c_str());
	cl_httprsp::response_message(fd,buf,strlen(buf));
}
//**********************************************************************************
//系统状态
void cly_httpapi::get_state(cl_HttpRequest_t* req)
{
	cly_seg_error_t seg_error;
	clyd_state_t st;
	if(0!=clyd::instance()->get_state(st))
		seg_error.code = 1;
	else
		seg_error.code = 0;
	char *xmlbuf = new char[204800];
	cly_tp_xml_begin(xmlbuf);
	cly_tp_xml_add_seg_error(xmlbuf,seg_error);
	if(0==seg_error.code)
	{
		sprintf(xmlbuf+strlen(xmlbuf),
			"	<seg id=\"state\">\r\n"
			"		<p n=\"peer_name\">%s</p>\r\n"
			"		<p n=\"peer_id\">%d</p>\r\n"
			"		<p n=\"used\">0x%x</p>\r\n"
			"		<p n=\"login\">%d</p>\r\n"
			"		<p n=\"login_fails\">%d</p>\r\n"
			"		<p n=\"keepalive_fails\">%d</p>\r\n"
			"		<p n=\"downauto_pause\">%d</p>\r\n"
			"		<p n=\"unused_down_times\">%d</p>\r\n"
			"		<p n=\"ready_num\">%d</p>\r\n"
			"		<p n=\"freespace_GB\">%d</p>\r\n"
			"		<p n=\"downconn_num\">%d</p>\r\n"
			"		<p n=\"download_kB\">%d-%d-%d-%d kB(count-app-valid-all)</p>\r\n"
			"		<p n=\"upload_kB\">%d-%d-%d kB(valid-all-count)</p>\r\n"
			"		<p n=\"sendfaild_count\">%d(%d,%d,%d,%d,%d,%d,%d,%d)</p>\r\n"
			"	</seg>\r\n",
			st.peer_name.c_str(),st.peer_id,st.used,
			st.login_state,st.login_fails,st.keepalive_fails,st.downauto_pause,st.unused_down_times,
			st.ready_num,st.free_space_GB,st.downconn_num,
			(st.down_speed>>10),(st.app_recvspeedB>>10),(st.valid_recvspeedB>>10),(st.recvspeedB>>10),
			(st.valid_sendspeedB>>10),(st.sendspeedB>>10),(st.up_speed>>10),
			st.sendfaild_count,st.sendfaild_errs[0], st.sendfaild_errs[1], st.sendfaild_errs[2], st.sendfaild_errs[3],
			st.sendfaild_errs[4], st.sendfaild_errs[5], st.sendfaild_errs[6], st.sendfaild_errs[7]
		);
		cly_tp_rspxml_add_seg_list(xmlbuf,"down",st.downi_speeds);
		cly_tp_rspxml_add_seg_list(xmlbuf,"up",st.upi_speeds);
	}
	
	cly_tp_xml_end(xmlbuf);
	cl_httprsp::response_message(req->fd,xmlbuf,strlen(xmlbuf));
	delete[] xmlbuf;
}
void cly_httpapi::get_setting(cl_HttpRequest_t* req)
{
	//用于查阅
	char buf[2048]={0,};
	cly_config_sprint(cly_pc,buf,2048);
	cl_httprsp::response_message(req->fd,buf,strlen(buf));
}
void cly_httpapi::set_speed(cl_HttpRequest_t* req)
{
	//
	string params;
	cly_seg_error_t seg_error;
	int shareiKB,shareKB,downKB;

	params = req->params;
	shareiKB = cl_util::atoi(cl_util::url_get_parameter(params,"upi").c_str(),-1);
	shareKB = cl_util::atoi(cl_util::url_get_parameter(params,"up").c_str(), -1);
	downKB = cl_util::atoi(cl_util::url_get_parameter(params,"down").c_str(), -1);
	seg_error.code = CLYSET->update_speed(shareiKB,shareKB,downKB,false);
	rsp_error(req->fd,seg_error);
}
//**********************************************************************************
//文件
void cly_httpapi::share_file(cl_HttpRequest_t* req)
{
	string params;
	cly_seg_error_t seg_error;
	string path,name,hash;

	params = req->params;
	path = cl_util::url_get_parameter(params,"path");
	name = cl_util::url_get_parameter(params,"name");
	if(0==cly_pc->lang_utf8)
	{
		cl_unicode::UTF_8ToGB2312(path,path.c_str(),path.length());
		cl_unicode::UTF_8ToGB2312(name,name.c_str(),name.length());
	}
	if(path.empty())
		seg_error.code = CLY_TERR_WRONG_PARAM;
	else
	{
		if(!cl_util::file_exist(path))
			seg_error.code = CLY_TERR_NO_FILE;
		else
			seg_error.code = cly_localCmdSngl::instance()->share_file(path,name,true,hash,true);
	}
	
	char xmlbuf[1024];
	cly_tp_xml_begin(xmlbuf);
	cly_tp_xml_add_seg_error(xmlbuf,seg_error);
	sprintf(xmlbuf+strlen(xmlbuf),
			"	<seg id=\"result\">\r\n"
			"		<p n=\"hash\">%s</p>\r\n"
			"	</seg>\r\n",hash.c_str());
	cly_tp_xml_end(xmlbuf);
	cl_httprsp::response_message(req->fd,xmlbuf,strlen(xmlbuf));
}

void cly_httpapi::delete_file(cl_HttpRequest_t* req)
{
	cly_seg_error_t seg_error;
	string hash = cl_util::url_get_parameter(req->params,"hash");
	int delphy = cl_util::atoi(cl_util::url_get_parameter(req->params,"delphy").c_str());
	seg_error.code = clyd::instance()->delete_file(hash,delphy==1?true:false);
	rsp_error(req->fd,seg_error);
}
void cly_httpapi::get_fileinfo(cl_HttpRequest_t* req)
{
	string params;
	cly_seg_error_t seg_error;
	string hash;
	cly_downloadInfo_t di;

	params = req->params;
	hash = cl_util::url_get_parameter(params,"hash");
	seg_error.code = 0;
	if(hash.empty() || 0!=clyd::instance()->get_fileinfo(hash,di))
		seg_error.code = 1;
	char xmlbuf[1024];
	cly_tp_xml_begin(xmlbuf);
	cly_tp_xml_add_seg_error(xmlbuf,seg_error);
	if(0==seg_error.code)
	{
		sprintf(xmlbuf+strlen(xmlbuf),
			"	<seg id=\"downloadinfoi\">\r\n"
			"		<p n=\"hash\">%s</p>\r\n"
			"		<p n=\"ftype\">%d</p>\r\n"
			"		<p n=\"path\">%s</p>\r\n"
			"		<p n=\"size\">%lld</p>\r\n"
			"		<p n=\"progress\">%d</p>\r\n"
			"		<p n=\"srcNum\">%d</p>\r\n"
			"		<p n=\"connNum\">%d</p>\r\n"
			"		<p n=\"speed_kB\">%d</p>\r\n"
			"		<p n=\"state\">%d</p>\r\n"
			"	</seg>\r\n",
			di.hash.c_str(),di.ftype,di.path.c_str(),
			di.size,di.progress,di.srcNum,di.connNum,(di.speedB>>10),di.state);
	}
	
	cly_tp_xml_end(xmlbuf);
	cl_httprsp::response_message(req->fd,xmlbuf,strlen(xmlbuf));
}
void cly_httpapi::get_allfileinfo(cl_HttpRequest_t* req)
{
	string path = CLY_DIR_INFO + "readyinfo.dat";
	if(cl_util::file_exist(path))
		cl_httprsp::response_file(req->fd, path);
	else
		cl_httprsp::response_message(req->fd, " ");
}
void cly_httpapi::read_rdbfile(cl_HttpRequest_t* req)
{
	if (!CLY_IS_USED(cly_pc->used, CLY_USED_read_rdbfile))
	{ 
		cl_httprsp::response_message(req->fd, "unused!");
		return;
	}
	m_mt.lock();
	m_readnum++;
	m_mt.unlock();

	DWORD tick = GetTickCount();
	jni_fire_rdbread_begin();
	string path = cl_util::url_get_parameter(req->params,"path");
	cl_httprsp::response_rdbfile(req,path);

	tick = (GetTickCount() - tick)/1000;
	if (tick > 30)
	{
		//上报
		string sn = cly_pc->peer_name;
		string hash;
		cly_readyInfo_t inf;
		char url[2048];
		char buf[1024];
		if (0 == clyd::instance()->get_readyinfo_by_path(path, inf))
			hash = inf.hash;
		sprintf(url, "http://ars.imovie.com.cn/api/devicePlay/report?sn=%s&filePath=%s&hash=%s", sn.c_str(), path.c_str(), hash.c_str());
		
		int ret = 0;
		int i = 0;
		while (0 != (ret = cl_httpc::http_get(url, buf, 1024)))
		{
			if (++i > 2)
				break;
			Sleep(1000);
		}
		DEBUGMSG("read_rdbfile sec=%d report(%s)=%d\n", (int)tick,url, ret);
	}
	else
	{
		DEBUGMSG("read_rdbfile un report sec=%d !!!\n", (int)tick);
	}

	m_mt.lock();
	m_readnum--;
	m_mt.unlock();
}
void cly_httpapi::get_readnum(cl_HttpRequest_t* req)
{
	char buf[1024];
	sprintf(buf,"%d",m_readnum);
	cl_httprsp::response_message(req->fd,buf);
}
//**********************************************************************************
//即时更新
void cly_httpapi::update_ddlist(cl_HttpRequest_t* req)
{
	cly_seg_error_t seg_error;
	seg_error.code = 0;
	if(cly_pc->timer_get_ddlistS>0)
		cly_cTrackerSngl::instance()->get_ddlist(cly_downAutoSngl::instance()->get_updateid());
	else
		seg_error.code = 1;

	rsp_error(req->fd,seg_error);
}

void cly_httpapi::update_scan(cl_HttpRequest_t* req)
{
	cly_seg_error_t seg_error;
	cly_localCmdSngl::instance()->scanf();
	seg_error.code = 0;
	rsp_error(req->fd,seg_error);
}
void cly_httpapi::update_progress(cl_HttpRequest_t* req)
{
	cly_seg_error_t seg_error;
	clyd::instance()->update_progress();
	seg_error.code = 0;
	rsp_error(req->fd,seg_error);
}

//**********************************************************************************
//downauto
void cly_httpapi::downauto_add(cl_HttpRequest_t* req)
{
	string params = req->params;
	cly_seg_error_t seg_error;
	string hash,name;
	hash = cl_util::url_get_parameter(params,"hash");
	name = cl_util::url_get_parameter(params,"name");
	int priority = cl_util::atoi(cl_util::url_get_parameter(params,"priority").c_str(),50);
	int save_original = cl_util::atoi(cl_util::url_get_parameter(params, "save_original").c_str(), 0);
	seg_error.code = clyd::instance()->downauto_add(hash,name,priority, save_original);
	rsp_error(req->fd,seg_error);
}

void cly_httpapi::downauto_start(cl_HttpRequest_t* req)
{
	cly_seg_error_t seg_error;
	string hash = cl_util::url_get_parameter(req->params,"hash");
	seg_error.code = clyd::instance()->downauto_start(hash);
	rsp_error(req->fd,seg_error);
}
void cly_httpapi::downauto_stop(cl_HttpRequest_t* req)
{
	cly_seg_error_t seg_error;
	string hash = cl_util::url_get_parameter(req->params,"hash");
	seg_error.code = clyd::instance()->downauto_stop(hash);
	rsp_error(req->fd,seg_error);
}
void cly_httpapi::downauto_allinfo(cl_HttpRequest_t* req)
{
	//
	char *buf = new char[102400];
	DownAutoInfoList ls;
	DownAutoInfoIter it;
	cly_downAutoInfo *inf;
	int n=0;
	clyd::instance()->downauto_allinfo(ls);
	sprintf(buf, "%d\r\n"
		"hash|size|name|path|priority|createtime|faileds|progress|speed|state|ftype|save_original\r\n"
		, ls.size());
	for (it = ls.begin(); it != ls.end(); ++it,++n)
	{
		inf = *it;
		if (n < 100)
		{
			sprintf(buf + strlen(buf), "%s|%lld|%s|%s|%d|%s|%d|%d|%d|%d|%d|%d\r\n",
				inf->hash.c_str(), inf->size, inf->name.c_str(), inf->path.c_str(), inf->priority, inf->createtime.c_str(), inf->faileds, inf->progress, inf->speed, inf->state, inf->ftype,inf->save_original);
		}
		delete inf;
	}

	cl_httprsp::response_message(req->fd, buf);

	delete[] buf;
}


