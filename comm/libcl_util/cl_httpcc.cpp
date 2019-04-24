#include "cl_httpcc.h"

#include "cl_net.h"


cl_httpcc::cl_httpcc(cl_reactor* rc)
	: m_fd(INVALID_SOCKET)
	, m_rc(rc)
	, m_mb(-1,102400)
{
	reset();
}


cl_httpcc::~cl_httpcc()
{
}
void cl_httpcc::reset()
{
	m_state = CL_DISCONNECTED;
	m_head = "";
	m_mb.wpos = 0;
	m_recv_size = 0;
	m_req_times = 0;
}
void cl_httpcc::close_socket()
{
	if (m_fd != INVALID_SOCKET)
	{
		closesocket(m_fd);
		m_fd = INVALID_SOCKET;
	}
}
void cl_httpcc::on_connected()
{
	m_state = CL_CONNECTED;
	m_rc->register_handler(this, SE_READ);
	call(cl_httpccListener::Connected(), this);
}
int cl_httpcc::connect(unsigned int ip, unsigned short port)
{
	disconnect();
	m_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == m_fd)
	{
		return -1;
	}
	if (0 != m_rc->register_handler(this, SE_WRITE))
	{
		close_socket();
		return -1;
	}

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(ip);
	if (SOCKET_ERROR == ::connect(m_fd, (sockaddr*)&addr, sizeof(addr)))
	{
#ifdef _WIN32
		int err = WSAGetLastError();
		if (WSAEWOULDBLOCK != err)
#else
		if (EINPROGRESS != errno)
#endif
		{
			//DEBUGMSG("#***connect() failed! \n");
			m_rc->unregister_handler(this, SE_WRITE);
			close_socket();
			return -1;
		}
		m_state = CL_CONNECTING;
	}
	else
	{
		m_rc->unregister_handler(this, SE_WRITE);
		on_connected(); //可能里面发送数据会注册写，所以不要在后面un write
	}
	return 0;
}
int cl_httpcc::disconnect()
{
	if (CL_DISCONNECTED != m_state)
	{
		m_rc->unregister_handler(this, SE_BOTH);
		close_socket();
		reset();
		call(cl_httpccListener::Disconnected(), this);
	}
	return 0;
}
int cl_httpcc::send_req(const string& server, const string& cgi, int64_t ibegin/*=0*/, int64_t iend/*=-1*/, int httpver/*=1*/)
{
	string header;
	m_head = "";
	m_req_begin = ibegin;
	m_req_end = iend;
	m_recv_size = 0;
	m_mb.wpos = 0;
	m_req_times++;
	format_gethead(header, server, cgi, ibegin, iend, httpver);
	if (0 != cl_net::sock_select_send_n(m_fd, header.c_str(), header.length(), 100))
	{
		disconnect();
		return -1;
	}
	return 0;
}

int cl_httpcc::handle_input()
{
	if (CL_CONNECTED != m_state)
		return 0;
	//read
	int ret;
	while (CL_CONNECTED == m_state)
	{
		ret = m_mb.buflen-m_mb.wpos-1;
		ret = this->recv(m_mb.write_ptr(), ret);
		if (ret <= 0)
		{
			disconnect();
			return -1;
		}
		m_mb.wpos += ret;
		on_data();
	}
	return 0;
}
int cl_httpcc::handle_output()
{
	//connected后取消
	if (CL_CONNECTING == m_state)
	{
		int err = 0;
		socklen_t len = sizeof(err);
		getsockopt(m_fd, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
		if (err)
		{
			//DEBUGMSG("***TCPChannel::handle_output()::CONNECTING::errno=0x%x\n",err);
			disconnect();
			return -1;
		}
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		len = sizeof(addr);
		if (0 == getsockname(m_fd, (sockaddr*)&addr, &len))
		{
			//DEBUGMSG("#TCP connect() ok my ipport=%s:%d \n",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
		}
		//DEBUGMSG("#TCP connect() ok desip(%s:%d) \n",cl_net::ip_htoa(m_hip),(int)m_hport);
		
		m_rc->unregister_handler(this, SE_WRITE);
		on_connected(); //可能里面发送数据会注册写，所以不要在后面un write
	}
	else
	{
		assert(0);
	}
	return 0;
}
int cl_httpcc::handle_error()
{
	int err = 0;
	socklen_t len = sizeof(err);
	getsockopt(m_fd, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
	//DEBUGMSG("***TcpConnection::OnError()::errno=0x%x\n",err);
	disconnect();
	return 0;
}
int cl_httpcc::recv(char* buf, int size)
{
	int ret = ::recv(m_fd, buf, size, 0);
	if (ret > 0)
	{
		return ret;
	}
	else
	{
#ifdef _WIN32
		int err = WSAGetLastError();
		if (-1 == ret && WSAEWOULDBLOCK == err)
#else
		if (-1 == ret && EAGAIN == errno)
#endif
			return 0;
		else
		{
			disconnect();
			return -1;
		}
	}
}
void cl_httpcc::on_data()
{
	if (m_head.empty())
	{
		m_mb.buf[m_mb.wpos] = '\0';
		char* p = strstr(m_mb.buf, "\r\n\r\n");
		if (!p)
		{
			//数据满都找不到标志
			if (m_mb.buflen-m_mb.wpos-1 == 0)
				this->disconnect();
			return;
		}
		*(p + 2) = '\0';
		m_head = m_mb.buf;
		m_recv_size = 0;
		call(cl_httpccListener::RspHead(), this, (char*)m_head.c_str(), (int)m_head.length());
		if (CL_CONNECTED != m_state)
			return;
		unsigned int len = (unsigned int)(m_mb.wpos - (p + 4 - m_mb.buf));
		if (len>0)
		{
			m_recv_size += len;
			call(cl_httpccListener::RspData(), this, p + 4, len);
		}
	}
	else
	{
		m_recv_size += m_mb.wpos;
		call(cl_httpccListener::RspData(), this, m_mb.buf, m_mb.wpos);
	}

	m_mb.wpos = 0;
}
void cl_httpcc::format_gethead(string& header, const string& server, const string& cgi, int64_t ibegin/*=0*/, int64_t iend/*=-1*/, int httpver/*=1*/)
{
	header = "";
	///1:方法,请求的路径,版本
	header = "GET ";
	header += cgi;
	if (0 == httpver)
		header += " HTTP/1.0";
	else
		header += " HTTP/1.1";
	header += "\r\n";

	///2:主机
	header += "Host: ";
	header += server;
	header += "\r\n";

	///3:接收的数据类型
	header += "Accept: */* \r\n";
	//header += "Accept-Encoding: \r\n";

	///4:浏览器类型
	header += "User-Agent: Mozilla/4.0 (compatible; clyun 1.1;)\r\n";

	header += "Pragma: no-cache\r\n";
	header += "Cache-Control: no-cache\r\n";

	///5:请求的数据起始字节位置(断点续传的关键)
	if (ibegin != 0 || iend != -1)
	{
		char szTemp[64];
		header += "";
		sprintf(szTemp, "Range: bytes=%lld-", ibegin);
		header += szTemp;
		if (iend > ibegin)
		{
			sprintf(szTemp, "%lld", iend);
			header += szTemp;
		}
		header += "\r\n";
	}

	///6:连接设置,保持
	header += "Connection: Keep-Alive \r\n";
	///最后一行:空行
	header += "\r\n";
}

