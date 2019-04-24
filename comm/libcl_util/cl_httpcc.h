#pragma once
#include "cl_reactor.h"
#include "cl_memblock.h"
#include "cl_speaker.h"
#include "cl_memblock.h"

//http client channel
class cl_httpcc;
class cl_httpccListener
{
public:
	virtual ~cl_httpccListener(void) {}

	template<int I>
	struct S { enum { T = I }; };

	typedef S<1> Connected;
	typedef S<2> Disconnected;
	typedef S<3> RspHead;
	typedef S<4> RspData;

	virtual void on(Connected, cl_httpcc* ch) {}
	virtual void on(Disconnected, cl_httpcc* ch) {}
	virtual void on(RspHead, cl_httpcc* ch,char* buf, int size) {}
	virtual void on(RspData, cl_httpcc* ch,char* buf, int size) {}
};

class cl_httpcc : public cl_rthandle
	, public cl_caller<cl_httpccListener>
{
public:
	cl_httpcc(cl_reactor* rc);
	virtual ~cl_httpcc();

	int connect(unsigned int ip, unsigned short port);
	int disconnect();
	int send_req(const string& server, const string& cgi, int64_t ibegin = 0, int64_t iend = -1, int httpver = 1);

	virtual int sock() { return (int)m_fd; }
	virtual int handle_input();
	virtual int handle_output();
	virtual int handle_error();
private:
	void reset();
	void close_socket();
	void on_connected();
	int recv(char* buf,int size);
	void on_data();
	void format_gethead(string& header, const string& server, const string& cgi, int64_t ibegin=0, int64_t iend=-1, int httpver=1);
protected:
	SOCKET			m_fd;
	cl_reactor*		m_rc;
	string			m_head;
	cl_memblock		m_mb;

	uint64_t		m_req_begin, m_req_end, m_req_times;
	uint64_t		m_recv_size;
	CL_GET(int, m_state, _state)
};

