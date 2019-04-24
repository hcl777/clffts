#include "cl_checksum.h"


unsigned char cl_checksum8(const unsigned char* buf,int len)
{
	unsigned char sum = 0;
	for(int i=0;i<len;sum += buf[i++]);
	return sum;
}
unsigned int cl_checksum32(const unsigned char* buf,int len)
{
	unsigned int sum = 0;
	const unsigned char *p = buf;
	unsigned char ex[4];
	int i=0;
	int n=len>>2;
	int m=len&0x03;
	for(;i<n;i++,p+=4)
		sum += *(unsigned int*)p;
	for(i=0;i<m;++i,++p)
		ex[i]=*p;
	for(;i<4;++i)
		ex[i]=0;
	sum += *(unsigned int*)ex;
	return sum;
}
