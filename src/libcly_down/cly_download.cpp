#include "cly_download.h"
#include "cly_filemgr.h"
#include "cly_error.h"
#include "cly_cTracker.h"
#include "cly_config.h"

cly_download::cly_download(void)
	:m_fi(NULL)
	,m_tick(0)
	,m_cnn_create_speed(0)
	,m_finished_state(0)
{
	m_hpeer = new cl_httpcc(cl_reactorSngl::instance());
	m_hpeer->set_listener(this);
}


cly_download::~cly_download(void)
{
	m_hpeer->set_listener();
	delete[] m_hpeer;
}
int cly_download::create(const string& hash,int ftype,const string& path,bool bsave_original)
{
	if(NULL!=m_fi)
		return -1;
	m_fi = cly_filemgrSngl::instance()->create_downinfo(hash,ftype,path, bsave_original);
	if(NULL==m_fi)
		return -1;
	if(m_fi->bt_memfini.is_setall())
		m_finished_state = 1;
	DEBUGMSG("create download(%s,%d,%s,%d) \n",hash.c_str(),ftype,path.c_str(), bsave_original?1:0);

	m_downInfo.hash = hash;
	m_downInfo.ftype = m_fi->ftype;
	m_downInfo.path = m_fi->fullpath;
	m_downInfo.state = 1;
	cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),1,1000);
	cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),2,5000); //更新table
	cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),3,600000); //search source

	this->add_source_list(CLYSET->m_sn_list);

	cly_cTrackerSngl::instance()->search_source(hash);
	return 0;
}
void cly_download::close()
{
	if(NULL==m_fi)
		return;
	cl_timerSngl::instance()->unregister_all(static_cast<cl_timerHandler*>(this));
	m_hpeer->disconnect();
	while(!m_peers.empty())
		m_peers.front()->disconnect();
	m_fi = NULL;
	m_finished_state = 0;
	assert(m_blockRefs.empty());

	{
	for(map<uint64,cly_source_t*>::iterator it=m_sources.begin();it!=m_sources.end();++it)
		delete it->second;
	m_sources.clear();
	}
	{
	for(list<cly_source_t*>::iterator it=m_sources_free.begin();it!=m_sources_free.end();++it)
		delete (*it);
	m_sources_free.clear();
	}

	clear_pending();
}
int cly_download::add_source_list(list<string>& ls)
{
	//ip:port:nattype-private_ip:port-user_type
	uint32 ip,private_ip;
	uint16 port,private_port;
	int ntype,user_type;
	for(list<string>::iterator sit=ls.begin();sit!=ls.end();++sit)
	{
		const string& src = *sit;
		
		cly_soucre_str_section(src,ip,port,ntype,private_ip,private_port,user_type);
		if(ip&&port)
			add_source(src,ip,port,ntype,user_type);
		//相同公网IP的同时加私IP
		if(ip==CLYSET->m_uacaddr.ip && private_ip && private_port)
			add_source(src,private_ip,private_port,1,user_type);
	}
	return 0;
}
int cly_download::add_source(const string& s,uint32 ip,uint16 port,int ntype,int user_type)
{
	
	uint64 id = (((uint64)ip)<<32) + port;
	if(m_sources.find(id)!=m_sources.end())
		return 1;
	for(list<cly_source_t*>::iterator it=m_sources_free.begin();it!=m_sources_free.end();++it)
	{
		if((*it)->id == id)
			return 1;
	}
	DEBUGMSG("# cly_download::add_source(%s) \n",s.c_str());
	cly_source_t *ps = new cly_source_t();
	ps->id = id;
	ps->straddr = s;
	ps->ip = ip;
	ps->port = port;
	ps->ntype = ntype;
	ps->user_type = user_type;
	create_connect(ps);
	return 0;
}
int cly_download::get_downloadinfo(cly_downloadInfo_t& inf)
{
	m_downInfo.srcNum = m_sources.size() + m_sources_free.size();
	m_downInfo.connNum = m_sources.size();
	m_downInfo.speedB = (int)m_speed.get_speed(2);
	if(m_downInfo.size>0)
		m_downInfo.progress = m_fi->bt_memfini.get_setsize()*1000/ m_fi->bt_memfini.get_bitsize();
	inf = m_downInfo;
	if (inf.progress == 1000)
		inf.progress = 999; //下载中的不存在100%
	return 0;
}

