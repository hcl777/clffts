#include "cly_source.h"
#include "cl_util.h"
#include "cl_crc32.h"
#include "cly_setting.h"

//一维下标:自身的nat类型，二维下标:对方nat类型 
//7表未知类型,自己是6的,当自己是3类型，即只返回0，1，2，3
static char g_ConnectionType[7][7] = 
{
	{1,1,1,1,1,1,0},
	{1,1,1,1,1,1,0},
	{1,1,1,1,1,0,0},
	{1,1,1,1,0,0,0},
	{1,1,1,0,0,0,0},
	{1,1,0,0,0,0,0},
	{1,1,1,1,0,0,0}
};
cly_source::cly_source(void)
{
	last_i_svr = 0;
}

cly_source::~cly_source(void)
{
	clear();
}

void cly_source::clear()
{
	{
		for(clyt_FileIter it=m_files.begin();it!=m_files.end();++it)
			delete it->second;
		m_files.clear();
	}
	{
		for(clyt_PeerIter it=m_peers.begin();it!=m_peers.end();++it)
			delete it->second;
		m_peers.clear();
		m_hidpeers.clear();
	}
	{
		for(clyt_SourceIter it=m_srcs.begin();it!=m_srcs.end();++it)
			delete it->second;
		m_srcs.clear();
	}
}


clyt_file_t* cly_source::get_file(int file_id)
{
	clyt_FileIter it=m_files.find(file_id);
	if(it!=m_files.end())
		return it->second;
	return NULL;
}

clyt_peer_t* cly_source::get_peer(int peer_id)
{
	clyt_PeerIter it=m_peers.find(peer_id);
	if(it!=m_peers.end())
		return it->second;
	return NULL;
}
clyt_peer_t* cly_source::get_peer(const string& peer_name)
{
	clyt_PeerHidIter it=m_hidpeers.find(peer_name);
	if(it!=m_hidpeers.end())
		return it->second;
	return NULL;
}
clyt_source_t* cly_source::get_source(int file_id)
{
	clyt_file_t* file = get_file(file_id);
	if(NULL==file) return NULL;
	return get_source(file->hash);
}
clyt_source_t* cly_source::get_source(const string& hash)
{
	if(hash.empty())
		return NULL;
	clyt_SourceIter it=m_srcs.find(hash);
	if(it!=m_srcs.end())
		return it->second;
	return NULL;
}
clyt_progress_t* cly_source::get_pg(list<clyt_progress_t>& ls,int peer_id)
{
	for(list<clyt_progress_t>::iterator it=ls.begin();it!=ls.end();++it)
	{
		if((*it).peer->peer_id == peer_id)
			return &(*it);
	}
	return NULL;
}
void cly_source::del_pg(list<clyt_progress_t>& ls,int peer_id,clyt_file_t* file)
{
	for(list<clyt_progress_t>::iterator it=ls.begin();it!=ls.end();++it)
	{
		if((*it).peer->peer_id == peer_id)
		{
			if((*it).progress==1000)
				(*it).peer->fini_files.remove(file->file_id);
			ls.erase(it);
			return;
		}
	}
}

