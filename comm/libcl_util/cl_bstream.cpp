#include "cl_bstream.h"
#include <string.h>

#define SS_OVERFLOW_READ_N_RETURN(n)    assert(m_rpos+n <= m_wpos);				  \
										if(m_rpos+n > m_wpos){ m_state=-1; return m_state;} 


#define SS_OVERFLOW_WRITE_N_RETURN(n)   if(-1==check_resize(n)) return m_state;


cl_bstream::cl_bstream(char endian/*=SS_LITTLE_ENDIAN*/)
{
	//m_size = 4;
	//m_buf = new char[m_size];
	m_size = 0;
	m_buf = NULL;
	m_mynew = true;
	m_rpos = 0;
	m_wpos = 0;
	m_state = 0;
	m_endian = endian;
}
cl_bstream::cl_bstream(int buflen,char endian/*=SS_LITTLE_ENDIAN*/)
{
	assert(buflen>0);
	m_size = buflen;
	m_buf = new char[m_size];
	m_mynew = true;
	m_rpos = 0;
	m_wpos = 0;
	m_state = 0;
	m_endian = endian;
}
cl_bstream::cl_bstream(char* buf,int buflen,int datalen/*=0*/,char endian/*=SS_LITTLE_ENDIAN*/)
{
	assert(buf && datalen>=0 && buflen >= datalen);
	m_buf = buf;
	m_size = buflen;
	m_rpos = 0;
	m_wpos = datalen;
	m_state = 0;
	m_endian = endian;
	m_mynew = false;
}
cl_bstream::~cl_bstream(void)
{
	if(m_mynew && m_buf)
		delete[] m_buf;
}

bool cl_bstream::little_endian()
{
	union{
		int a;
		char b;
	}c;
	c.a = 1;
	return (c.b==1);
}
void cl_bstream::swap(void *arr,int len)
{
	assert(len>1);
	char *ptr = (char*)arr;
	char tmp = 0;
	for(int i=0;i<len/2;++i)
	{
		tmp = ptr[i];
		ptr[i] = ptr[len-1-i];
		ptr[len-1-i] = tmp;
	}
}

//h:host endian; b:big endian; l:little endian ;
void cl_bstream::htol(void *arr,int len)
{
	if(!little_endian()) 
		swap(arr,len);
}
void cl_bstream::ltoh(void *arr,int len)
{
	if(!little_endian()) 
		swap(arr,len);
}
void cl_bstream::htob(void *arr,int len)
{
	if(little_endian()) 
		swap(arr,len);
}
void cl_bstream::btoh(void *arr,int len)
{
	if(little_endian()) 
		swap(arr,len);
}

short cl_bstream::htol16(short val)
{
	htol(&val,sizeof(short));
	return val;
}  
short cl_bstream::ltoh16(short val)
{
	ltoh(&val,sizeof(short));
	return val;
}  
short cl_bstream::htob16(short val)
{
	htob(&val,sizeof(short));
	return val;
} 
short cl_bstream::btoh16(short val)
{
	btoh(&val,sizeof(short));
	return val;
} 

sint32 cl_bstream::htol32(sint32 val)
{
	htol(&val,sizeof(sint32));
	return val;
}
sint32 cl_bstream::ltoh32(sint32 val)
{
	ltoh(&val,sizeof(sint32));
	return val;
}
sint32 cl_bstream::htob32(sint32 val)
{
	htob(&val,sizeof(sint32));
	return val;
}
sint32 cl_bstream::btoh32(sint32 val)
{
	btoh(&val,sizeof(sint32));
	return val;
}  

sint64 cl_bstream::htol64(sint64 val)
{
	htol(&val,sizeof(sint64));
	return val;
}
sint64 cl_bstream::ltoh64(sint64 val)
{
	ltoh(&val,sizeof(sint64));
	return val;
}
sint64 cl_bstream::htob64(sint64 val)
{
	htob(&val,sizeof(sint64));
	return val;
} 
sint64 cl_bstream::btoh64(sint64 val)
{
	btoh(&val,sizeof(sint64));
	return val;
} 


