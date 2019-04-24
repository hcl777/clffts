#pragma once
#include "cl_bstream.h"

/**************
按位操作置位.
bitsize指位数.
setsize指置1的个数.
自动计算位值为1的个数.
**************/
class cl_bittable
{
public:
	cl_bittable(void);
	~cl_bittable(void);

	void alloc(int bitsize,const unsigned char* vbuf=0); //如果有vbuf,就拷
	void free();
	
	const unsigned char* buffer() const {return m_buf;}
	int get_bitsize()const {return m_bitsize;}
	int get_setsize()const {return m_setsize;}
	bool is_setall() const {return (m_bitsize!=0 && m_setsize==m_bitsize);}

	void set(int i,bool v=true);
	void setall(bool v=true);
	bool get(int i) const;
	bool operator[](unsigned int i) const {return get(i);}
	const cl_bittable& operator=(const cl_bittable& bt);

	unsigned char* buffer(){return m_buf;}

	//计算二进制整数含1的个数
	static int mm_popcnt_u8(unsigned char n);
	static int mm_popcnt_u32(unsigned int n);
	static int mm_popcnt_ub(const unsigned char *buf,int bitsize);

	//test:
	static void print(cl_bittable& bt);
	static void test();
private:
	unsigned char* m_buf;
	int m_bitsize; //
	int m_setsize; //设计为1的个数
};

int operator << (cl_bstream& ss, const cl_bittable& inf);
int operator >> (cl_bstream& ss, cl_bittable& inf);
