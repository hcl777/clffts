
#ifdef _MSC_VER
#include "cl_util.h"
#include "cl_net.h"
#include "cly_https.h"

int main(int argc, char** argv)
{
	cl_util::debug_memleak();
	cl_util::chdir(cl_util::get_module_dir().c_str());
	cl_net::socket_init();

	cly_https http;
	http.open(9880, 5);

	while (1)
		Sleep(1000);

	http.close();

	cl_net::socket_fini();

	return 0;
}

#endif

