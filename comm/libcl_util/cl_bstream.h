#pragma once
#include "cl_ntypes.h"
#include <assert.h>
#include "cl_string.h"

enum {SS_LITTLE_ENDIAN=0,SS_BIG_ENDIAN=1,SS_HOST_ENDIAN=2};
class cl_bstream
{
public:
	cl_bstream(char endian=SS_LITTLE_ENDIAN);
	cl_bstream(int buflen,char endian=SS_LITTLE_ENDIAN);
	cl_bstream(char* buf,int buflen,int datalen=0,char endian=SS_LITTLE_ENDIAN);
	virtual ~cl_bstream(void);
public:
	static bool little_endian();
	static void swap(void *arr,int len);

	//h:host endian; b:big endian; l:little endian ;
	static void htol(void *arr,int len);
	static void ltoh(void *arr,int len);
	static void htob(void *arr,int len);
	static void btoh(void *arr,int len);

	static short htol16(short val);  
	static short ltoh16(short val);  
	static short htob16(short val); 
	static short btoh16(short val); 

	static sint32 htol32(sint32 val); 
	static sint32 ltoh32(sint32 val); 
	static sint32 htob32(sint32 val); 
	static sint32 btoh32(sint32 val);  

	static sint64 htol64(sint64 val); 
	static sint64 ltoh64(sint64 val); 
	static sint64 htob64(sint64 val); 
	static sint64 btoh64(sint64 val);  

public:
	int ok() const { return m_state;}
	char* buffer() const { return m_buf;}
	char* read_ptr() const { return m_buf+m_rpos; }
	char* write_ptr() const { return m_buf+m_wpos; }
	int buffer_size() const { return m_size;}
	int length() const { return m_wpos-m_rpos; }
	int tellr() const {return m_rpos;}
	int tellw() const {return m_wpos;}
	void zero_rw() {m_rpos=m_wpos=0;}
	int seekr(int pos);
	int seekw(int pos);
	int skipr(int len);
	int skipw(int len);
	int read(void *arr,int len);
	int write(const void *arr,int len);
	int set_memery(int pos,void *arr,int len);
	int get_memery(int pos,void *arr,int len);

	template<typename T>
	int read_array(T *arr,int len)
	{
		for(int i=0;i<len;++i)
			(*this) >> arr[i];
		return m_state;
	}

	template<typename T>
	int write_array(T *arr,int len)
	{
		for(int i=0;i<len;++i)
			(*this) << arr[i];
		return m_state;
	}

	int read_string(char* str,int maxsize);
	int write_string(const char* str);

	int operator >> (char& val);
	int operator >> (uchar& val);
	int operator >> (short& val);
	int operator >> (ushort& val);
	int operator >> (sint32& val);
	int operator >> (uint32& val);
	int operator >> (sint64& val);
	int operator >> (uint64& val);
	//浮点数，假设大多数CPU架构都是IEEE754 标准，不考虑转换。没有字节顺的说法
	//float = 32位，double=64 位
	int operator >> (float& val); 
	int operator >> (double& val);

	int operator << (char val);
	int operator << (uchar val);
	int operator << (short val);
	int operator << (ushort val);
	int operator << (sint32 val);
	int operator << (uint32 val);
	int operator << (sint64 val);
	int operator << (uint64 val);
	int operator << (float val);
	int operator << (double val);

protected:
	void htomy(void *arr,int len);
	void mytoh(void *arr,int len);
	int check_resize(int more);

public:
	int attach(char* buf,int buflen,int datalen=0);
	void reset();
	int fitsize32(int pos)
	{
		//pos位置打上32位包大小
		assert(m_wpos >= (pos + 4));
		if (m_wpos >= (pos + 4))
		{
			int tmp = m_wpos;
			htomy(&tmp, 4);
			set_memery(pos, &tmp, 4);
			return 0;
		}
		m_state = -1;
		return m_state;
	}
protected:
	char m_endian;
	char* m_buf;
	int m_size;
	int m_state;
	int m_rpos;
	int m_wpos;
	bool m_mynew;
};

int operator << (cl_bstream& ss, const cl_string& s);
int operator >> (cl_bstream& ss, cl_string& s);
int operator << (cl_bstream& ss, const puid_t& inf);
int operator >> (cl_bstream& ss, puid_t& inf);
int operator << (cl_bstream& ss, const fhash_t& inf);
int operator >> (cl_bstream& ss, fhash_t& inf);

typedef cl_bstream cl_ptlstream;

//#define  PTL_ENDIAN_TYPE      SS_LITTLE_ENDIAN
//class cl_ptlstream : public cl_bstream
//{
//public:
//	cl_ptlstream() : cl_bstream((char)PTL_ENDIAN_TYPE) {}
//	cl_ptlstream(int buflen) : cl_bstream(buflen,(char)PTL_ENDIAN_TYPE) {}
//	cl_ptlstream(char* buf,int buflen,int datalen=0) : cl_bstream(buf,buflen,datalen,(char)PTL_ENDIAN_TYPE) {}
//	
//};

