#include "cly_db.h"
#include "cly_setting.h"
#include "cl_util.h"
#include "cl_crc32.h"
#include "cl_checksum.h"
#include "cl_fhash.h"

//=======================================================
#define  DBIP          "localhost"
#define  DBPORT        3306
#define  DBNAME        "vodstat"
#define  DBUSER        "root"
#define  DBPASS        "mysql"
#define  UNIX_SOCKET   "/tmp/mysql.sock"
//#define  UNIT_SOCKET   NULL
//========================================================


cly_dbmgr::cly_dbmgr(void)
{
}


cly_dbmgr::~cly_dbmgr(void)
{
}

int cly_dbmgr::init()
{
	if(0!=m_db.connect(cly_settingSngl::instance()->get_dbaddr().c_str()))
		return -1;
	if(0!=db_load())
		return -1;
	cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),1,30000);
	return 0;
}
void cly_dbmgr::fini()
{
	m_db.disconnect();
	cl_timerSngl::instance()->unregister_all(static_cast<cl_timerHandler*>(this));
}

void cly_dbmgr::on_timer(int e)
{
	m_db.ping();
}
int cly_dbmgr::db_load()
{
	int ret;
	char sql[1024];
	int affected_rows=0;
	char** row;
	char buf[128];
	int i = 0;

	clyt_PeerMap& peers = cly_sourceSngl::instance()->m_peers;
	clyt_PeerHidMap& hids = cly_sourceSngl::instance()->m_hidpeers;
	clyt_FileMap& files = cly_sourceSngl::instance()->m_files;
	clyt_SourceMap& srcs = cly_sourceSngl::instance()->m_srcs;
	list<clyt_peer_t*>& svr_peers = cly_sourceSngl::instance()->m_svr_peers;
	clyt_peer_t *peer;
	clyt_file_t *file;
	clyt_source_t *src;
	clyt_progress_t pg;


	//file table
	sprintf(sql,"select id,hash,name,subhash from file;");
	ret = m_db.query.query(sql,&affected_rows);
	if(0==ret)
	{
		while(0==m_db.query.next_row(&row))
		{
			file = new clyt_file_t();
			file->file_id = atoi(row[0]);
			file->hash = row[1];
			file->name = row[2];
			file->subhash = row[3];
			files[file->file_id] = file;
			//DEBUGMSG("# db_file(%d,%s,%s) \n",file->file_id,row[1],row[2]);

			src = new clyt_source_t();
			src->file = file;
			src->last_i_peer = 0;
			srcs[file->hash] = src;
		}
	}
	else
	{
		return -1;
	}

	//peer table
	sprintf(sql,"select id,group_id,peer_name,peer_type,addr,nattype from peer;");
	ret = m_db.query.query(sql,&affected_rows);
	if(0==ret)
	{
		while(0==m_db.query.next_row(&row))
		{
			i = 0;
			peer = new clyt_peer_t();
			peer->is_login = false;
			peer->keepalive_count = 0;
			peer->last_tick = 0;
			peer->last_time = "";
			peer->fini_chsum = 0;
			peer->peer_id = atoi(row[i++]);
			peer->group_id = atoi(row[i++]);
			peer->peer_name = row[i++];
			peer->peer_type = atoi(row[i++]);
			sprintf(buf,"%s-%d",row[i++],peer->peer_type);
			peer->addr = buf;
			peer->nattype = atoi(row[i++]);
			if(peer->nattype<0||peer->nattype>6)  peer->nattype=6;
			peers[peer->peer_id] = peer;
			hids[peer->peer_name] = peer;
			if(CLY_PEER_TYPE_CLIENT!=peer->peer_type)
				svr_peers.push_back(peer);
		}
	}
	else
	{
		return -1;
	}

	//source table
	sprintf(sql,"select file_id,peer_id,progress from source;");
	ret = m_db.query.query(sql,&affected_rows);
	if(0==ret)
	{
		while(0==m_db.query.next_row(&row))
		{
			src = cly_sourceSngl::instance()->get_source(atoi(row[0]));
			if(NULL==src)
			{
				DEBUGMSG("# *** source_file not in the file table(file_id=%s) \n",row[0]);
				continue;
			}
			pg.peer =  cly_sourceSngl::instance()->get_peer(atoi(row[1]));
			if(NULL==pg.peer)
			{
				DEBUGMSG("# *** source_file not in the peer table(file_id=%s,peer_id=%s) \n",row[0],row[1]);
				continue;
			}
			pg.progress = atoi(row[2]);
			src->ls_peer.push_back(pg);
				
			if(1000==pg.progress)
				pg.peer->add_fini_file(src->file);
		}
	}
	else
	{
		return -1;
	}

	////清理无源记录，或者无peer记录见 《数据库维护说明》；然后重启cly_htracker

	DEBUGMSG("# ----- DB SOURCE -----\n");
	DEBUGMSG("# peer num	= %d \n",peers.size());
	DEBUGMSG("# file num	= %d \n",files.size());
	DEBUGMSG("# src  num	= %d \n",srcs.size());
	DEBUGMSG("# ----- --------- -----\n");
	return 0;
}