int cl_bstream::seekr(int pos)
{
	if(0!=m_state)
		return m_state;
	if(pos<0 || pos>m_wpos)
		return -1;
	m_rpos = pos;
	return m_state;
}
int cl_bstream::seekw(int pos)
{
	if(0!=m_state)
		return m_state;
	if(pos<m_rpos || pos>m_size)
		return -1;
	m_wpos = pos;
	return m_state;
}
int cl_bstream::skipr(int len)
{
	SS_OVERFLOW_READ_N_RETURN(len)
	m_rpos += len;
	return m_state;
}
int cl_bstream::skipw(int len)
{
	SS_OVERFLOW_WRITE_N_RETURN(len)
	m_wpos += len;
	return m_state;
}
int cl_bstream::read(void *arr,int len)
{
	SS_OVERFLOW_READ_N_RETURN(len)
	memcpy(arr,m_buf+m_rpos,len);
	m_rpos += len;
	return m_state;
}
int cl_bstream::write(const void *arr,int len)
{
	SS_OVERFLOW_WRITE_N_RETURN(len)
	memcpy(m_buf+m_wpos,arr,len);
	m_wpos += len;
	return m_state;
}
int cl_bstream::set_memery(int pos,void *arr,int len)
{
	if(pos+len>m_size)
		return -1;
	memcpy(m_buf+pos,arr,len);
	return 0;
}
int cl_bstream::get_memery(int pos,void *arr,int len)
{
	if(pos+len>m_size)
		return -1;
	memcpy(arr,m_buf+pos,len);
	return 0;
}

int cl_bstream::read_string(char* str,int maxsize)
{
	SS_OVERFLOW_READ_N_RETURN(4)
	uint32 n = 0;
	*this >> n;
	if(n==0)
	{
		str[0] = '\0';
		return m_state;
	}
	SS_OVERFLOW_READ_N_RETURN((int)n)
	if(n>=(uint32)maxsize)
	{
		skipr(n);
		str[0] = '\0';
		m_state = -1;
		return m_state;
	}
	this->read(str,n);
	str[n] = '\0';
	return m_state;
}
int cl_bstream::write_string(const char* str)
{
	uint32 n = 0;
	if(str) n=(uint32)strlen(str);
	SS_OVERFLOW_WRITE_N_RETURN((int)(n+4))
	*this << n;
	if(n) this->write(str,n);
	return m_state;
}

int cl_bstream::operator >> (char& val)
{
	SS_OVERFLOW_READ_N_RETURN(1)
	val = m_buf[m_rpos++];
	return m_state;
}
int cl_bstream::operator >> (uchar& val)
{
	SS_OVERFLOW_READ_N_RETURN(1)
	val = (uchar)m_buf[m_rpos++];
	return m_state;
}
int cl_bstream::operator >> (short& val)
{
	read(&val,sizeof(short));
	mytoh(&val,sizeof(short));
	return m_state;
}
int cl_bstream::operator >> (ushort& val)
{
	read(&val,sizeof(ushort));
	mytoh(&val,sizeof(ushort));
	return m_state;
}
int cl_bstream::operator >> (sint32& val)
{
	read(&val,sizeof(sint32));
	mytoh(&val,sizeof(sint32));
	return m_state;
}
int cl_bstream::operator >> (uint32& val)
{
	read(&val,sizeof(uint32));
	mytoh(&val,sizeof(uint32));
	return m_state;
}
int cl_bstream::operator >> (sint64& val)
{
	read(&val,sizeof(sint64));
	mytoh(&val,sizeof(sint64));
	return m_state;
}
int cl_bstream::operator >> (uint64& val)
{
	read(&val,sizeof(uint64));
	mytoh(&val,sizeof(uint64));
	return m_state;
}
int cl_bstream::operator >> (float& val)
{
	read(&val,sizeof(float)); //32λ
	return m_state;
}
int cl_bstream::operator >> (double& val)
{
	read(&val,sizeof(double)); //64
	return m_state;
}


