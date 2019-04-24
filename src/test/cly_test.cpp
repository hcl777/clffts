#include "cly_test.h"
#include "cl_basetypes.h"
#include "cly_cTrackerInfo.h"

#define CLY_CGI_ "/clyun"
#define CLY_HTRACK_ADDR "39.105.106.77:8098"

#define CLY_CT_HTTPREQ_BASE_DEFINE \
	int ret = 0;	\
	char url[2048];	\
	cly_seg_error_t seg_err;	\
	cl_xml xml;

int login(const string& peer_name,int& peer_id)
{
	CLY_CT_HTTPREQ_BASE_DEFINE
	cly_seg_config_t seg_config;

	sprintf(url, "http://%s%s/login.php?peer_name=%s&ver=%s&addr=%s",
		CLY_HTRACK_ADDR, CLY_CGI_,
		peer_name.c_str(), "cly_test_20180813","");
	//DEBUGMSG("#login ( %s )... \n",url);

	ret = cly_tracker_http_get(url, xml, seg_err, 1024);
	if (0 == ret)
	{
		if (0 == seg_err.code)
		{
			if (0 == cly_xml_get_seg_config(xml, seg_config))
			{
				peer_id = seg_config.peer_id;
				printf("#");
				return 0;
			}
		}
		else
		{
			printf("*le%d", seg_err.code);
		}
	}
	{
		printf("*");
	}
	return -1;
}
int keepalive(const string& peer_name, int peer_id)
{
	//cly_seg_keepalive_ack_t seg;
	CLY_CT_HTTPREQ_BASE_DEFINE
		sprintf(url, "http://%s%s/keepalive.php?peer_id=%d&peer_name=%s",
			CLY_HTRACK_ADDR, CLY_CGI_, peer_id,peer_name.c_str());
	ret = cly_tracker_http_get(url, xml, seg_err, 1024);
	if (0 == ret)
	{
		if (0 == seg_err.code)
		{
			printf(".");
			return 0;
		}
	}
	printf("$");
	return -1;
}


cly_testLock::cly_testLock()
	
{
}


cly_testLock::~cly_testLock()
{
}
void cly_testLock::init()
{
	
	activate(4);
}

int cly_testLock::work(int e)
{
	string peer_name = "test1111111111";
	int peer_id = 0;
	while (1)
	{
		Sleep(1);
		if (0 == login(peer_name, peer_id))
		{
			Sleep(1);
			keepalive(peer_name, peer_id);
		}
	}

	return 0;
}
