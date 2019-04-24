#include "cl_rsa.h"
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/****************************************************
cl_rsa32 (n,e,d):
实现密钥长度为17至32位以内的算法.只供学习,但考虑32位以内用64位运算不溢出,所以.
产生的n : 0x0000ffff < n < 0xffffffff.
明文按16位分块加密, 密文块以32位保存.
由于n太小,所以可以轻易分解p,q.并求得d.所以知道公钥可直接破解.
*****************************************************/


//判断是否为素数
bool cl_rsa32_is_prime(unsigned int i)
{
	if(i<4) return true;
	unsigned int k = (unsigned int)sqrt((double)i);
	for(unsigned int j=2;j<=k;j++)
	{
		if(i%j==0)
			return false;
	}
	return true;
}

//判断是否互质,返回1表示互质
//Euclid算法: 转展相除.最后
int cl_rsa32_mut_prime(unsigned int a,unsigned int b)
{
	if(a%b==0) //如果b是a的约数.
		return b;
    else
    	return cl_rsa32_mut_prime(b,a%b);
}
//n=N((p-1)*(q-1)), u=e,return d
unsigned int cl_rsa32_niyuan(unsigned int n,unsigned int u)//求逆元
{
	//ed mod N = 1,即 de - xN = 1,
	unsigned int n1=n;
	unsigned int n2=u;
	unsigned int b1=0;
	unsigned int b2=1;
	unsigned int q=n1/n2;
	unsigned int r=n1-q*n2;
	while(r!=0)
	{
		n1=n2;
		n2=r;
		int t=b2;
		b2=b1-q*b2;
		b1=t;
		q=n1/n2;
		r=n1-q*n2;
	}
	if(n2!=1)
	{
		return 0 ;
	}
	else
	{
		return (b2+n)%n;
	}
}

int cl_rsa32_randkey1(unsigned int& p,unsigned int& q,unsigned int& N,unsigned int& n,unsigned int& e,unsigned int& d)
{
	srand((unsigned int)time(NULL)+(unsigned int)clock());
	while(1)
	{
		p = rand()%0x0000ffff;
		if(p<1000)
			continue;
		if(cl_rsa32_is_prime(p))
			break;
	}
	while(1)
	{
		q = rand()%0x0000ffff;
		if(p<1000||q==p)
			continue;
		if(cl_rsa32_is_prime(q))
		{
			n = q*p;
			if(n>0x00080000)
				break;
		}
	}
	N = (p-1)*(q-1);

	//选e,与N互质,并且e小于N
	while(1)
	{
		e = rand()%0x00000fff;
		if(e<18 || e>N)
			continue;
		if(cl_rsa32_is_prime(e))
		{
			if(1==cl_rsa32_mut_prime(N,e))
				break;
		}
	}

	//选d, ed mod N = 1,即 de - xN = 1
	d = cl_rsa32_niyuan(N,e);

	return 0;
}
int cl_rsa32_randkey(unsigned int& n,unsigned int& e,unsigned int& d)
{
	unsigned int p,q,N;
	while(1)
	{
		cl_rsa32_randkey1(p,q,N,n,e,d);

		//检验,d不要太大.
		unsigned long long ed = e*(unsigned long long)d;
		unsigned int t1 = (unsigned int)(ed>N?ed%N:N%ed);
		if(1==t1 && d<0x000fffff)
		{
			break;
		}
	}
	return 0;
}
//按16位加密,余数部分不加.osize 必须预先指定为osize>=2size
int cl_rsa32_encrypt(unsigned int n,unsigned int e,const char* buf,int size,char* obuf,int* osize)
{
	unsigned long long m;
	unsigned short s;

	if(*osize<2*size)
		return -1;

	for(int i=0;i<size-1;i+=2)
	{
		s = *(unsigned short*)(buf+i);
		m = 1;
		for(unsigned int j=0;j<e;j++)
			m = (s*m)%n;
		*(unsigned int*)(obuf+2*i) = (unsigned int)m;
	}
	if(size%2)
	{
		//最后1字节不加密
		obuf[2*(size-1)] = buf[size-1];
		*osize = 2*size-1;
	}
	else
		*osize = 2*size;
	return 0;
}
//按32位解密,余数部分不加.osize 必须预先指定为osize>=(size/2+1)
int cl_rsa32_decrypt(unsigned int n,unsigned int d,const char* buf,int size,char* obuf,int* osize)
{
	unsigned long long m;
	unsigned int s;

	if(*osize<(size/2+1))
		return -1;

	for(int i=0;i<size-3;i+=4)
	{
		s = *(unsigned int*)(buf+i);
		m = 1;
		for(unsigned int j=0;j<d;j++)
			m = (s*m)%n;
		*(unsigned short*)(obuf+i/2) = (unsigned short)m;
	}
	if(size%4)
	{
		assert(1==size%4);
		obuf[size/4*2] = buf[size-1];
		*osize = size/4*2+1;
	}
	else
		*osize = size/2;
	return 0;
}

//
#include "cl_basetypes.h"
void cl_rsa32_test()
{
	//unsigned int p,q,N;
	unsigned int n,e,d;
	int j=0;
	//unsigned long long ed = 0;
	while(++j)
	{
		DWORD tick = GetTickCount();

		//cl_rsa32_randkey1(p,q,N,n,e,d);
		//tick = GetTickCount() - tick;
		//printf("#%ds:(p,q,N,n,e,d) =(%d,%d,%d,%d,%d,%d)\n",tick,p,q,N,n,e,d);
		
		cl_rsa32_randkey(n,e,d);
		tick = GetTickCount() - tick;
		printf("#%d s:(n,e,d) =(%d,%d,%d)\n",(int)tick,n,e,d);

		char buf1[1024];
		char buf2[1024];
		char buf3[1024];
		int size1,size2,size3;
		size2 = 1024;
		size3 = 1024;
		sprintf(buf1,"ASDFTDASDFAefgaa 2345qt'__ l !\n");
		size1 = (int)strlen(buf1)+1;
		cl_rsa32_encrypt(n,e,buf1,size1,buf2,&size2);
		cl_rsa32_decrypt(n,d,buf2,size2,buf3,&size3);
		buf3[size3] = '\0';
		printf("#%s \n",buf1);
		printf("#%s \n",buf3);
		
		Sleep(500);
	}
	getchar();
}

