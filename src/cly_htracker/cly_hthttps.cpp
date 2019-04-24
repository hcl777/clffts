#include "cly_hthttps.h"
#include "cl_net.h"
#include "cl_util.h"
#include "cly_usermgr.h"
#include "cly_setting.h"

cly_hthttps::cly_hthttps(void)
{
}

cly_hthttps::~cly_hthttps(void)
{
}
int cly_hthttps::open()
{
	if(m_svr.is_open())
		return 1;

	if(0!=m_svr.open(cly_settingSngl::instance()->get_http_port(),NULL,on_request,this,true,cly_settingSngl::instance()->get_http_num(),CLY_HTRACKER_VERSION,
		cly_settingSngl::instance()->get_http_support_keepalive()))
	{
		close();
		return -1;
	}
	m_svrbak.open(cly_settingSngl::instance()->get_http_port() + 1, NULL, on_request, this, true, 5, CLY_HTRACKER_VERSION,
		cly_settingSngl::instance()->get_http_support_keepalive());

	return 0;
}
void cly_hthttps::close()
{
	m_svr.stop();
	m_svrbak.stop();
}


int cly_hthttps::on_request(cl_HttpRequest_t* req)
{
	printf("# http_req: %s \n",req->cgi);
	cly_hthttps* ph = (cly_hthttps*)req->fun_param;
#define EIF(func) else if(strstr(req->cgi,"/clyun/"#func".php")) ph->func(req)

	if(0==strcmp(req->cgi,"/") || strstr(req->cgi,"/index."))
		ph->index(req);
	EIF(show_httpconf);
	EIF(login);
	EIF(keepalive);
	EIF(update_nat);
	EIF(search_source);
	EIF(search_sources);
	EIF(report_progress);
	EIF(report_fdfile);
	EIF(update_filename);
	EIF(get_fileinfo);
	EIF(report_error);

	EIF(new_task);				//外
	EIF(set_task_state);	//外
	EIF(get_task_info);		//外
	EIF(get_ddlist);

	EIF(check_fini_chsum);
	EIF(get_finifiles);
	EIF(put_finifiles);
	
	EIF(get_state);
	EIF(get_peer_state);	//外
	EIF(get_server);	//外
	else
	{
		//目前只当他为文件
		string path = cl_util::get_module_dir();
		path += (req->cgi+1);
		cl_httprsp::response_file(req->fd,path);
	}

	return 0;
}
void cly_hthttps::index(cl_HttpRequest_t* req)
{
	char *buf = new char[20480];
#define CAT_TITLE(s) sprintf(buf+strlen(buf),"<br><strong><font>%s</font></strong><br>",s)
#define CAT(cgi) sprintf(buf+strlen(buf),"<a target=\"_blank\" href=\"%s\">%s</a><br>",cgi,cgi)
	
	sprintf(buf,"<h3>HTTP API(cly_htracker):</h3>");
	CAT("/version");
	CAT("/clyun/show_httpconf.php");
	CAT_TITLE("base:");
	CAT("/clyun/login.php?peer_name=&ver=");
	CAT("/clyun/keepalive.php?peer_id=&peer_name=&down_state=&up_state=");
	CAT("/clyun/update_nat.php?peer_id=&addr=");
	CAT("/clyun/search_source.php?peer_id=&hash=");
	CAT("/clyun/search_sources.php?peer_id=&hash=");
	CAT("/clyun/report_progress.php?body"); //xml中包含peer_id
	CAT("/clyun/report_fdfile.php?peer_id=&fields=&val=");
	CAT("/clyun/update_filename.php?&hash=&name=");
	CAT("/clyun/get_fileinfo.php?&hash=");
	CAT("/clyun/report_error.php?peer_name=&err=&appname=&appver=&systemver=&description=");
	
	CAT_TITLE("task:");
	CAT("/clyun/new_task.php?body");
	CAT("/clyun/set_task_state.php?task_name=&state=");
	CAT("/clyun/get_task_info.php?task_name=");
	CAT("/clyun/get_ddlist.php?peerid=&taskid=");
	
	CAT_TITLE("checksum:");
	CAT("/clyun/check_fini_chsum.php?peer_id=&ff_num=&ff_checksum=");
	CAT("/clyun/get_finifiles.php?peer_id=");
	CAT("/clyun/put_finifiles.php?peer_id=");
	
	CAT_TITLE("state:");
	CAT("/clyun/get_state.php");
	CAT("/clyun/get_peer_state.php?peer_name=");
	CAT("/clyun/get_server.php");
	cl_httprsp::response_message(req->fd,buf);
	delete[] buf;
}
void cly_hthttps::show_httpconf(cl_HttpRequest_t* req)
{
	const cl_httppubconf_t *c;
	char buf[1024];

	c = m_svr.get_conf();
	sprintf(buf, "<br>svr1:<br>"
		"ver=%s<br>"
		"begintime=%s<br>"
		"bmulti_thread=%d<br>"
		"bsupport_keepalive=%d<br>"
		"max_client_num=%d<br>"
		"current_clients=%d<br>"
		"request_amount=%d<br>"
		, c->ver.c_str(), c->begin_time.c_str(), c->multi_thread ? 1 : 0,
		c->bsupport_keepalive ? 1 : 0, c->max_client_num,
		c->client_num, c->request_amount);

	c = m_svrbak.get_conf();
	sprintf(buf+strlen(buf), "<br>svr2:<br>"
		"ver=%s<br>"
		"begintime=%s<br>"
		"bmulti_thread=%d<br>"
		"bsupport_keepalive=%d<br>"
		"max_client_num=%d<br>"
		"current_clients=%d<br>"
		"request_amount=%d<br>"
		, c->ver.c_str(), c->begin_time.c_str(), c->multi_thread ? 1 : 0,
		c->bsupport_keepalive ? 1 : 0, c->max_client_num,
		c->client_num, c->request_amount);
	cl_httprsp::response_message(req->fd, buf, strlen(buf));
}
//*********************************************************************************
void cly_hthttps::rsp_error(int fd,const cly_seg_error_t& seg)
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