int cly_source::get_all_peerid(list<int>& ls)
{
	for(clyt_PeerIter it=m_peers.begin();it!=m_peers.end();++it)
	{
		ls.push_back(it->second->peer_id);
	}
	return 0;
}
int cly_source::get_peerids_by_hids(list<string>& ls_hids,list<int>& ls_peerid)
{
	clyt_PeerHidIter hidit;
	for(list<string>::iterator it=ls_hids.begin();it!=ls_hids.end();++it)
	{
		hidit = m_hidpeers.find(*it);
		if(hidit!=m_hidpeers.end())
			ls_peerid.push_back(hidit->second->peer_id);
	}
	return 0;
}
int cly_source::get_fileids_by_hashs(list<string>& ls_hash,list<int>& ls_fids)
{
	clyt_SourceIter sit;
	for(list<string>::iterator it=ls_hash.begin();it!=ls_hash.end();++it)
	{
		sit = m_srcs.find(*it);
		if(sit!=m_srcs.end())
			ls_fids.push_back(sit->second->file->file_id);
	}
	return 0;
}
int cly_source::src_login(int peer_id,const string& peer_name,int peer_type,int group_id)
{
	//不存在就创建
	clyt_peer_t* peer = get_peer(peer_id);
	if(!peer)
	{
		peer = new clyt_peer_t();
		peer->peer_id = peer_id;
		peer->fini_chsum = 0;
		peer->peer_name = peer_name;
		peer->nattype = 6;
		m_peers[peer_id] = peer;
		m_hidpeers[peer->peer_name] = peer;
	}
	assert(peer_name == peer->peer_name);
	peer->is_login = true;
	peer->keepalive_count = 0;
	peer->group_id = group_id;
	if(peer->peer_type!=peer_type)
		change_peer_type(peer,peer_type);
	peer->last_tick =  GetTickCount();
	peer->last_time = cl_util::time_to_datetime_string(time(NULL));
	return 0;
}
int cly_source::src_update_group_id(int peer_id,int group_id)
{
	clyt_peer_t* peer = get_peer(peer_id);
	if(peer) peer->group_id = group_id;
	return 0;
}
int cly_source::src_keepalive(int& peer_id,const string& peer_name)
{
	clyt_peer_t* peer;
	if(!peer_name.empty())
	{
		peer = get_peer(peer_name);
		if(peer) 
			peer_id = peer->peer_id;
	}
	else
		peer = get_peer(peer_id);
	if(peer && peer->is_login)
	{
		peer->last_tick = GetTickCount();
		peer->last_time = cl_util::time_to_datetime_string(time(NULL));
		return 0;
	}
	return CLY_TERR_NO_PEER;
}
void cly_source::src_update_nat(int peer_id,const string& addr,int nattype)
{
	clyt_peer_t* peer = get_peer(peer_id);
	char buf[128];
	if(peer)
	{
		peer->last_tick = GetTickCount();
		peer->last_time = cl_util::time_to_datetime_string(time(NULL));
		peer->nattype = nattype;
		if(peer->nattype<0||peer->nattype>6)  peer->nattype=6;
		sprintf(buf,"%s-%d",addr.c_str(),peer->peer_type);
		peer->addr = buf;
	}
}

