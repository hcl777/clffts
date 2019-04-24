
#include <stdio.h>
#include "cl_util.h"
#include "cl_net.h"
#include "cl_umpsearch.h"
#include "cl_RDBFile64.h"
#include "cl_cpusn.h"
#include "cly_test.h"
#include "cl_memblock.h"
void test_read(const char* path, int mode);


int main(int argc, char** argv)
{
	
	cl_cyclist<string> ls[3];
	ls[0].push_back(new char[1024]);
	string p;
	p = ls[0].front();
	ls[0].pop_front();
	printf("p=%s\n", p.c_str());
	p = ls[0].front();
	ls[0].pop_front();
	printf("p=%s\n", p.c_str());
	p = ls[0].front();
	ls[0].pop_front();
	printf("p=%s\n", p.c_str());


	//cl_net::socket_init();
	//cly_testLock tl;
	//tl.init();


	while (1)
	{
		Sleep(1000);
		
	}
		
}
int main1(int argc,char** argv)
{
	//printf("check_size()=%d \n",cl_ERDBFile64::check_size_ok(argv[1]));
	//cl_umpsarech_main(argc,argv);
	return 0;
}

void test_read_add(char* buf, int size, size64_t& res, int mode)
{
	for (int i = 0; i < size - 7; i += 8)
	{
		if (1 == mode)
			res ^= *(size64_t*)(buf + i);
		else
			res += *(size64_t*)(buf + i);
	}
}
void test_read(const char* path,int mode)
{
	cl_ERDBFile64 file;
	char *buf = new char[102400];
	size64_t size, read_size;
	int n;
	read_size = 0;
	size64_t result = 0;
	DWORD begin_tick = GetTickCount();
	int i = 0;
	if (0 == file.open(path, F64_READ, RDBF_AUTO))
	{
		size = file.get_file_size();
		while (read_size < size)
		{
			n = file.read(buf, 102400);
			if (n <= 0)
				break;
			if(mode>0)
				test_read_add(buf, n, result, mode);
			read_size += n;
			if (++i % 1000 == 0)
			{
				printf("%.2f \n", read_size * 100 / (double)size);
			}
		}
	}
	else
	{
		printf("*** open file fail! \n");
	}
	delete[] buf;
	DWORD second = (GetTickCount() - begin_tick)/1000;
	read_size >>= 20;
	if (second > 0)
	{
		printf("mode=%d, read_size=%d MB,second=%d s\n"
			"speed = %d MB/S , res=%lld \n",
			mode, (int)read_size, (int)second, (int)(read_size / second),result);
	}
}