void cly_hthttps::login(cl_HttpRequest_t* req)
{
	string str;
	cly_seg_error_t seg_error;
	cly_seg_config_t seg_config;
	cly_login_info_t inf;
	str = req->params;
	inf.peer_name = cl_util::url_get_parameter(str,"peer_name");
	inf.ver = cl_util::url_get_parameter(str,"ver");
	inf.addr = cl_util::url_get_parameter(str,"addr");
	if(inf.peer_name.empty()||inf.peer_name.length()>31) 
	{
		seg_error.code = CLY_TERR_NO_PEER;
		rsp_error(req->fd,seg_error); 
		return; 
	}

	seg_error.code = cly_usermgrSngl::instance()->login(inf,seg_config);

	char xml[1024];
	cly_tp_xml_begin(xml);
	cly_tp_xml_add_seg_error(xml,seg_error);
	cly_tp_rspxml_add_seg_config(xml,seg_config);
	cly_tp_xml_end(xml);
	cl_httprsp::response_message(req->fd,xml,strlen(xml));
}

void cly_hthttps::keepalive(cl_HttpRequest_t* req)
{
	cly_seg_error_t seg_error;
	cly_seg_keepalive_t sk;
	cly_seg_keepalive_ack_t seg_ack;
	sk.peer_id = cl_util::atoi(cl_util::url_get_parameter(req->params,"peer_id").c_str());
	sk.peer_name = cl_util::url_get_parameter(req->params,"peer_name");
	sk.upstate = cl_util::url_get_parameter(req->params, "up_state");
	sk.downstate = cl_util::url_get_parameter(req->params, "down_state");

	seg_error.code = cly_usermgrSngl::instance()->keepalive(sk, seg_ack);

	char xml[1024];
	cly_tp_xml_begin(xml);
	cly_tp_xml_add_seg_error(xml, seg_error);
	cly_tp_rspxml_add_seg_keepalive_ack(xml, seg_ack);
	cly_tp_xml_end(xml);
	cl_httprsp::response_message(req->fd, xml, strlen(xml));
}

