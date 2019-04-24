
#include <stdio.h>
#include "cl_util.h"
#include "cl_net.h"
#include "cl_httpc2.h"

int http_post(const char* filepath,const char* url);
int main(int argc,char** argv)
{
	if(1==argc || cl_util::string_array_find(argc,argv,"-h")>0)
	{
		printf("%s --httppost localpath url \n",cl_util::get_module_name().c_str());
		return 0;
	}
	int i;
	cl_util::debug_memleak();
	cl_util::chdir(cl_util::get_module_dir().c_str());
	cl_net::socket_init();
	if((i=cl_util::string_array_find(argc,argv,"--httppost"))>0)
	{
		if(i+2<argc)
		{
			http_post(argv[i+1],argv[i+2]);
		}
	}

	cl_net::socket_fini();
	return 0;
}
int http_post(const char* filepath,const char* url)
{
	string rsp;
	if(0==cl_httpc2::post_file(rsp,filepath,url))
		printf("%s \n",rsp);
	else
		printf("post fail! \n");
	return 0;
}


