#pragma once

#include "cl_thread.h"


//udp multicast protocol search
//经测试，多播实际上也只能在同一个子网内可搜索到。无法夸子网段发送多播。

class cl_umpsearch : public cl_thread
{
public:
	cl_umpsearch(void);
	virtual ~cl_umpsearch(void);

public:
	int run(const char* multiaddr);
	void end();
	virtual int work(int e);

	static int find(const char* multiaddr);
private:
	bool				m_brun;
	char				m_multiip[64];
	unsigned short		m_port;
};

int cl_umpsarech_main(int argc,char** argv);