void cly_hthttps::update_nat(cl_HttpRequest_t* req)
{
	string params;
	int peer_id=0;
	cly_seg_error_t seg_error;
	string addr;
	params = req->params;
	peer_id = cl_util::atoi(cl_util::url_get_parameter(params,"peer_id").c_str());
	addr = cl_util::url_get_parameter(params,"addr");
	seg_error.code = cly_usermgrSngl::instance()->update_nat(peer_id,addr);
	rsp_error(req->fd,seg_error);
}

void cly_hthttps::search_source(cl_HttpRequest_t* req)
{
	string params;
	int peer_id=0;
	cly_seg_error_t seg_error;
	string hash;
	list<string> ls;
	params = req->params;
	peer_id = cl_util::atoi(cl_util::url_get_parameter(params,"peer_id").c_str());
	hash = cl_util::url_get_parameter(params,"hash");
	seg_error.code = cly_usermgrSngl::instance()->search_source(peer_id,hash,ls);
	
	char xml[2048];
	cly_tp_xml_begin(xml);
	cly_tp_xml_add_seg_error(xml,seg_error);
	cly_tp_rspxml_add_seg_list(xml,"sources",ls);
	cly_tp_xml_end(xml);
	cl_httprsp::response_message(req->fd,xml,strlen(xml));
}
void cly_hthttps::search_sources(cl_HttpRequest_t* req)
{
	return search_source(req);
}
void cly_hthttps::report_progress(cl_HttpRequest_t* req)
{
	int peer_id;
	list<string> ls;
	cly_seg_error_t seg_error;
	seg_error.code = 0;
	if(req->Content_Length>2048 ||req->Content_Length==0)
		seg_error.code = CLY_TERR_POSTBODY_SIZEOUT;
	else
	{
		if(0!=cl_net::sock_select_recv_n(req->fd,req->body+req->bodylen,(int)(req->Content_Length-req->bodylen),5000))
			seg_error.code = CLY_TERR_RECVBODY_WRONG;
		else
		{
			req->body[(int)req->Content_Length] = '\0';
			//前面为xml=
			if(0!=cly_xml_get_report_progress(req->body+4,peer_id,ls))
				seg_error.code = CLY_TERR_BODY_WRONG;
			else
				seg_error.code = cly_usermgrSngl::instance()->report_progress(peer_id,ls);
		}
	}
	rsp_error(req->fd,seg_error);
}

void cly_hthttps::report_fdfile(cl_HttpRequest_t* req)
{
	string params;
	int peer_id=0;
	cly_seg_error_t seg_error;
	string fields,val;
	params = req->params;
	peer_id = cl_util::atoi(cl_util::url_get_parameter(params,"peer_id").c_str());
	fields = cl_util::url_get_parameter(params,"fields");
	val = cl_util::url_get_parameter(params,"val");
	seg_error.code = cly_usermgrSngl::instance()->report_fdfile(peer_id,fields,val);
	rsp_error(req->fd,seg_error);
}
void cly_hthttps::update_filename(cl_HttpRequest_t* req)
{
	string params;
	cly_seg_error_t seg_error;
	string hash,name;
	params = req->params;
	hash = cl_util::url_get_parameter(params,"hash");
	name = cl_util::url_get_parameter(params,"name");
	seg_error.code = cly_usermgrSngl::instance()->update_filename(hash,name);
	rsp_error(req->fd,seg_error);
}
void cly_hthttps::get_fileinfo(cl_HttpRequest_t* req)
{
	string params;
	cly_seg_error_t seg_error;
	clyt_fileinfo_t fi;
	string hash;
	params = req->params;
	hash = cl_util::url_get_parameter(params,"hash");
	seg_error.code = cly_usermgrSngl::instance()->get_fileinfo(hash,fi);
	
	char xml[2048];
	cly_tp_xml_begin(xml);
	cly_tp_xml_add_seg_error(xml,seg_error);
	cly_tp_xml_add_seg_clyt_fileinfo(xml,fi);
	cly_tp_xml_end(xml);
	cl_httprsp::response_message(req->fd,xml,strlen(xml));
}