int cly_dbmgr::db_login(const cly_login_info_t& inf,cly_seg_config_t& seg)
{
	int ret;
	char sql[1024];
	int affected_rows=0;
	char** row;
	bool binsert = false;

	if (inf.peer_name.length() < 12 || inf.peer_name.length() > 31)
		return CLY_TERR_NO_PEER;

	//查询
	sprintf(sql,"select id,group_id,used,peer_type from peer where peer_name='%s';",inf.peer_name.c_str());
	ret = m_db.query.query(sql,&affected_rows);
	if(0!=ret) return CLY_TERR_DB_WRONG;
	if(m_db.query.get_num_rows()<=0)
	{
		//插入
		char sql2[1024];
		sprintf(sql2,"insert into peer(peer_name,peer_type,appver,addr,nattype,downing_hashs,ff_num,ff_checksum,alive_time,create_time)"
			"values('%s',2,'%s','%s',6,'',0,0,now(),now());",inf.peer_name.c_str(),inf.ver.c_str(),inf.addr.c_str());
		ret = m_db.query.query(sql2,&affected_rows);
		DEBUGMSG("#db_login() insert peer_name(%s) affected %d \n",inf.peer_name.c_str(),affected_rows);
		binsert = true;
		ret = m_db.query.query(sql,&affected_rows);
	}
	if(0==ret && m_db.query.get_num_rows()>0)
	{
		m_db.query.next_row(&row);
		seg.peer_id = atoi(row[0]);
		seg.group_id = atoi(row[1]);
		seg.used = atoi(row[2]);
		seg.peer_type = atoi(row[3]);
		if(seg.used == 0)
		{
			//seg.peer_id = 0;
			return CLY_TERR_PEER_UNUSED;
		}
		cly_sourceSngl::instance()->src_login(seg.peer_id,inf.peer_name,seg.peer_type,seg.group_id);
		if(!binsert)
		{
			sprintf(sql,"update peer set appver='%s',alive_time=now() where id=%d;",inf.ver.c_str(),seg.peer_id);
			ret = m_db.query.query(sql,&affected_rows);
			DEBUGMSG("#db_login() update peer_name(%s) affected %d \n",inf.peer_name.c_str(),affected_rows);
		}
		return 0;
	}

	return CLY_TERR_DB_WRONG;
}
int cly_dbmgr::db_update_group_id(int peer_id,int group_id)
{
	int ret;
	char sql[1024];
	int affected_rows=0;
	sprintf(sql,"update peer set group_id=%d where id=%d;",group_id,peer_id);
	ret = m_db.query.query(sql,&affected_rows);
	if(0!=ret) return CLY_TERR_DB_WRONG;
	cly_sourceSngl::instance()->src_update_group_id(peer_id,group_id);
	return 0;
}
int cly_dbmgr::db_keepalive(cly_seg_keepalive_t& sk, cly_seg_keepalive_ack_t& seg)
{
	int ret;
	char sql[1024];
	int affected_rows=0;
	clyt_peer_t* peer;
	seg.used = 1;
	if (!sk.peer_name.empty())
		peer = cly_sourceSngl::instance()->get_peer(sk.peer_name);
	else
		peer = cly_sourceSngl::instance()->get_peer(sk.peer_id);
	if (!peer || !peer->is_login)
		return CLY_TERR_NO_PEER;

	peer->last_tick = GetTickCount();
	peer->last_time = cl_util::time_to_datetime_string(time(NULL));
	peer->keepalive_count++;

	//检查used
	//if (0 == peer->keepalive_count % 5)
	{
		sprintf(sql, "select used from peer where id=%d;", peer->peer_id);
		ret = m_db.query.query(sql, &affected_rows);
		if (0 == ret && m_db.query.get_num_rows() > 0)
		{
			char** row;
			m_db.query.next_row(&row);
			seg.used = atoi(row[0]);
			if (seg.used == 0)
			{
				peer->is_login = false;
				peer->keepalive_count = 0;
				return CLY_TERR_NO_PEER;
			}
		}
	}
	
	sprintf(sql,"update peer set alive_time=now(),down_state='%s',up_state='%s' where id=%d;",sk.downstate.c_str(),sk.upstate.c_str(), peer->peer_id);
	ret = m_db.query.query(sql,&affected_rows);
	if(0==ret)
		return 0;
	return CLY_TERR_DB_WRONG;
}
int cly_dbmgr::db_update_nat(int peer_id,const string& addr)
{
	if(addr.empty())
		return CLY_TERR_WRONG_PARAM;
	int ret;
	char sql[1024];
	int affected_rows=0;
	int nattype = cl_util::atoi(cl_util::get_string_index(addr,2,":").c_str(),6);
	if(nattype<0||nattype>6) nattype = 6;
	cly_sourceSngl::instance()->src_update_nat(peer_id,addr,nattype);
	sprintf(sql,"update peer set addr='%s',nattype=%d, alive_time=now() where id=%d;",addr.c_str(),nattype,peer_id);
	ret = m_db.query.query(sql,&affected_rows);
	if(0==ret)
		return 0;
	return CLY_TERR_DB_WRONG;
}
int cly_dbmgr::db_report_finifile(clyt_peer_t* peer,const cly_seg_fdfile_t& seg,bool bupdate_checksum)
{
	int peer_id = peer->peer_id;
	DEBUGMSG("#db_report_finifile(pid=%d,hash=%s) \n",peer_id,seg.hash.c_str());
	if(seg.hash.empty())
		return CLY_TERR_WRONG_PARAM;
	int ret;
	int file_id=0;
	char sql[4096];
	int affected_rows=0;
	char **row;
	long long fsize = cl_fhash_getfsize_from_sbhash(seg.subhash.c_str());

	//1。插file表，获取file_id
	//查ID
	sprintf(sql,"select id from file where hash='%s';",seg.hash.c_str());
	ret = m_db.query.query(sql,&affected_rows);
	if(0!=ret) return CLY_TERR_DB_WRONG;
	if(0==m_db.query.get_num_rows())
	{
		//插入file表
		char subhash[2048];
		if(m_db.query.escape_string(subhash,2048,seg.subhash.c_str(),m_db.get_mysql()))
		{
			sprintf(sql,"insert into file(hash,size,name,subhash,flag,extmsg) "
				"values('%s',%lld,'%s','%s',1,'');",seg.hash.c_str(),fsize,seg.name.c_str(),subhash);
			ret = m_db.query.query(sql,&affected_rows);
			if(0!=ret || 1!=affected_rows)
				return CLY_TERR_DB_WRONG;
		}
		//再查ID
		sprintf(sql,"select id from file where hash='%s';",seg.hash.c_str());
		ret = m_db.query.query(sql,&affected_rows);
		if(0!=ret || 0==m_db.query.get_num_rows())
			return CLY_TERR_DB_WRONG;
	}
	//获取file_id;
	if(0!=m_db.query.next_row(&row))
		return CLY_TERR_DB_WRONG;
	file_id = atoi(row[0]);
	if(0==file_id)
		return CLY_TERR_DB_WRONG;

	//获取到file_id插入缓冲
	cly_sourceSngl::instance()->src_finifile(peer,file_id,seg.hash,seg.subhash,seg.name);
	//2.插入source表
	sprintf(sql,"select progress from source where file_id=%d and peer_id=%d ;",file_id,peer_id);
	ret = m_db.query.query(sql,&affected_rows);
	if(0!=ret)
		return CLY_TERR_DB_WRONG;
	if(0==m_db.query.get_num_rows())
	{
		//插入,
		sprintf(sql,"insert into source(file_id,peer_id,ftype,progress,task_date,modify_time,create_time) "
			"values(%d,%d,%d,1000,curdate(),now(),now());",file_id,peer_id,seg.ftype);
	}
	else
	{
		//更新
		if(0==m_db.query.next_row(&row))
		{
			//判断是否已经完成，注意：上报进度不能上报1000的进度
			if(0==strcmp(row[0],"1000"))
				return 0;
		}
		sprintf(sql,"update source set progress=1000,ftype=%d,speed=0,modify_time=now() where file_id=%d and peer_id=%d ;",seg.ftype,file_id,peer_id);
	}
	ret = m_db.query.query(sql,&affected_rows);
	if(0!=ret)
		return CLY_TERR_DB_WRONG;

	//插入带宽统计表
	size64_t rcvB[5];
	memset(rcvB,0,sizeof(size64_t)*5);
	sscanf(seg.rcvB.c_str(),"%lld,%lld,%lld,%lld,%lld",&rcvB[0],&rcvB[1],&rcvB[2],&rcvB[3],&rcvB[4]);
	if((rcvB[0]+rcvB[1]+rcvB[2]+rcvB[3]+rcvB[4])>0)
	{
		sprintf(sql,"insert into netflow(file_id,peer_id,group_id,peer_type,file_size,rcvB0,rcvB1,rcvB2,rcvB3,rcvB4,create_time) "
			"values(%d,%d,%d,%d,%lld,%lld,%lld,%lld,%lld,%lld,now());",file_id,peer_id,peer->group_id,peer->peer_type,fsize
			,rcvB[0],rcvB[1],rcvB[2],rcvB[3],rcvB[4]);
		ret = m_db.query.query(sql,&affected_rows);
	}

	//3.更新peer 表ff_fini;
	if(bupdate_checksum)
		return  update_ff_checksum(peer_id,seg.hash,false);
	return 0;
}
int cly_dbmgr::db_update_filename(const string& hash,const string& name)
{
	if(hash.empty() || name.empty())
		return CLY_TERR_WRONG_PARAM;
	int ret;
	char sql[1024];
	int affected_rows=0;

	//提高效率，先更新内存，内存不存在的直接返回
	ret = cly_sourceSngl::instance()->src_update_filename(hash,name);
	if(0!=ret)
		return ret;

	sprintf(sql,"update file set name='%s' where hash='%s';",name.c_str(),hash.c_str());
	ret = m_db.query.query(sql,&affected_rows);
	if(0!=ret)
		return CLY_TERR_DB_WRONG;
	return 0==affected_rows?CLY_TERR_NO_CHANGE:0;
}
int cly_dbmgr::db_report_delfile(int peer_id,const cly_seg_fdfile_t& seg)
{
	if(seg.hash.empty())
		return CLY_TERR_WRONG_PARAM;
	int ret;
	int file_id=0;
	char sql[1024];
	int affected_rows=0;
	char **row;
	string id,progress;

	if(0!=(ret = get_file_id(file_id,seg.hash.c_str())))
		return ret;

	//删除缓冲源
	cly_sourceSngl::instance()->src_delfile(peer_id,file_id);

	//删除source表
	sprintf(sql,"select id,progress from source where file_id=%d and peer_id=%d;",file_id,peer_id);
	ret = m_db.query.query(sql,&affected_rows);
	if(0!=ret)
		return CLY_TERR_DB_WRONG;
	if(m_db.query.get_num_rows()==0)
		return 0;
	if(0!=m_db.query.next_row(&row))
		return CLY_TERR_DB_WRONG;
	
	id = row[0];
	progress = row[1];
	sprintf(sql,"delete from source where id=%s;",id.c_str());
	ret = m_db.query.query(sql,&affected_rows);
	if(0!=ret ||1!=affected_rows)
		return CLY_TERR_DB_WRONG;

	//如果已经完成的，则要更新sum
	if(progress == "1000")
		return update_ff_checksum(peer_id,seg.hash,true);
	return 0;
}
int cly_dbmgr::db_delsource(int peer_id,int file_id)
{
	int ret;
	char sql[1024];
	int affected_rows=0;

	sprintf(sql,"delete from source where file_id=%d and peer_id=%d;",file_id,peer_id);
	ret = m_db.query.query(sql,&affected_rows);
	if(0!=ret)
		return CLY_TERR_DB_WRONG;
	return 0;
}
int cly_dbmgr::db_report_progress(int peer_id,list<string>& ls)
{
	//第1个为files
	int ret = 0;
	int file_id=0;
	char sql[1024];
	int affected_rows=0;
	char **row;

	string hash,name;
	int ftype,speedkb,progress,fails;
	
	if(!ls.empty())
		ls.pop_front();
	for(list<string>::iterator it=ls.begin();it!=ls.end();++it)
	{
		//"hash|ftype|name|speedkb|progress|fails
		string& str = *it;
		hash = cl_util::get_string_index(str,0,"|");
		ftype = cl_util::atoi(cl_util::get_string_index(str,1,"|").c_str());
		name = cl_util::get_string_index(str,2,"|");
		speedkb = cl_util::atoi(cl_util::get_string_index(str,3,"|").c_str());
		progress = cl_util::atoi(cl_util::get_string_index(str,4,"|").c_str());
		fails = cl_util::atoi(cl_util::get_string_index(str,5,"|").c_str());
		if(hash.empty()) continue;
		if(progress>999) progress = 999;
		cly_sourceSngl::instance()->src_update_progress(peer_id,hash,progress);
		if(0!=get_file_id(file_id,hash))
		{
			DEBUGMSG("#*** UPDATE PROGRESS file(%s) not found! \n",hash.c_str());
			continue;
		}

		//2.插入source表
		sprintf(sql,"select id,progress from source where file_id=%d and peer_id=%d ;",file_id,peer_id);
		ret = m_db.query.query(sql,&affected_rows);
		if(0!=ret)
		{
			ret = CLY_TERR_DB_WRONG;
			continue;
		}
		if(0==m_db.query.get_num_rows())
		{
			//插入,
			sprintf(sql,"insert into source(file_id,peer_id,ftype,progress,speed,fails,task_date,modify_time,create_time) "
				"values(%d,%d,%d,%d,%d,%d,curdate(),now(),now());",file_id,peer_id,ftype,progress,speedkb,fails);
		}
		else
		{
			//更新
			if(0==m_db.query.next_row(&row))
			{
				//判断是否已经完成，注意：上报进度不能上报1000的进度
				if(0==strcmp(row[1],"1000"))
					continue;
				sprintf(sql,"update source set progress=%d,ftype=%d,speed=%d,fails=%d,modify_time=now() where id=%s;",progress,ftype,speedkb,fails,row[0]);
			}
		
		}
		ret = m_db.query.query(sql,&affected_rows);
		if(0!=ret)
			ret = CLY_TERR_DB_WRONG;
	}
	return ret;
}

