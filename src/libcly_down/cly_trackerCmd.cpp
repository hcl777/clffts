#include "cly_trackerCmd.h"

#include "cly_cTracker.h"
#include "cly_d.h"
#include "cly_fileinfo.h"
#include "cl_util.h"
#include "cly_downAuto.h"
#include "uac.h"
#include "cl_unicode.h"
#include "cly_downloadMgr.h"
#include "cly_server.h"

cly_trackerCmd::cly_trackerCmd(void)
{
}


cly_trackerCmd::~cly_trackerCmd(void)
{
}
int cly_trackerCmd::init()
{
	cTrackerConf_t tconf;
	tconf.peer_name = cly_pc->peer_name;
	tconf.tracker = cly_pc->server_tracker;
	tconf.app_version = CLY_DOWN_VERSION;
	tconf.lang_utf8 = cly_pc->lang_utf8;
	tconf.func = on_ctracker_msg;
	tconf.workdir = CLY_DIR_TRACKER;
	tconf.fdfile_fields = "fdtype|hash|ftype|name|subhash";
	tconf.progress_fields = "hash|ftype|name|speedkb|progress|fails";
	if(!cly_pc->uac_default_addr.empty())
	{
		char buf[1024];
		sprintf(buf,"%s-%s:%d",cly_pc->uac_default_addr.c_str(),CLYSET->m_local_ip.c_str(),cly_pc->uac_port);
		tconf.uacaddr = buf; //可以预指字
	}
	cly_cTrackerSngl::instance()->init(tconf);
	cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this), 1, 18000);

	return 0;
}
void cly_trackerCmd::run()
{
	cly_cTrackerSngl::instance()->run();
}

void cly_trackerCmd::fini()
{
	cl_timerSngl::instance()->unregister_all(static_cast<cl_timerHandler*>(this));
	cly_cTrackerSngl::instance()->end();
	cly_cTrackerSngl::destroy();
}
void cly_trackerCmd::on_timer(int e)
{
	switch(e)
	{
	case 1:
	{
		//更新down状态
		char buf[1024];
		UAC_statspeed_t ss;
		uac_get_statspeed(ss);
		sprintf(buf, "%d-%d:%d-%d-%d:%d", cly_downloadMgrSngl::instance()->get_down_num(),
			cly_downloadMgrSngl::instance()->get_conn_num(),
			(ss.app_recvspeedB >> 10), (ss.valid_recvspeedB >> 10), (ss.recvspeedB >> 10),
			cly_pc->dconf.limit_down_speedKB);
		cly_cTrackerSngl::instance()->m_info.down_state = buf;

		sprintf(buf, "%d:%d-%d:%d-%d", cly_serverSngl::instance()->get_peer_num(),
			(ss.valid_sendspeedB>>10), (ss.sendspeedB >> 10),
			cly_pc->dconf.limit_share_speediKB, cly_pc->dconf.limit_share_speedKB);
		cly_cTrackerSngl::instance()->m_info.up_state = buf;
		break;
	}
	default:
		break;
	}
}

//***********************************************************************
void cly_trackerCmd::on_ctracker_msg(int msg,void* param,void* param2)
{
	switch(msg)
	{
	case CLY_CTRACKER_MSG_CONF:
		{
			cly_seg_config_t * conf = (cly_seg_config_t*)param;
			clyd::instance()->update_login_config(conf);
		}
		break;
	case CLY_CTRACKER_MSG_KEEPALIVE_ACK:
	{
		cly_seg_keepalive_ack_t * seg = (cly_seg_keepalive_ack_t*)param;
		clyd::instance()->update_keepalive_ack(seg);
	}
	break;
	case CLY_CTRACKER_MSG_SOURCE:
		{
			clyd::instance()->add_source(*(string*)param,*(list<string>*)param2);
		}
		break;
	case CLY_CTRACKER_MSG_DDLIST:
		{
			//保存为ddlist.txt
			string path =  CLY_DIR_AUTO + "ddlist.txt";
			cl_util::put_stringlist_to_file(path,*(list<string>*)param);
			clyd::instance()->load_ddlist(path);
		}
		break;
	case CLY_CTRACKER_CHSUM_DISTINCT:
		{
			DEBUGMSG("# *** CLY_CTRACKER_CHSUM_DISTINCT \n");
			//cly_cTrackerSngl::instance()->get_finifiles();

			//不再更新数据库数据，直接以本地数据更新（方便测试，并且本地缓冲.sha3文件也可快速恢复）
			string path = CLY_DIR_TRACKER + "~put_finifiles.txt";
			if(0==clyd::instance()->save_readyfile_utf8(path))
			{
				DEBUGMSG("# put_finifiles() \n");
				cly_cTrackerSngl::instance()->put_finifiles(path);
			}
		}
		break;
	case CLY_CTRACKER_MSG_GET_FINIFILES:
		{
			assert(0);
			DEBUGMSG("# *** CLY_CTRACKER_MSG_GET_FINIFILES \n");
			string path,str,hash,name,subhash;
			list<string> ls;
			path = *(string*)param;
			cl_util::get_stringlist_from_file(path,ls);
			ls.pop_front();
			ls.pop_front();
			for(list<string>::iterator it=ls.begin();it!=ls.end();++it)
			{
				if(cly_pc->lang_utf8)
					str = *it;
				else
					cl_unicode::UTF_8ToGB2312(str,(*it).c_str(),(*it).length());
				//hash|name|subhash
				hash = cl_util::get_string_index(str,0,"|");
				name = cl_util::get_string_index(str,1,"|");
				subhash = cl_util::get_string_index(str,2,"|");
				DEBUGMSG("GET_FINIFILES share(%s) \n",str.c_str());
				clyd::instance()->share_file(hash,subhash,name,name,false);
			}

			path = CLY_DIR_TRACKER + "~put_finifiles.txt";
			if(0==clyd::instance()->save_readyfile_utf8(path))
			{
				DEBUGMSG("# put_finifiles() \n");
				cly_cTrackerSngl::instance()->put_finifiles(path);
			}
			else
			{
				DEBUGMSG("#*** save_readyfile_utf8(%s) fail \n",path.c_str());
			}
		}
		break;
	default:
		break;
	}
}
void cly_trackerCmd::on_uac_natok(int ntype)
{
	UAC_sockaddr& ua = CLYSET->m_uacaddr;
	ua.nattype = ntype;
	if(cly_pc->uac_default_addr.empty())
		_update_nattype();
}
void cly_trackerCmd::on_uac_ipport(unsigned int ip,unsigned short port)
{
	UAC_sockaddr& ua = CLYSET->m_uacaddr;
	ua.ip = ip;
	ua.port = port;
	if(cly_pc->uac_default_addr.empty())
		_update_nattype();
}

//******************************************************************
void cly_trackerCmd::_update_nattype()
{
	char buf[1024];
	UAC_sockaddr& ua = CLYSET->m_uacaddr;
	if(ua.nattype>0&&ua.nattype<6)
	{
		sprintf(buf,"%s:%d:%d-%s:%d",cl_net::ip_htoa(ua.ip),(int)ua.port,ua.nattype,CLYSET->m_local_ip.c_str(),cly_pc->uac_port);
		cly_cTrackerSngl::instance()->update_nat(buf);
	}
}