int cl_bstream::operator << (char val)
{
	SS_OVERFLOW_WRITE_N_RETURN(1)
	m_buf[m_wpos]=val;
	m_wpos++;
	return m_state;
}
int cl_bstream::operator << (uchar val)
{
	SS_OVERFLOW_WRITE_N_RETURN(1)
	m_buf[m_wpos]=(char)val;
	m_wpos++;
	return m_state;
}
int cl_bstream::operator << (short val)
{
	htomy(&val,sizeof(short));
	write(&val,sizeof(short));
	return m_state;
}
int cl_bstream::operator << (ushort val)
{
	htomy(&val,sizeof(ushort));
	write(&val,sizeof(ushort));
	return m_state;
}
int cl_bstream::operator << (sint32 val)
{
	htomy(&val,sizeof(sint32));
	write(&val,sizeof(sint32));
	return m_state;
}
int cl_bstream::operator << (uint32 val)
{
	htomy(&val,sizeof(uint32));
	write(&val,sizeof(uint32));
	return m_state;
}
int cl_bstream::operator << (sint64 val)
{
	htomy(&val,sizeof(sint64));
	write(&val,sizeof(sint64));
	return m_state;
}
int cl_bstream::operator << (uint64 val)
{
	htomy(&val,sizeof(uint64));
	write(&val,sizeof(uint64));
	return m_state;
}
int cl_bstream::operator << (float val)
{
	write(&val,sizeof(float)); //32λ
	return m_state;
}
int cl_bstream::operator << (double val)
{
	write(&val,sizeof(double)); //64
	return m_state;
}


void cl_bstream::htomy(void *arr,int len)
{
	if(SS_LITTLE_ENDIAN==m_endian)
	{
		if(!little_endian())
			swap(arr,len);
	}
	else if(SS_BIG_ENDIAN==m_endian)
	{
		if(little_endian())
			swap(arr,len);
	}
}
void cl_bstream::mytoh(void *arr,int len)
{
	htomy(arr,len);
}
int cl_bstream::check_resize(int more)
{
	if(-1!=m_state && m_wpos+more > m_size)
	{
		if(m_mynew)
		{
			int newsize = m_size * 2;
			if (newsize < m_wpos+more)
				newsize = m_wpos+more;
			char* newbuf = new char[newsize];
			if(!newbuf)
			{
				m_state = -1;
				return -1;
			}
			if (m_wpos>0)
			{
				memcpy(newbuf,m_buf,m_wpos);
			}
			if(m_buf)
				delete[] m_buf;
			m_buf = newbuf;
			m_size = newsize;
		}
		else
		{
			m_state = -1;
		}
	}
	return m_state;
}


int cl_bstream::attach(char* buf,int buflen,int datalen/*=0*/)
{
	if(!buf || !buflen)
		return -1;
	reset();
	m_buf = buf;
	m_size = buflen;
	m_wpos = datalen;
	return 0;
}

void cl_bstream::reset()
{
	if(m_mynew && m_buf)
	{
		delete[] m_buf;
	}
	m_buf = 0;
	m_mynew = false;
	m_size = 0;
	m_rpos = 0;
	m_wpos = 0;
	m_state = 0;
}
int operator << (cl_bstream& ss, const cl_string& s)
{
	return ss.write_string(s.c_str());
}
int operator >> (cl_bstream& ss, cl_string& s)
{
	char buf[1024]={0,};
	int ret=ss.read_string(buf,1024);
	if(0==ret) s = buf;
	return ret;
}

int operator << (cl_bstream& ss, const puid_t& inf)
{
	ss.write(inf,PUIDLEN);
	return ss.ok();
}
int operator >> (cl_bstream& ss, puid_t& inf)
{
	ss.read(inf,PUIDLEN);
	return ss.ok();
}
int operator << (cl_bstream& ss, const fhash_t& inf)
{
	ss.write(inf,HASHLEN);
	return ss.ok();
}
int operator >> (cl_bstream& ss, fhash_t& inf)
{
	ss.read(inf,HASHLEN);
	return ss.ok();
}