int cly_download::create_connect(cly_source_t* s)
{
	if(0!=m_finished_state || m_cnn_create_speed>10)
	{
		m_sources_free.push_back(s);
		return -1;
	}
	cly_peer* peer = new cly_peer();
	peerData_t* data = new peerData_t();
	data->source_id = s->id;
	data->user_type = s->user_type;
	peer->set_peerdata(data);
	peer->set_listener(static_cast<cly_peerListener*>(this));

	s->last_use_tick = GetTickCount();
	if(0==peer->connect(s->ip,s->port,s->ntype))
	{
		m_cnn_create_speed++;
		m_peers.push_back(peer);
		m_sources[s->id] = s;
		return 0;
	}
	else
	{
		m_pending.push_back(peer);
		m_sources_free.push_back(s);
		return -1;
	}
}
void cly_download::clear_pending()
{
	for(list<cly_peer*>::iterator it=m_pending.begin();it!=m_pending.end();++it)
		delete (*it);
	m_pending.clear();
}
void cly_download::on(cly_peerListener::Connected,cly_peer* peer)
{
	((peerData_t*)peer->get_peerdata())->last_req_subtable_tick = GetTickCount();
	char buf[128];
	sprintf(buf,"%d:%s",cly_pc->group_id,cly_pc->peer_name.c_str());
	string s = buf;
	cly_peer::send_ptl_packet(peer,CLYP_REQ_SUBTABLE,m_fi->fhash.c_str(),s,1024);
}

void cly_download::on(cly_peerListener::Disconnected,cly_peer* peer)
{
	peerData_t *data = (peerData_t*)peer->get_peerdata();
	map<uint64,cly_source_t*>::iterator it=m_sources.find(data->source_id);
	if(it!=m_sources.end())
	{
		it->second->last_use_tick = GetTickCount();
		if(0==data->err)
			m_sources_free.push_back(it->second);
		else
			delete it->second;
		m_sources.erase(it);
	}
	for(list<blockInfo_t>::iterator it2=data->bis.begin();it2!=data->bis.end();++it2)
	{
		del_block_ref((*it2).index,peer);
		(*it2).block->release();
	}
	delete data;
	m_peers.remove(peer);
	m_pending.push_back(peer);
}

void cly_download::on(cly_peerListener::Data,cly_peer* peer,char* buf,int len)
{
	clyp_head_t			head;
	cl_ptlstream ps(buf,len,len);
	if(0!=ps>>head)
		return;
	if(m_fi->fhash != head.hash)
		return;
	switch(head.cmd)
	{
	case CLYP_RSP_SUBTABLE:
		on_rsp_subtable(peer,ps);
		break;
	case CLYP_CANCEL_BLOCKS:
		on_cancel_blocks(peer,ps);
		break;
	case CLYP_RSP_BLOCK_DATA:
		on_rsp_block_data(peer,ps);
		break;
	default:
		assert(0);
		break;
	}
	assert(0==ps.length());
}