void cly_hthttps::report_error(cl_HttpRequest_t* req)
{
	string params;
	cly_seg_error_t seg_error;
	cly_report_error_t re;
	params = req->params;
	re.err = cl_util::atoi(cl_util::url_get_parameter(params,"err").c_str());
	re.peer_name = cl_util::url_get_parameter(params,"peer_name");
	re.appname = cl_util::url_get_parameter(params,"appname");
	re.appver = cl_util::url_get_parameter(params,"appver");
	re.systemver = cl_util::url_get_parameter(params,"systemver");
	re.description = cl_util::url_get_parameter(params,"description");
	seg_error.code = cly_usermgrSngl::instance()->report_error(re);
	rsp_error(req->fd,seg_error);
}

void cly_hthttps::new_task(cl_HttpRequest_t* req)
{
	list<string> ls_hids;
	list<string> ls_files;
	cly_seg_error_t seg_error;
	//cly_account_t ac;
	seg_error.code = 0;
	int task_id = 0;
	char* body;
	string task_name;

	if(req->Content_Length>102400 ||req->Content_Length==0)
		seg_error.code = CLY_TERR_POSTBODY_SIZEOUT;
	else
	{
		body = new char[(int)req->Content_Length+2];
		if(req->bodylen>0)
			memcpy(body,req->body,req->bodylen);
		if(0!=cl_net::sock_select_recv_n(req->fd,body+req->bodylen,(int)(req->Content_Length-req->bodylen),5000))
			seg_error.code = CLY_TERR_RECVBODY_WRONG;
		else
		{
			req->body[(int)req->Content_Length] = '\0';
			//前面为xml=
			if(0!=cly_xml_get_seg_task(body+4,task_name,ls_hids,ls_files))
				seg_error.code = CLY_TERR_BODY_WRONG;
			else
				seg_error.code = cly_usermgrSngl::instance()->new_task(task_name,ls_hids,ls_files,task_id);

			if (CLY_TERR_TASK_NAME_LIST_EMPTY == seg_error.code)
			{
				cl_util::write_log(body, (cly_settingSngl::instance()->get_datadir() + "log/new_task_error.log").c_str());
			}
		}
		delete[] body;
	}
	

	char xmlbuf[1024];
	cly_tp_xml_begin(xmlbuf);
	cly_tp_xml_add_seg_error(xmlbuf,seg_error);
	sprintf(xmlbuf+strlen(xmlbuf),"<p n=\"task_id\">%d</p>\r\n",task_id);
	cly_tp_xml_end(xmlbuf);
	cl_httprsp::response_message(req->fd,xmlbuf,strlen(xmlbuf));
}

void cly_hthttps::set_task_state(cl_HttpRequest_t* req)
{
	string params;
	cly_seg_error_t seg_error;
	string task_name;
	int state;
	params = req->params;
	task_name = cl_util::url_get_parameter(params,"task_name");
	state = cl_util::atoi(cl_util::url_get_parameter(params,"state").c_str());
	seg_error.code = cly_usermgrSngl::instance()->set_task_state(task_name,state);
	rsp_error(req->fd,seg_error);
}
void cly_hthttps::get_task_info(cl_HttpRequest_t* req)
{
	string params;
	cly_seg_error_t seg_error;
	string task_name;
	string path;
	params = req->params;
	task_name = cl_util::url_get_parameter(params,"task_name");
	seg_error.code = cly_usermgrSngl::instance()->get_task_info(task_name,path);
	if(0==seg_error.code)
		cl_httprsp::response_file(req->fd,path);
	else
		rsp_error(req->fd,seg_error);
}
void cly_hthttps::get_ddlist(cl_HttpRequest_t* req)
{
	string params;
	int peer_id=0;
	cly_seg_error_t seg_error;
	int update_id;
	string path;
	params = req->params;
	peer_id = cl_util::atoi(cl_util::url_get_parameter(params,"peer_id").c_str());
	update_id = cl_util::atoi(cl_util::url_get_parameter(params,"update_id").c_str());
	seg_error.code = cly_usermgrSngl::instance()->get_ddlist(peer_id,update_id,path);
	if(0==seg_error.code)
		cl_httprsp::response_file(req->fd,path);
	else
		rsp_error(req->fd,seg_error);
}

