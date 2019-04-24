#pragma once
#include <string.h>

class cl_membuf
{
public:
	cl_membuf(int size)
		:buf(new char[size+1])
		,buflen(size)
		,rpos(0)
		,wpos(0){buf[size]='\0';}
	~cl_membuf(void){delete[] buf;}
public:
	char	*buf;
	int		buflen,rpos,wpos;
public:
	int len() const {return wpos-rpos;}
	int overage() const {return buflen-wpos;}
	char* read_ptr() const {return buf+rpos;}
	char* write_ptr() const {return buf+wpos;}
	void move0() //ÒÆµ½Í·²¿
	{
		wpos -= rpos;
		if(rpos>0 && wpos>0)
			memmove(buf,buf+rpos,wpos);
		rpos = 0;
		buf[wpos] = '\0';
	}
};