void cly_download::on_timer(int e)
{
	peerData_t* data;
	cly_peer* peer;
	m_tick = GetTickCount();
	
	switch(e)
	{
	case 1:
		{
			m_speed.on_second();
			DEBUGMSG("#speed = %d kb \n",(int)(m_speed.get_speed(3)>>10));

			if(!m_pending.empty())
				clear_pending();

			if(0==m_finished_state)
			{
				//create connect
				m_cnn_create_speed = 0;
				unsigned int create_max = m_sources_free.size()>3?3:m_sources_free.size();
				cly_source_t* src;
				for(list<cly_source_t*>::iterator it=m_sources_free.begin();it!=m_sources_free.end() && m_cnn_create_speed<create_max;)
				{
					src = *it;
					if(_timer_after(m_tick,src->last_use_tick+30000))
					{
						m_sources_free.erase(it++);
						if(0!=create_connect(src))
							break;
					}
					else
						break;
				}

				//算速度
				for(list<cly_peer*>::iterator it=m_peers.begin();it!=m_peers.end();++it)
					((peerData_t*)(*it)->get_peerdata())->speed.on_second();
				
			}
			else if(1==m_finished_state)
			{
				if(!m_fi->bt_memfini.is_setall())
					m_finished_state = 0;
				else if(m_fi->sh.bt_ok.is_setall())
				{
					m_finished_state = 2;
					cly_filemgrSngl::instance()->on_file_done(m_fi->fhash);
				}
			}
		}
		break;
	case 2:
		//更新table
		if(0==m_finished_state)
		{
			for(list<cly_peer*>::iterator it=m_peers.begin();it!=m_peers.end();++it)
			{
				peer = *it;
				data = (peerData_t*)peer->get_peerdata();
				if(peer->get_channel()->get_state()==CL_CONNECTED)
				{
					if(!data->is_bt_fini && _timer_after(m_tick,data->last_req_subtable_tick+20000))
					{
						data->last_req_subtable_tick = m_tick;
						//发送失败会更新连表
						if(-1==cly_peer::send_ptl_packet(peer,CLYP_REQ_SUBTABLE,m_fi->fhash.c_str(),cly_pc->peer_name,1024))
							break;
					}
				}
			}	
		}
		break;
	case 3:
		if(0==m_finished_state)
		{
			cly_cTrackerSngl::instance()->search_source(m_fi->fhash);
		}
		break;
	default:
		assert(0);
		break;
	}
}
void cly_download::on_rsp_subtable(cly_peer* peer,cl_ptlstream& ps)
{	
	peerData_t* data = (peerData_t*)peer->get_peerdata();
	clyp_rsp_subtable_t rsp;
	if(0!=ps >> rsp)
		return;
	if(-1==rsp.finitype)
	{
		data->err = CLYE_CNN_NOFILE;
		peer->disconnect();
		return;
	}

	if(0!=m_fi->set_subhash(rsp.subhash))
	{
		data->err = CLYE_CNN_WRONG_SUBHASH;
		peer->disconnect();
		return;
	}
	m_downInfo.size = m_fi->sh.size;
	if(1==rsp.finitype)
	{
		data->bt.alloc(m_fi->sh.blocks);
		data->bt.setall();
	}
	else
	{
		if(rsp.bitsize!=m_fi->sh.blocks)
		{
			data->err = CLYE_CNN_WRONG_SUBHASH;
			peer->disconnect();
			return;
		}
		data->bt.alloc(rsp.bitsize,(unsigned char*)rsp.bt_buf);
	}
	if(data->bis.empty())
		assign_job(peer);
}