int cly_dbmgr::db_report_error(const cly_report_error_t& inf)
{
	int ret;
	char sql[1024];
	int affected_rows=0;
	sprintf(sql,"insert into error(peer_name,err,systemver,appname,appver,description,create_time)"
		"values('%s',%d,'%s','%s','%s','%s',now());",inf.peer_name.c_str(),inf.err,
		inf.systemver.c_str(),inf.appname.c_str(),inf.appver.c_str(),inf.description.c_str());
	ret = m_db.query.query(sql,&affected_rows);
	if(0!=ret || 1!=affected_rows)
		return CLY_TERR_DB_WRONG;
	
	return 0;
}
int cly_dbmgr::db_new_task(const string& task_name,list<int>& ls_peerids,list<int>& ls_fids,int& task_id)
{
	//获取新task_id
	int ret,retcode = CLY_TERR_DB_WRONG;
	char *sql = new char[10240];
	char **row;
	int affected_rows=0;
	int pid,fid;
	bool b = true;
	task_id = 0;
	do
	{
		if(0!=(ret=get_task_id(task_id,task_name)))
		{
			sprintf(sql,"insert into task(name,peer_ids,file_ids,create_time)"
				"values('%s','%d','%d',now());",task_name.c_str(),ls_peerids.size(),ls_fids.size());
			ret = m_db.query.query(sql,&affected_rows);
			if(0!=ret)
				break;
			if(0!=get_last_id(task_id))
				break;
		}
		if(task_id<1)
			break;
		list<int>::iterator it_pid,it_fid;
		for(it_pid=ls_peerids.begin();it_pid!=ls_peerids.end();++it_pid)
		{
			pid = *it_pid;
			b = true;
			for(it_fid=ls_fids.begin();it_fid!=ls_fids.end();++it_fid)
			{
				fid = *it_fid;
				sprintf(sql,"select id from source where peer_id=%d and file_id=%d;",pid,fid);
				ret = m_db.query.query(sql,&affected_rows);
				if(0!=ret)
				{
					b = false;
					break;
				}
				if(0==m_db.query.next_row(&row))
				{
					//更新任务号
					sprintf(sql,"update source set ftype=3,task_id=%d,task_date=curdate() where id=%s ;",task_id,row[0]);
					if(0!=m_db.query.query(sql,&affected_rows))
					{
						b = false;
						break;
					}
				}
				else
				{
					//插入任务
					sprintf(sql,"insert into source(file_id,peer_id,ftype,task_id,task_date,modify_time,create_time) "
						"values(%d,%d,3,%d,curdate(),now(),now());",fid,pid,task_id);
					if(0!=m_db.query.query(sql,&affected_rows))
					{
						b = false;
						break;
					}
				}
			}
			if(!b)
				break;
			//更新peer 的task_id,这里的task_id只+1,跟
			sprintf(sql,"update peer set task_update_id=task_update_id+1 where id=%d;",pid);
			if(0!=m_db.query.query(sql,&affected_rows))
			{
				b = false;
				break;
			}
		}
		if(b)
			retcode = 0;
	}while(0);
	delete[] sql;
	DEBUGMSG("# db_new_task: task_id=%d, retcode=%d \n",task_id,retcode);
	return retcode;
}

