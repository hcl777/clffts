#include "cly_https.h"
#include "cl_util.h"
#include "cly_jni.h"

cly_https::cly_https()
	:m_readnum(0)
{
}


cly_https::~cly_https()
{
}
int cly_https::open(unsigned short port, int threadnum)
{
	if (m_svr.is_open())
		return 1;

	bool keepalive = false; //如果使用很容易成死连接占满线程
	if (0 != m_svr.open(port, NULL, on_request, this, true, threadnum, CLY_HTTPS_VERSION, keepalive))
	{
		close();
		return -1;
	}
	m_svrbak.open(port+1, NULL, on_request, this, true, 4, CLY_HTTPS_VERSION, keepalive);

	return 0;
}
void cly_https::close()
{
	m_svr.stop();
	m_svrbak.stop();
}
int cly_https::on_request(cl_HttpRequest_t* req)
{
	printf("# http_req: %s \n", req->cgi);
	cly_https* ph = (cly_https*)req->fun_param;

#define EIF(func) else if(strstr(req->cgi,"/clyun/"#func".php")) ph->func(req)

	if (0 == strcmp(req->cgi, "/") || strstr(req->cgi, "/index."))
		ph->index(req);
	EIF(show_httpconf);
	EIF(read_rdbfile);
	EIF(get_readnum);
	else
	{
		//目前只当他为文件
		string path = cl_util::get_module_dir().c_str();
		path += (req->cgi + 1);
		cl_httprsp::response_file(req->fd, path);
	}

	return 0;
}
void cly_https::index(cl_HttpRequest_t* req)
{
	char buf[2048];
#define CAT_TITLE(s) sprintf(buf+strlen(buf),"<br><strong><font>%s</font></strong><br>",s)
#define CAT(cgi) sprintf(buf+strlen(buf),"<a target=\"_blank\" href=\"%s\">%s</a><br>",cgi,cgi)

	sprintf(buf, "<h3>HTTP API:</h3>");
	CAT_TITLE("system:");
	CAT("/version");
	CAT("/clyun/show_httpconf.php");

	CAT_TITLE("file:");
	CAT("/clyun/read_rdbfile.php?path=");
	CAT("/clyun/get_readnum.php");

	cl_httprsp::response_message(req->fd, buf);
}
void cly_https::show_httpconf(cl_HttpRequest_t* req)
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
	sprintf(buf + strlen(buf), "<br>svr2:<br>"
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
void cly_https::read_rdbfile(cl_HttpRequest_t* req)
{

	m_mt.lock();
	m_readnum++;
	m_mt.unlock();

	jni_fire_rdbread_begin();
	string path = cl_util::url_get_parameter(req->params, "path");
	cl_httprsp::response_rdbfile(req, path);

	m_mt.lock();
	m_readnum--;
	m_mt.unlock();
}
void cly_https::get_readnum(cl_HttpRequest_t* req)
{
	char buf[1024];
	sprintf(buf, "%d", m_readnum);
	cl_httprsp::response_message(req->fd, buf);
}