int cly_source::src_finifile(clyt_peer_t* peer,int file_id,const string& hash,const string& suhash,const string& name)
{
	//file
	clyt_file_t* file = get_file(file_id);
	if(!file)
	{
		file = new clyt_file_t();
		file->file_id = file_id;
		file->hash = hash;
		file->subhash = hash;
		file->name = name;
		m_files[file->file_id] = file;
	}

	//soure
	clyt_source_t* src = get_source(hash);
	if(!src)
	{
		src = new clyt_source_t();
		src->file = file;
		src->last_i_peer = 0;
		m_srcs[file->hash] = src;
	}

	clyt_progress_t* ppg = NULL;
	ppg = get_pg(src->ls_peer,peer->peer_id);
	if(NULL==ppg)
	{
		clyt_progress_t pg;
		pg.peer = peer;
		pg.progress = 1000;
		src->ls_peer.push_back(pg);
	}
	else
	{
		ppg->progress = 1000;
	}
	peer->add_fini_file(file);
	return 0;
}
int cly_source::src_update_filename(const string& hash,const string& name)
{
	clyt_source_t* src = get_source(hash);
	if(!src)
		return CLY_TERR_NO_FILE;
	src->file->name = name;
	return 0;
}
int cly_source::src_get_fileinfo(const string& hash,clyt_fileinfo_t& fi)
{
	fi.file_id = 0;
	fi.peer_num = 0;
	clyt_source_t* src = get_source(hash);
	if(!src)
		return CLY_TERR_NO_FILE;
	fi.file_id = src->file->file_id;
	fi.hash = src->file->hash;
	fi.name = src->file->name;
	fi.subhash = src->file->subhash;
	fi.peer_num = src->ls_peer.size();
	return 0;
}
int cly_source::src_delfile(int peer_id,int file_id)
{
	//永不删除file
	int id;
	clyt_file_t* file = get_file(file_id);
	if(!file) return -1;
	clyt_peer_t* peer = get_peer(peer_id);
	if(!peer) return -1;
	return src_delfile(peer,file->hash,id);
}
int cly_source::src_delfile(clyt_peer_t* peer,const string& hash,int& file_id)
{
	clyt_source_t* src = get_source(hash);
	if(src)
	{
		del_pg(src->ls_peer,peer->peer_id,src->file);
		file_id = src->file->file_id;
		DEBUGMSG("# src_delfile(pid=%d,hash=%s,file_id=%d)\n",peer->peer_id,hash.c_str(),file_id);
		return 0;
	}
	else
	{
		DEBUGMSG("# **** src_delfile(pid=%d,hash=%s) fail!\n",peer->peer_id,hash.c_str());
	}
	return -1;
}
int cly_source::src_update_progress(int peer_id,const string& hash,int progress)
{
	clyt_peer_t* peer = get_peer(peer_id);
	if(!peer) return -1;
	clyt_source_t* src = get_source(hash);
	if(!src) return -1;

	clyt_progress_t* ppg = NULL;
	ppg = get_pg(src->ls_peer,peer->peer_id);

	if(NULL==ppg)
	{
		clyt_progress_t pg;
		pg.peer = peer;
		pg.progress = progress;
		src->ls_peer.push_back(pg);
	}
	else
	{
		ppg->progress = progress;
	}
	return 0;
}
void cly_source::get_source_from_svr(int file_id,list<string>& srcls,clyt_peer_t* peer,list<clyt_peer_t*>& ls,int& index,int maxnum)
{
	//只搜索client 类型
	DWORD tick = GetTickCount();
	list<clyt_peer_t*>::iterator it;
	int i = 0,n=0;
	if(index >= (int)ls.size())
		index = 0;
	//跳到搜索开始位置
	for(it=ls.begin(),i=0;it!=ls.end()&&i<index;++it,++i);
	for(;it!=ls.end() && n<maxnum;++it,++i)
	{
		clyt_peer_t* p = *it;
		if(peer!=p && _timer_after(p->last_tick + 600000,tick) 
			&& g_ConnectionType[peer->nattype][p->nattype]
			&& p->fini_files.find(file_id)!=p->fini_files.end())
		{
			srcls.push_back(p->addr);
			n++;
		}
	}
	if(n<maxnum)
	{
		for(it=ls.begin(),i=0;it!=ls.end() && i<index && n<maxnum;++it,++i)
		{
			clyt_peer_t* p = *it;
			if(peer!=p && _timer_after(p->last_tick + 600000,tick) 
			&& g_ConnectionType[peer->nattype][p->nattype]
			&& p->fini_files.find(file_id)!=p->fini_files.end())
			{
				srcls.push_back(p->addr);
				n++;
			}
		}
	}
	index = i;
}
void cly_source::get_source_from_client(list<string>& srcls,clyt_peer_t* peer,list<clyt_progress_t>& ls,int& index,int maxnum)
{
	//只搜索client 类型
	DWORD tick = GetTickCount();
	list<clyt_progress_t>::iterator it;
	int i = 0,n=0;
	if(index >= (int)ls.size())
		index = 0;
	//跳到搜索开始位置
	for(it=ls.begin(),i=0;it!=ls.end()&&i<index;++it,++i);
	for(;it!=ls.end() && n<maxnum;++it,++i)
	{
		clyt_progress_t& pg = *it;
		if(pg.peer->peer_type != CLY_PEER_TYPE_CLIENT)
			continue;
		if(peer!=pg.peer && pg.progress>100 && _timer_after(pg.peer->last_tick + 600000,tick) && g_ConnectionType[peer->nattype][pg.peer->nattype])
		{
			srcls.push_back(pg.peer->addr);
			n++;
		}
	}
	if(n<maxnum)
	{
		for(it=ls.begin(),i=0;it!=ls.end() && i<index && n<maxnum;++it,++i)
		{
			clyt_progress_t& pg = *it;
			if(pg.peer->peer_type != CLY_PEER_TYPE_CLIENT)
				continue;
			if(peer!=pg.peer && pg.progress>100 && _timer_after(pg.peer->last_tick + 600000,tick) && g_ConnectionType[peer->nattype][pg.peer->nattype])
			{
				srcls.push_back(pg.peer->addr);
				n++;
			}
		}
	}
	index = i;
}


