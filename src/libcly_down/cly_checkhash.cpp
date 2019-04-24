#include "cly_checkhash.h"
#include "cly_filemgr.h"
#include "cl_crc32.h"

cly_checkhash::cly_checkhash(void)
	:m_brun(false)
{
}


cly_checkhash::~cly_checkhash(void)
{
}

int cly_checkhash::run()
{
	if(m_brun) return 1;
	m_brun = true;
	this->activate();
	cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),1,1000);
	return 0;
}
void cly_checkhash::end()
{
	m_brun = false;
	cl_timerSngl::instance()->unregister_all(static_cast<cl_timerHandler*>(this));
	wait();
}
int cly_checkhash::work(int e)
{
	cly_checkhash_info_t inf;
	while(m_brun)
	{
		if(m_ls.empty())
		{
			Sleep(200);
			continue;
		}
		{
			cl_TLock<Mutex> l(m_mt);
			inf = m_ls.front();
			m_ls.pop_front();
		}
		if(0==crc32_fileblock(inf))
		{
			cl_TLock<Mutex> l(m_mt);
			m_ls2.push_back(inf);
		}
	}
	return 0;
}
void cly_checkhash::on_timer(int e)
{
	if(!m_ls2.empty())
	{
		cl_TLock<Mutex> l(m_mt);
		for(list<cly_checkhash_info_t>::iterator it=m_ls2.begin();it!=m_ls2.end();++it)
		{
			cly_fileinfo* fi = cly_filemgrSngl::instance()->get_downinfo((*it).hash);
			if(fi) fi->on_check_subhash_result((*it));
		}
		m_ls2.clear();
	}
}
void cly_checkhash::add_check(const cly_checkhash_info_t& inf)
{
	cl_TLock<Mutex> l(m_mt);
	m_ls.push_back(inf);
}
int cly_checkhash::crc32_fileblock(cly_checkhash_info_t& inf)
{
	inf.res_subhash = 0;
	unsigned int nhash = CL_CRC32_FIRST;
	const int BUFSIZE = 64<<10; //16384
	char *buf = new char[BUFSIZE];
	size64_t pos,size;
	int n;
	pos = inf.pos;
	size = inf.pos + inf.size;
	while(pos<size)
	{
		n = BUFSIZE;
		if((size64_t)n > size-pos)
			n = (int)(size-pos);
		n = cly_filemgrSngl::instance()->read_down_data(inf.hash,pos,buf,n);
		if(n<=0)
			break;
		pos += n;
		nhash = cl_crc32_write(nhash,(unsigned char*)buf,n);
	}
	delete[] buf;
	if(pos==size)
	{
		nhash &= 0x3fffffff; //ตอ30ฮป.
		inf.res_subhash = nhash;
		return 0;
	}
	return -1;
}


