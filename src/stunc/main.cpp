

#include "uac_stunc_test.h"

int main(int argc, char** argv)
{
	printf("[CMD] %s -s stunsvr -p binport \n", cl_util::get_module_name().c_str());

	string stunsvr = "uacstun.imovie.com.cn:8111";
	unsigned short port = 9208;
	int i;
	if ((i = cl_util::string_array_find(argc, argv, "-s")) > 0)
	{
		if(i+1<argc)
			stunsvr = argv[i+1];
	}
	if ((i = cl_util::string_array_find(argc, argv, "-p")) > 0)
	{
		if (i + 1<argc)
			port = atoi(argv[i + 1]);
	}

	cl_net::socket_init();
	cl_util::chdir(cl_util::get_module_dir().c_str());

	uac_stunc_test st;
	st.start(stunsvr.c_str(),port);
	while (1)
	{
		Sleep(1000);
	}
	return 0;
}

