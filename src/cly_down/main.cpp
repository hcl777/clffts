

#include <stdio.h>
#include "cl_util.h"
#include "cl_net.h"
#include "cly_d.h"
#include "cl_unicode.h"
#include "cl_lic.h"
#include "cly_error.h"

void test();


void on_signal_exit(int sig)
{
	cl_util::signal_exit();
}
void check_fail()
{
	cl_util::write_buffer_to_file("coredump",8,(cl_util::get_module_dir()+"core.5192").c_str());
	cl_util::signal_exit();
}
void check_coredump_error();
int main(int argc,char** argv)
{

	cl_util::linux_register_signal_exit(on_signal_exit);
	cl_util::debug_memleak();
	cl_unicode::setlocal_gb2312();
	//-v version:
	if(cl_util::string_array_find(argc,argv,"-h")>0
		||cl_util::string_array_find(argc,argv,"--help")>0
		||cl_util::string_array_find(argc,argv,"-v")>0)
	{
		cly_print_help();
		return 0;
	}

	//load config:
	string errmsg;
	cly_config_t conf;
	if(0!=cly_config_load(conf))
	{
		printf("*** load xml config faild! \n");
		return -1;
	}
	//down_path
	int argi = 0;
	string down_path;
	if(0<(argi=cl_util::string_array_find(argc,argv,"--down_path")))
	{
		if(argi+1<argc)
			down_path = argv[argi+1];
		cl_util::str_replace(down_path,"#","|");
		cl_util::str_replace(down_path,"@",">");
	}
	if(!down_path.empty())
		conf.down_path = down_path;

	//-c  check config:
	if(cl_util::string_array_find(argc,argv,"-c")>0)
	{
		char buf[2048];
		cly_config_sprint(&conf,buf,2048);
		printf("%s",buf);

		unsigned int cid[4];
		memset(cid, 0, 16);
		cl_util::get_cpuid(cid);
		printf("\ncpu0:%08X-%08X-%08X-%08X\n", cid[0],cid[1],cid[2],cid[3]);
		return 0;
	}

	//singleton:
	cl_net::socket_init();
	PSL_HANDLE pslh = cl_util::process_single_lockname_create();
	if(0==pslh)
	{
		printf("*** %s is runing! \n",cl_util::get_module_name().c_str());
		return 0;
	}
	test();

	
	printf("%s run:\n",cl_util::get_module_name().c_str());
	cl_util::write_log("down run...",(cl_util::get_module_dir()+"run.log").c_str());

	if(0!=clyd::instance()->init(&conf))
		return -1;

	//cl_licClientSngl::instance()->run(conf.peer_name,CLY_DOWN_VERSION,check_fail,36000);

	Sleep(1000);
	check_coredump_error();

	//**********************
#if defined(_WIN32) && defined(_DEBUG)
	while('q'!=getchar())
		Sleep(1000);
#else
	cl_util::wait_exit();
#endif
	//**********************
	
	clyd::instance()->fini();
	clyd::destroy();

	//cl_licClientSngl::instance()->end();
	//cl_licClientSngl::destroy();

	//end
	cl_net::socket_fini();
	cl_util::process_single_lockname_close(pslh);
	return 0;
}


#include "cly_cTracker.h"
void check_coredump_error()
{
	//脚本启动时，每启动一次，就将数字写入coredump.txt文件中，从0开始写
	//所以检查到大于0时表示coredump过，检查完删除coredump.txt
	char buf[1024] = {0,};
	string path = cl_util::get_module_dir()+"coredump.txt";
	cl_util::read_buffer_from_file(buf,1024,path.c_str());
	int n = cl_util::atoi(buf);
	if(n>0)
	{
		sprintf(buf,"coredump_%d",n);
		cly_error_report(CLY_ERR_COREDUMP,buf);

		cl_util::file_delete(path+".bak");
		cl_util::file_rename(path,path+".bak");
	}
}


//*****************************
//test:

#include "cl_bittable.h"
#include "cl_fhash.h"
#include "cly_fileinfo.h"
#include "cl_util.h"

void test()
{
	//cl_util::chdir(cl_util::get_module_dir().c_str());

}



