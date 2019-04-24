#include "cl_bittable.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

cl_bittable::cl_bittable(void)
	:m_buf(NULL)
	,m_bitsize(0)
	,m_setsize(0)
{
}
cl_bittable::~cl_bittable(void)
{
	free();
}
void cl_bittable::alloc(int bitsize,const unsigned char* vbuf/*=0*/)
{
	free();
	assert(bitsize>0);
	if(bitsize<=0) return;
	m_bitsize = bitsize;
	m_buf = new unsigned char[(m_bitsize+7)>>3];
	if(0==vbuf)
	{
		memset(m_buf,0,(m_bitsize+7)>>3);
	}
	else
	{
		memcpy(m_buf,vbuf,(m_bitsize+7)>>3);
		m_setsize = mm_popcnt_ub(m_buf,m_bitsize);
	}
}
void cl_bittable::free()
{
	if(m_buf)
	{
		delete[] m_buf;
		m_buf = NULL;
		m_bitsize = 0;
		m_setsize = 0;
	}
}
void cl_bittable::set(int i,bool v/*=true*/)
{
	assert(i>=0 && i<m_bitsize);
	if(i>=0&&i<m_bitsize)
	{
		int index = i>>3;
		int offset = i&0x07;
		unsigned char mask = (unsigned char)0x01<<offset;
		if(v)
		{
			if((m_buf[index]&mask)==0)
			{
				m_buf[index] |= mask;
				m_setsize++;
			}
		}
		else
		{
			if((m_buf[index]&mask)!=0)
			{
				m_buf[index] &= ~mask;
				m_setsize--;
			}
		}
	}
}
void cl_bittable::setall(bool v/*=true*/)
{
	int n = v?0xffffffff:0;
	if(m_buf)
	{
		memset(m_buf,n,(m_bitsize+7)>>3);
		if(v) 
			m_setsize = m_bitsize;
		else 
			m_setsize = 0;
	}
}
bool cl_bittable::get(int i) const
{
	assert(i>=0 && i<m_bitsize);
	if(i>=0&&i<m_bitsize)
	{
		int index = i>>3;
		int offset = i&0x07;
		unsigned char mask = (unsigned char)0x01<<offset;
		return (m_buf[index]&mask)!=0;
	}
	return false;
}
const cl_bittable& cl_bittable::operator=(const cl_bittable& bt)
{
	alloc(bt.get_bitsize(),bt.buffer());
	return *this;
}
int cl_bittable::mm_popcnt_u8(unsigned char n)
{
	n = (n&0x55) + ((n>>1)&0x55);
	n = (n&0x33) + ((n>>2)&0x33);
	n = (n&0x0f) + ((n>>4)&0x0f);
	return n;
}
int cl_bittable::mm_popcnt_u32(unsigned int n)
{ 
	n = (n &0x55555555) + ((n >>1) &0x55555555) ; //相邻1位和,2位保存
    n = (n &0x33333333) + ((n >>2) &0x33333333) ; //相邻2位和,4位保存
    n = (n &0x0f0f0f0f) + ((n >>4) &0x0f0f0f0f) ; //相邻4位和,8位保存
    n = (n &0x00ff00ff) + ((n >>8) &0x00ff00ff) ; //相邻8位和,16位保存
    n = (n &0x0000ffff) + ((n >>16) &0x0000ffff); //相邻16位和,32位保存 
    return n ; 
}
int cl_bittable::mm_popcnt_ub(const unsigned char *buf,int bitsize)
{
	int i,n,m,j,k;
	char c;
	n = bitsize>>3;
	m = n&0x03;
	j = bitsize&0x7;
	k=0;
	for(i=0;i+3<n;i+=4,buf+=4)
		k += mm_popcnt_u32(*(unsigned int*)buf);
	for(i=0;i<m;++i,++buf)
		k += mm_popcnt_u8(*buf);
	if(j>0)
	{
		c=*buf;
		for(i=0;i<j;++i,c>>=1)
			k+=(c&1);
	}
	return k;
}
int operator << (cl_bstream& ss, const cl_bittable& inf)
{
	assert(inf.get_setsize() == inf.mm_popcnt_ub(inf.buffer(),inf.get_bitsize()));
	ss << inf.get_bitsize();
	if(inf.get_bitsize()>0)
		ss.write(inf.buffer(),((inf.get_bitsize()+7)>>3));
	return ss.ok();
}
int operator >> (cl_bstream& ss, cl_bittable& inf)
{
	sint32 bitsize = 0;
	ss >> bitsize;
	if(bitsize>0)
	{
		inf.alloc(bitsize,(const unsigned char*)ss.read_ptr());
		ss.skipr((bitsize+7)>>3);
	}
	return ss.ok();
}


//test:
#include <stdio.h>
void cl_bittable::print(cl_bittable& bt)
{
	printf("#bt[%d / %d]:",bt.get_setsize(),bt.get_bitsize());
	for(int i=0;i<bt.get_bitsize();++i)
	{
		if(0==i%8)printf(" ");
		printf("%d",bt[i]?1:0);
	}
	printf("\n");
}
void cl_bittable::test()
{
	//bool b = false;
	cl_bittable bt;
	bt.alloc(50);

	bt.setall();
	printf("bit,set=%d:",bt.mm_popcnt_ub(bt.buffer(),bt.get_bitsize()));
	for(int i=0;i<bt.get_bitsize();++i)
	{
		if(0==i%8)printf(" ");
		printf("%d",bt[i]?1:0);
	}
	printf("\n");

	bt.set(17,false);
	bt.set(27,false);
	bt.set(32,false);
	bt.set(39,false);
	printf("bit,set=%d:",bt.mm_popcnt_ub(bt.buffer(),bt.get_bitsize()));
	for(int i=0;i<bt.get_bitsize();++i)
	{
		if(0==i%8)printf(" ");
		printf("%d",bt[i]?1:0);
	}
	printf("\n");

	
	bt.set(12,false);
	bt.set(46,false);
	bt.set(48,false);
	printf("bit,set=%d:",bt.mm_popcnt_ub(bt.buffer(),bt.get_bitsize()));
	for(int i=0;i<bt.get_bitsize();++i)
	{
		if(0==i%8)printf(" ");
		printf("%d",bt[i]?1:0);
	}
	printf("\n");
	//b = false;
}

