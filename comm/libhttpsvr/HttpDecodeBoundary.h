#pragma once
#include "cl_basetypes.h"

class HttpDecodeBoundary
{
public:
	HttpDecodeBoundary(const char* boundary);
	~HttpDecodeBoundary(void);
	typedef struct tagMemBlock
	{
		char *buf;
		int bufsize;
		int datasize;
		tagMemBlock(void)
			:buf(NULL)
			,bufsize(0)
			,datasize(0)
		{
		}
		~tagMemBlock(void)
		{
			reset();
		}
		void reset()
		{
			if(buf)
				delete[] buf;
			buf = NULL;
			bufsize = 0;
			datasize = 0;
		}
		//检测将要加入size数据是否可以
		bool tryputsize(int size)
		{
			if(datasize + size > bufsize)
			{
				bufsize = datasize + size + 1024;
				char *newbuf = new char[bufsize];
				if(datasize>0)
					memcpy(newbuf,buf,datasize);
				if(buf) delete[] buf;
				buf = newbuf;
			}
			return true;
		}
	}MemBlock_t;
public:
	int put_in(const char* buf,int size);
	int get_out(char *buf,int size);

	static string get_boundary_from_head(const char* httpheader);
private:
	string m_boundary;
	MemBlock_t	m_mem;
};