int cly_source::src_search_source(int peer_id,const string& hash,list<string>& ls)
{
	clyt_peer_t* peer = get_peer(peer_id);
	if(!peer) return CLY_TERR_NO_PEER;
	clyt_source_t* src = get_source(hash);
	if(!src) return CLY_TERR_NO_SOURCE;
	get_source_from_svr(src->file->file_id,ls,peer,m_svr_peers,last_i_svr,2);
	get_source_from_client(ls,peer,src->ls_peer,src->last_i_peer,20);
	return 0;
}
int cly_source::src_save_peer_finifiles(int peer_id,const string& path)
{
	char buf[2048];
	clyt_file_t *file;
	clyt_peer_t* peer = get_peer(peer_id);
	if(!peer)
		return CLY_TERR_NO_PEER;
	FILE *fp = fopen(path.c_str(),"wb+");
	if(!fp)
		return CLY_TERR_WRITE_FILE_FAILED;
	//前两行为 peer_name|peer_id ; hash|name|subhash
	sprintf(buf,"%s|%d\r\nhash|name|subhash\r\n",peer->peer_name.c_str(),peer->peer_id);
	fputs(buf,fp);
	for(map<int,clyt_file_t*>::iterator it=peer->fini_files.begin();it!=peer->fini_files.end();++it)
	{
		file = it->second;
		sprintf(buf,"%s|%s|%s\r\n",file->hash.c_str(),file->name.c_str(),file->subhash.c_str());
		fputs(buf,fp);
	}

	fclose(fp);
	return 0;
}
int cly_source::src_get_state(clyt_state_t& st)
{
	char buf[1024];
	clyt_peer_t* p;
	int i=0;
	st.begin_time = cly_settingSngl::instance()->get_begin_time();
	st.peer_num = m_peers.size();
	st.file_num = m_files.size();
	st.svr_num = m_svr_peers.size();
	for(clyt_PeerIter it=m_peers.begin();it!=m_peers.end();++it)
	{
		p = it->second;
		sprintf(buf,"%s|%s|%s",p->peer_name.c_str(),p->addr.c_str(),p->last_time.c_str());
		st.peers.push_back(buf);
		//最
		if(++i>=1000)
			break;
	}
	return 0;
}
int cly_source::src_get_peer_state(const string& peer_name,clyt_peer_t& p)
{
	clyt_peer_t* peer = get_peer(peer_name);
	if(!peer)
		return CLY_TERR_NO_PEER;
	p.addr = peer->addr;
	p.nattype = peer->nattype;
	p.peer_id = peer->peer_id;
	p.peer_name = peer->peer_name;
	p.peer_type = peer->peer_type;
	p.last_tick = peer->last_tick;
	p.last_time = peer->last_time;
	p.fini_num = peer->fini_files.size();
	p.fini_chsum = 0;
	for(map<int,clyt_file_t*>::iterator it=peer->fini_files.begin();it!=peer->fini_files.end();++it)
	{
		string& hash = it->second->hash;
		p.fini_chsum += cl_crc32_write(CL_CRC32_FIRST,(const unsigned char*)hash.c_str(),hash.length());
	}
	return 0;
}
void cly_source::change_peer_type(clyt_peer_t* peer,int type)
{
	if(type == peer->peer_type)
		return;
	if(CLY_PEER_TYPE_CLIENT == type)
		m_svr_peers.remove(peer);
	else if(CLY_PEER_TYPE_CLIENT == peer->peer_type)
		m_svr_peers.push_back(peer);

	peer->peer_type = type;
	//要变更地址，变更
	int pos = (int)peer->addr.rfind('-');
	if(pos>0)
	{
		char buf[1024];
		sprintf(buf,"%s-%d",peer->addr.substr(0,pos).c_str(),type);
		peer->addr = buf;
	}
}
int cly_source::src_get_server(list<string>& ls)
{
	char buf[1024];
	clyt_peer_t* p;
	int i = 0;
	for (list<clyt_peer_t*>::iterator it = m_svr_peers.begin(); it != m_svr_peers.end(); ++it)
	{
		p = *it;
		sprintf(buf, "http://%s:9880/", cl_util::get_string_index(p->addr, 0, ":").c_str());
		ls.push_back(buf);
		//最
		if (++i >= 1000)
			break;
	}
	return 0;
}