int cly_dbmgr::db_set_task_state(const string& task_name,int state)
{
	int ret;
	char sql[1024];
	int affected_rows=0;
	int task_id = 0;

	if(0!=(ret=get_task_id(task_id,task_name)))
		return ret;
	sprintf(sql,"update source set state=%d where task_id=%d;",state,task_id);
	ret = m_db.query.query(sql,&affected_rows);
	if(0==ret)
	{
		if(affected_rows<1)
			return CLY_TERR_NO_TASK;
		return 0;
	}
	return CLY_TERR_DB_WRONG;
}
int cly_dbmgr::db_get_task_info(const string& task_name,list<cly_task_node_t>& ls)
{
	int ret;
	char sql[1024];
	int affected_rows=0;
	char **row;
	cly_task_node_t tn;
	int task_id=0;

	if(0!=(ret=get_task_id(task_id,task_name)))
		return ret;
	sprintf(sql,"select file_id,peer_id,progress from source where task_id=%d;",task_id);
	ret = m_db.query.query(sql,&affected_rows);
	if(0==ret)
	{
		while(0==m_db.query.next_row(&row))
		{
			tn.file_id = atoi(row[0]); 
			tn.peer_id = atoi(row[1]); 
			tn.progress = atoi(row[2]); 
			ls.push_back(tn);
		}
		return 0;
	}
	return CLY_TERR_DB_WRONG;
}
int cly_dbmgr::db_get_ddlist(int peer_id,int& update_id,list<cly_ddlist_node_t>& ls)
{
	int ret,peer_task_update_id;
	list<int> ls_sync_peers;
	char sql[4096];
	int affected_rows=0;
	char **row;

	cly_ddlist_node_t dn;

	if(0!=(ret=get_peer_task_update_id(peer_id,peer_task_update_id,ls_sync_peers)))
		return ret;
	if(ls_sync_peers.empty())
	{
		if(update_id==peer_task_update_id)
			return 0;
		update_id = peer_task_update_id;
		//获取未完成的，并且是分发的任务
		sprintf(sql,"select file_id,state,priority from source where peer_id=%d and progress<1000 and task_id>0;",peer_id);
	}
	else
	{
		//
		update_id = 0;
		char buf2[2048];
		list<int>::iterator it = ls_sync_peers.begin();
		sprintf(buf2,"peer_id=%d",*it);
		it++;
		for(;it!=ls_sync_peers.end();it++)
			sprintf(buf2+strlen(buf2)," or peer_id=%d",*it);
		sprintf(sql,"select file_id,state,priority,count(distinct file_id) from source where progress=1000 and state=1 and (%s) group by file_id;",buf2);
	}
	ret = m_db.query.query(sql,&affected_rows);
	if(0==ret)
	{
		while(0==m_db.query.next_row(&row))
		{
			dn.file_id = atoi(row[0]); 
			dn.state = atoi(row[1]); 
			dn.priority = atoi(row[2]); 
			ls.push_back(dn);
		}
		return 0;
	}
	
	return CLY_TERR_DB_WRONG;
}
int cly_dbmgr::db_check_fini_chsum(int peer_id,const cly_seg_fchsum_t& seg)
{
	int ret;
	char sql[1024];
	int affected_rows=0;
	char **row;
	int ff_num;
	unsigned long long ff_checksum;

	sprintf(sql,"select ff_num,ff_checksum,peer_name from peer where id=%d ;",peer_id);
	ret = m_db.query.query(sql,&affected_rows);
	if(0==ret)
	{
		if(0==m_db.query.next_row(&row))
		{
			ff_num = cl_util::atoi(row[0]);
			ff_checksum = cl_util::atoll(row[1]);
			//DEBUGMSG("# (%d,%d),(%lld,%lld) \n",seg.ff_num,ff_num ,seg.ff_checksum , ff_checksum);
			if(seg.ff_num == ff_num && seg.ff_checksum == ff_checksum)
				return 0;
			else
			{
				cly_report_error_t e;
				e.peer_name = row[2];
				e.err = CLY_TERR_CHSUM_DISTINCT;
				db_report_error(e);
				return CLY_TERR_CHSUM_DISTINCT;
			}
		}
		return CLY_TERR_NO_PEER;
	}
	return CLY_TERR_DB_WRONG;
}
int cly_dbmgr::db_reset_chsum(int peer_id,int ff_num,unsigned long long ff_checksum)
{
	int ret;
	char sql[1024];
	int affected_rows=0;

	sprintf(sql,"update peer set ff_num=%d,ff_checksum=%lld where id=%d ;",ff_num,ff_checksum,peer_id);
	ret = m_db.query.query(sql,&affected_rows);
	if(0==ret)
	{
		DEBUGMSG("# db_reset_chsum(pid=%d,ff_num=%d, ff_checksum=%lld) \n",peer_id,ff_num,ff_checksum);
		return 0;
	}
	return CLY_TERR_DB_WRONG;
}
//*************************************************************
int cly_dbmgr::get_last_id(int& id)
{
	int ret;
	char sql[1024];
	int affected_rows=0;
	char **row;
	int n = 0;

	sprintf(sql,"SELECT LAST_INSERT_ID();");
	ret = m_db.query.query(sql,&affected_rows);
	n = m_db.query.get_num_rows();
	if(0==ret&& n>0)
	{
		for(int i=0;i<n;++i)
		{
			if(0==m_db.query.next_row(&row))
				id = atoi(row[0]); 
		}
		return 0;
	}
	return CLY_TERR_DB_WRONG;
}
int cly_dbmgr::update_ff_checksum(int peer_id,const string& hash,bool bdel)
{
	int ret;
	char sql[1024];
	int affected_rows=0;
	char **row;

	sprintf(sql,"select ff_num,ff_checksum from peer where id=%d ;",peer_id);
	ret = m_db.query.query(sql,&affected_rows);
	if(0==ret && m_db.query.get_num_rows()>0)
	{
		if(0==m_db.query.next_row(&row))
		{
			unsigned int ff_num;
			unsigned long long ff_checksum;
			ff_num = cl_util::atoi(row[0]);
			ff_checksum = cl_util::atoll(row[1]);
			if(bdel)
			{
				ff_num--;
				ff_checksum -= cl_crc32_write(CL_CRC32_FIRST,(const unsigned char*)hash.c_str(),hash.length());
			}
			else
			{
				ff_num++;
				ff_checksum += cl_crc32_write(CL_CRC32_FIRST,(const unsigned char*)hash.c_str(),hash.length());
			}
			sprintf(sql,"update peer set ff_num=%d,ff_checksum=%lld where id=%d ;",ff_num,ff_checksum,peer_id);
			ret = m_db.query.query(sql,&affected_rows);
			if(0==ret)
				return 0;
		}
	}
	return CLY_TERR_DB_WRONG;
}
int cly_dbmgr::get_file_id(int& file_id,const string& hash)
{
	int ret;
	char sql[1024];
	int affected_rows=0;
	char **row;

	sprintf(sql,"select id from file where hash='%s';",hash.c_str());
	ret = m_db.query.query(sql,&affected_rows);
	if(0==ret && m_db.query.get_num_rows()>0)
	{
		if(0==m_db.query.next_row(&row))
		{
			file_id = atoi(row[0]);
			if(0!=file_id)
				return 0;
		}
	}
	return CLY_TERR_DB_WRONG;
}
int cly_dbmgr::get_peer_task_update_id(int peer_id,int& update_id,list<int>& ls_sync_peers)
{
	int ret;
	char sql[1024];
	int affected_rows=0;
	char **row;
	string str;
	int pid;

	sprintf(sql,"select task_update_id,sync_peer_ids from peer where id=%d;",peer_id);
	ret = m_db.query.query(sql,&affected_rows);
	if(0==ret && m_db.query.get_num_rows()>0)
	{
		if(0==m_db.query.next_row(&row))
		{
			update_id = atoi(row[0]);
			str = row[1];
			int n = cl_util::get_string_index_count(str,",");
			for(int i=0;i<n;++i)
			{
				pid = cl_util::atoi(cl_util::get_string_index(str,i,",").c_str());
				if(pid>0)
					ls_sync_peers.push_back(pid);
			}
			return 0;
		}
		return CLY_TERR_NO_PEER;
	}
	return CLY_TERR_DB_WRONG;
}
int cly_dbmgr::get_task_id(int& task_id,const string& name)
{
	int ret;
	char sql[1024];
	int affected_rows=0;
	char **row;

	sprintf(sql,"SELECT id FROM task WHERE NAME='%s';",name.c_str());
	ret = m_db.query.query(sql,&affected_rows);
	if(0!=ret)
		return CLY_TERR_DB_WRONG;

	if(0==m_db.query.next_row(&row))
	{
		task_id = atoi(row[0]);
		return 0;
	}
	return CLY_TERR_NO_TASK;
}