void cly_download::on_cancel_blocks(cly_peer* peer,cl_ptlstream& ps)
{
	list<blockInfo_t>::iterator it;
	peerData_t* data = (peerData_t*)peer->get_peerdata();
	clyp_blocks_t rsp;
	if(0!=ps >> rsp)
		return;
	DEBUGMSG("# cly_download::on_cancel_blocks(num = %d) \n",rsp.num);
	for(unsigned int i=0;i<rsp.num;++i)
	{
		for(it=data->bis.begin();it!=data->bis.end();++it)
		{
			blockInfo_t& bi = *it;
			if(bi.index == rsp.indexs[i])
			{
				del_block_ref(bi.index,peer);
				bi.block->release();
				data->bis.erase(it);
				break;
			}
		}
	}
	//中途获取数据失败断开连接
	if(data->bis.empty())
	{
		data->err = CLYE_CNN_NOFILE;
		peer->disconnect();
	}
}
void cly_download::on_rsp_block_data(cly_peer* peer,cl_ptlstream& ps)
{
	peerData_t* data = (peerData_t*)peer->get_peerdata();
	list<blockInfo_t>::iterator it;
	clyp_block_t rsp;
	if(0!=ps >> rsp)
		return;
	for(it=data->bis.begin();it!=data->bis.end();++it)
	{
		if((*it).index == rsp.index)
			break;
	}
	if(it==data->bis.end())
	{
		//刚取消任务，可能会收到过时数据的。
		return;
	}
	blockInfo_t bi = *it;
	data->speed.add(rsp.size);
	m_speed.add(rsp.size);
	m_fi->rcvB[data->user_type] += rsp.size;
	if(bi.block->write(rsp.pdata,rsp.size,rsp.offset)>0)
	{
		if(bi.block->wpos >= bi.block->buflen)
		{
			assert(bi.block->wpos == bi.block->buflen);
			m_fi->on_block_done(bi.index);
			bi.block->release();
			del_block_ref(bi.index,peer);
			data->bis.erase(it);
			cancel_job(bi.index);
			if(m_fi->bt_memfini.is_setall())
			{
				m_finished_state = 1;
				//关所有连接
				while(!m_peers.empty())
					m_peers.front()->disconnect();
			}
			else
			{
				//DEBUGMSG("task_num = %d\n",data->bis.size());
				this->assign_job(peer);
			}
		}
	}
}
int cly_download::assign_job(cly_peer* peer)
{
#define MAX_BLOCK_NUM 10
	peerData_t* data = (peerData_t*)peer->get_peerdata();
	unsigned int blocks = m_fi->bt_fini.get_bitsize();
	unsigned int i;
	list<unsigned int> ls;
	if(data->bis.size()>=MAX_BLOCK_NUM)
		return 0;
	unsigned int max_num = MAX_BLOCK_NUM - data->bis.size();

	//分配空闲块
	for(i=m_fi->block_gap;i<blocks && ls.size()<max_num;++i)
	{
		if(!m_fi->bt_memfini[i] && 0==get_block_ref_num(i) && data->bt[i/m_fi->sh.block_times] )
		{
			ls.push_back(i);
		}
	}

	//抢速度慢块,只抢1块，而且块速度小于1M的最慢块，并且自身速度要大于它2倍。
	if(data->bis.empty() && ls.empty())
	{
		int myspeed = (int)data->speed.get_speed(3);
		int tmp = 1000000,tmp2=0,min_index=-1;
		for(i=m_fi->block_gap;i<blocks && ls.empty();++i)
		{
			if(!m_fi->bt_memfini[i] && data->bt[i/m_fi->sh.block_times] )
			{
				tmp2 = get_block_ref_speed(i,3);
				//用等号可以使抢到更后面的空闲块。
				if(tmp2 <= tmp)
				{
					min_index = i;
					tmp = tmp2;
				}
			}
		}
		if(-1!=min_index && (myspeed<=0 || myspeed>2*tmp))
		{
			ls.push_back(min_index);
			DEBUGMSG("# rob slowly block(%d) \n",min_index);
		}
	}
	//
	if(!ls.empty())
	{
		blockInfo_t bi;
		clyp_blocks_t req;
		req.num = 0;
		for(list<unsigned int>::iterator it=ls.begin();it!=ls.end();++it)
		{
			i = *it;
			bi.index = i;
			bi.block = m_fi->get_download_block(i);
			if(bi.block && bi.block->wpos < bi.block->buflen)
			{
				data->bis.push_back(bi);
				add_block_ref(i,peer);
				req.indexs[req.num] = i;
				req.offsets[req.num] = bi.block->wpos;
				req.num++;
			}
		}
		ls.clear();
		return cly_peer::send_ptl_packet(peer,CLYP_REQ_BLOCKS,m_fi->fhash.c_str(),req,1024);
	}
	return 0;
}
void cly_download::cancel_job(unsigned int index)
{
	BlockRefIter it = m_blockRefs.find(index);
	peerData_t* data;
	cly_peer* peer;
	clyp_blocks_t req;
	req.num = 1;
	req.indexs[0] = index;
	if(it!=m_blockRefs.end())
	{
		list<cly_peer*> ls = it->second;
		for(list<cly_peer*>::iterator it2=ls.begin();it2!=ls.end();++it2)
		{
			peer = *it2;
			data = (peerData_t*)peer->get_peerdata();
			for(list<blockInfo_t>::iterator it3=data->bis.begin();it3!=data->bis.end();++it3)
			{
				if((*it3).index == index)
				{
					(*it3).block->release();
					data->bis.erase(it3);
					if(-1!=cly_peer::send_ptl_packet(peer,CLYP_CANCEL_BLOCKS,m_fi->fhash.c_str(),req,1024))
					{
						assign_job(peer);
					}
					break;
				}
			}
		}
		it->second.clear();
		m_blockRefs.erase(it);
	}
}
int cly_download::add_block_ref(int index,cly_peer* peer)
{
	m_blockRefs[index].push_back(peer);
	return 0;
}
int cly_download::del_block_ref(int index,cly_peer* peer)
{ 
	BlockRefIter it=m_blockRefs.find(index);
	if(it!=m_blockRefs.end())
	{
		it->second.remove(peer);
		if(it->second.empty())
			m_blockRefs.erase(it);
		return 0;
	}
	return -1;
}
int cly_download::get_block_ref_num(int index)
{
	BlockRefIter it=m_blockRefs.find(index);
	if(it==m_blockRefs.end())
		return 0;
	return (int)it->second.size();
}
int cly_download::get_block_ref_speed(int index,int seconds)
{
	peerData_t* data;
	int speed = 0,tmp=0;
	BlockRefIter it = m_blockRefs.find(index);
	if(it==m_blockRefs.end())
	{
		return 0;
	}
	list<cly_peer*>& ls = it->second;
	for(list<cly_peer*>::iterator it=ls.begin();it!=ls.end();++it)
	{
		data = (peerData_t*)(*it)->get_peerdata();
		//预分配未下载到的，直接当速度0
		if(data->bis.front().index!=(unsigned int)index)
			tmp = 0;
		else
			tmp = (int)data->speed.get_speed(5);
		if(speed<tmp)
			speed = tmp;
	}
	return speed;
}