void cly_hthttps::check_fini_chsum(cl_HttpRequest_t* req)
{
	string params;
	int peer_id=0;
	cly_seg_error_t seg_error;
	cly_seg_fchsum_t seg_fchsum;
	params = req->params;
	peer_id = cl_util::atoi(cl_util::url_get_parameter(params,"peer_id").c_str());
	seg_fchsum.ff_num = cl_util::atoi(cl_util::url_get_parameter(params,"ff_num").c_str());
	seg_fchsum.ff_checksum = cl_util::atoll(cl_util::url_get_parameter(params,"ff_checksum").c_str());
	seg_error.code = cly_usermgrSngl::instance()->check_fini_chsum(peer_id,seg_fchsum);
	rsp_error(req->fd,seg_error);
}

void cly_hthttps::get_finifiles(cl_HttpRequest_t* req)
{
	string params;
	int peer_id=0;
	cly_seg_error_t seg_error;
	string path;
	params = req->params;
	peer_id = cl_util::atoi(cl_util::url_get_parameter(params,"peer_id").c_str());
	seg_error.code = cly_usermgrSngl::instance()->get_finifiles(peer_id,path);
	if(0==seg_error.code)
		cl_httprsp::response_file(req->fd,path);
	else
		rsp_error(req->fd,seg_error);
}

