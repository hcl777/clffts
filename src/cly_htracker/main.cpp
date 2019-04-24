
#include <stdio.h>
#include "cly_schedule.h"
#include "cly_hthttps.h"
#include "cl_util.h"
#include "cl_net.h"
#include "cly_setting.h"
#include "cly_usermgr.h"

void check_coredump_error();
int main(int argc,char** argv)
{
	if(cl_util::string_array_find(argc,argv,"-v")>0)
	{
		printf("#[VERSION] : %s \n",CLY_HTRACKER_VERSION);
		return 0;
	}

	cl_util::debug_memleak();
	cl_util::chdir(cl_util::get_module_dir().c_str());
	PSL_HANDLE pslh = cl_util::process_single_lockname_create();
	if(0==pslh)
	{
		printf("*** %s is runing ***\n",cl_util::get_module_name().c_str());
		return -1;
	}
	cl_util::write_log("htracker run...",(cl_util::get_module_dir()+"run.log").c_str());
	cl_net::socket_init();
	if(0!=cly_scheduleSngl::instance()->run())
	{
		printf("#*** init faild *** \n");
		goto end;
	}

	Sleep(1000);
	check_coredump_error();

	//
	cl_util::wait_exit();

end:
	cly_scheduleSngl::instance()->end();
	cly_scheduleSngl::destroy();

	cl_net::socket_fini();
	cl_util::process_single_lockname_close(pslh);
	return 0;
}

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
		cly_report_error_t i;
		i.appname = "htracker";
		i.appver = CLY_HTRACKER_VERSION;
		sprintf(buf,"coredump_%d",n);
		i.description = buf;
		i.err = CLY_ERR_COREDUMP;
		i.peer_name = "htracker_1";
		i.systemver = cl_util::get_system_version_str();
		cly_usermgrSngl::instance()->report_error(i);

		cl_util::file_delete(path+".bak");
		cl_util::file_rename(path,path+".bak");
	}
}