void cly_hthttps::put_finifiles(cl_HttpRequest_t* req)
{
	cly_seg_error_t seg_error;
	seg_error.code = 0;
	string params;
	int peer_id=0;
	char path[512];
	unsigned long long size=0;
	FILE *fp = NULL;

	params = req->params;
	peer_id = cl_util::atoi(cl_util::url_get_parameter(params,"peer_id").c_str());
	if(0==peer_id)
		seg_error.code = CLY_TERR_NO_PEER;
	else if(req->Content_Length<=0)
		seg_error.code = CLY_TERR_POSTBODY_SIZEOUT;
	else
	{
		sprintf(path,"%sfflist/put_finifiles_%d.txt",cly_settingSngl::instance()->get_datadir().c_str(),peer_id);
		fp = fopen(path,"wb+");
		if(!fp)
			seg_error.code = CLY_TERR_WRITE_FILE_FAILED;
	}
	
	if(0!=seg_error.code)
	{
		rsp_error(req->fd,seg_error);
		return;
	}

	//收文件
	if(req->bodylen>0)
	{
		if(1!=fwrite(req->body,req->bodylen,1,fp))
			seg_error.code = CLY_TERR_WRITE_FILE_FAILED;
		size += req->bodylen;
	}

	char *buf = new char[10240];
	int n;
	while(0==seg_error.code && size < req->Content_Length)
	{
		n = 10240;
		if(size+n>req->Content_Length)
			n = (int)(req->Content_Length - size);
		n = recv(req->fd,buf,n,0);
		if(n<=0 || 1!=fwrite(buf,n,1,fp))
		{
			seg_error.code = CLY_TERR_WRITE_FILE_FAILED;
			break;
		}
		size += n;
	}
	delete[] buf;
	fclose(fp);
	if(0==seg_error.code)
		seg_error.code = cly_usermgrSngl::instance()->put_finifiles(peer_id,path);

	rsp_error(req->fd,seg_error);
}
void cly_hthttps::get_state(cl_HttpRequest_t* req)
{
	//todo:
	clyt_state_t s;
	char *xmlbuf;
	cly_seg_error_t seg_error;
	seg_error.code = cly_usermgrSngl::instance()->get_state(s);
	xmlbuf = new char[s.peers.size()*100+1024];
	cly_tp_xml_begin(xmlbuf);
	cly_tp_xml_add_seg_error(xmlbuf,seg_error);
	if(0==seg_error.code)
	{
		sprintf(xmlbuf+strlen(xmlbuf),"<seg id=\"state\">\r\n"
			"  <p id=\"peer_num\">%d</p>\r\n"
			"  <p id=\"svr_num\">%d</p>\r\n"
			"  <p id=\"file_num\">%d</p>\r\n"
			"  <p id=\"begin_time\">%s</p>\r\n"
			"  <list id=\"peers\" size=\"%d\">\r\n",
			s.peer_num,s.svr_num,s.file_num,s.begin_time.c_str(),s.peers.size());
		for(list<string>::iterator it=s.peers.begin();it!=s.peers.end();++it)
		{
			sprintf(xmlbuf+strlen(xmlbuf),"    <p>%s</p>\r\n",(*it).c_str());
		}
		sprintf(xmlbuf+strlen(xmlbuf),"  </list>\r\n</seg>\r\n");
	}
	cly_tp_xml_end(xmlbuf);
	cl_httprsp::response_message(req->fd,xmlbuf,strlen(xmlbuf));
	delete[] xmlbuf;
}
void cly_hthttps::get_peer_state(cl_HttpRequest_t* req)
{
	string params;
	clyt_peer_t p;
	string peer_name;
	cly_seg_error_t seg_error;
	params = req->params;
	peer_name = cl_util::url_get_parameter(params,"peer_name");
	seg_error.code = cly_usermgrSngl::instance()->get_peer_state(peer_name,p);

	char xmlbuf[1024];
	cly_tp_xml_begin(xmlbuf);
	cly_tp_xml_add_seg_error(xmlbuf,seg_error);
	if(0==seg_error.code)
	{
		sprintf(xmlbuf+strlen(xmlbuf),"	<seg id=\"peer_state\">\r\n"
			"		<p n=\"peer_id\">%d</p>\r\n"
			"		<p n=\"peer_name\">%s</p>\r\n"
			"		<p n=\"peer_type\">%d</p>\r\n"
			"		<p n=\"nattype\">%d</p>\r\n"
			"		<p n=\"addr\">%s</p>\r\n"
			"		<p n=\"last_tick\">%d</p>\r\n"
			"		<p n=\"last_time\">%s</p>\r\n"
			"		<p n=\"fini_num\">%d</p>\r\n"
			"		<p n=\"fini_chsum\">%lld</p>\r\n"
			"	</seg>\r\n",p.peer_id,p.peer_name.c_str(),p.peer_type,
			p.nattype,p.addr.c_str(),(int)p.last_tick,p.last_time.c_str(),p.fini_num,p.fini_chsum);
	}
	cly_tp_xml_end(xmlbuf);
	cl_httprsp::response_message(req->fd,xmlbuf,strlen(xmlbuf));

}
void cly_hthttps::get_server(cl_HttpRequest_t* req)
{
	list<string> ls;
	char *xmlbuf;
	cly_seg_error_t seg_error;
	seg_error.code = cly_usermgrSngl::instance()->get_server(ls);
	xmlbuf = new char[ls.size() * 200 + 1024];
	cly_tp_xml_begin(xmlbuf);
	cly_tp_xml_add_seg_error(xmlbuf, seg_error);
	if (0 == seg_error.code)
	{
		sprintf(xmlbuf + strlen(xmlbuf), "<seg id=\"server\">\r\n"
			"  <p id=\"svr_num\">%d</p>\r\n"
			"  <list id=\"servers\" size=\"%d\">\r\n",
			ls.size(), ls.size());
		for (list<string>::iterator it = ls.begin(); it != ls.end(); ++it)
		{
			sprintf(xmlbuf + strlen(xmlbuf), "    <p><a target=\"_blank\" href=\"%s\">%s</a><br></p>\r\n",
				(*it).c_str(), (*it).c_str());
		}
		sprintf(xmlbuf + strlen(xmlbuf), "  </list>\r\n</seg>\r\n");
	}
	cly_tp_xml_end(xmlbuf);
	cl_httprsp::response_message(req->fd, xmlbuf, strlen(xmlbuf));
	delete[] xmlbuf;
}

