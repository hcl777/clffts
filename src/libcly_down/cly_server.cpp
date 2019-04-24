#include "cly_server.h"
#include "cly_filemgr.h"
#include "cly_peerRecycling.h"
#include "cl_util.h"
#include "cl_net.h"
#include "cly_config.h"

cly_server::cly_server(void)
:m_max_peernum(100)
,m_thread_num(4)
{
}

cly_server::~cly_server(void)
{
}
int cly_server::init(int max_peernum,int thread_num)
{
	m_max_peernum = max_peernum<1?1:max_peernum;
	m_thread_num = thread_num;
	UAC_SocketSelectorSngl::instance()->set_acceptor(static_cast<UAC_SocketAcceptor*>(this));
	//cl_timerSngl::instance()->register_timer(static_cast<cl_timerHandler*>(this),2,1000);
	
	if (m_thread_num > 0)
	{
		m_queues = new cl_MessageQueue[m_thread_num];
		m_thread_ref = new int[m_thread_num];
		memset(m_thread_ref, 0, m_thread_num * sizeof(int));
		activate(m_thread_num);
	}
	DEBUGMSG("share thread num = %d \n",m_thread_num);
	return 0;
}

void cly_server::fini()
{
	UAC_SocketSelectorSngl::instance()->set_acceptor(NULL);
	//cl_timerSngl::instance()->unregister_all(static_cast<cl_timerHandler*>(this));
	while(!m_peers.empty())
		m_peers.front()->disconnect();

	if (m_thread_num > 0)
	{
		for (int i = 0; i < m_thread_num; ++i)
			m_queues[i].AddMessage(new cl_Message(2, NULL)); //退出消息
		wait();
		delete[] m_queues;
		delete[] m_thread_ref;
	}
}
void cly_server::stop_all()
{
	while (!m_peers.empty())
		m_peers.front()->disconnect();
}
void cly_server::on_delete_file(const string& hash)
{
	//关掉共享此文件的连接
	list<cly_peer*> ls;
	cly_peer* peer;
	cly_svrPeerData *data;
	list<cly_peer*>::iterator it;
	for(it=m_peers.begin();it!=m_peers.end();++it)
	{
		peer = *it;
		data = (cly_svrPeerData *)peer->get_peerdata();
		if(data->curr_ready_hash == hash || data->curr_down_hash==hash)
			ls.push_back(peer);
	}
	for(it=ls.begin();it!=ls.end();++it)
	{
		(*it)->disconnect();
	}
}
int cly_server::work(int e)
{
	cl_Message *msg = NULL;
	taskinfo_t *ti;
	//cl_memblock *block;
	//int size;
	//int n = 0;
	//ssize64_t pos;
	while(1)
	{
		msg = m_queues[e].GetMessage();
		if(NULL == msg) continue;
		if(1==msg->cmd)
		{
			ti = (taskinfo_t*)msg->data;
			read_block(ti->data, ti->index, ti->hash, ti->path);
			//if(ti->hash != ti->data->curr_ready_hash)
			//{
			//	ti->data->file.close();
			//	ti->data->fsize = 0;
			//	ti->data->curr_ready_hash = ti->hash;
			//	if(0==ti->data->file.open(ti->path.c_str(),F64_READ))
			//	{
			//		ti->data->fsize = ti->data->file.get_file_size();
			//		//DEBUGMSG("OPEN_READY(%d)= %s\n",e,ti->path.c_str());
			//	}
			//}
			////read block
			//size = cly_fileinfo::get_block_real_size(ti->data->fsize,ti->index);
			//block = cl_memblock::allot(CLY_FBLOCK_SIZE);
			//block->buflen = size;
			//pos = ti->index*(ssize64_t)CLY_FBLOCK_SIZE;
			//if(pos==ti->data->file.seek(pos,SEEK_SET))
			//{
			//	while(block->wpos<size)
			//	{
			//		if(0>=(n=ti->data->file.read(block->buf + block->wpos,size - block->wpos)))
			//			break;
			//		block->wpos += n;
			//	}
			//}
			//if(block && (block->rpos!=0||size!=block->wpos))
			//{
			//	block->release();
			//	block = NULL;
			//}
			////DEBUGMSG("READ_BLOCK_OK(t=%d) = %d \n",e,ti->index);
			//ti->data->update_block(ti->hash,ti->index,block);
			//ti->data->release();

			delete ti;
			delete msg;
		}
		else if(2==msg->cmd)
		{
			delete msg;
			break;
		}
	}
	return 0;
}
bool cly_server::uac_attach_socket(UAC_SOCKET fd,const UAC_sockaddr& addr)
{
	if(!CLY_IS_USED(cly_pc->used, CLY_USED_SHARE) ||  (int)m_peers.size()>=m_max_peernum)
		return false;
	cly_peer* peer = new cly_peer();
	peer->set_listener(static_cast<cly_peerListener*>(this));

	sockaddr_in sock_addr;
	memset(&sock_addr,0,sizeof(sock_addr));
	sock_addr.sin_port = htons(addr.port);
	sock_addr.sin_addr.s_addr = htonl(addr.ip);
	peer->get_channel()->attach(fd,sock_addr);
	peer->get_channel()->uac_set_alwayswrite(true);

	cly_svrPeerData *data = cly_svrPeerData::new_instance();
	peer->set_peerdata(data);
	m_peers.push_back(peer);

	//分配线程号,取任务最小的
	data->thread_i = 0;

	if(m_thread_num>0)
	{
		int n = m_thread_ref[0];
		int m;
		for(int i=1;i<m_thread_num;++i)
		{
			m = m_thread_ref[i];
			if(n>m)
			{
				n = m;
				data->thread_i = i;
			}
		}
		m_thread_ref[data->thread_i]++;
	}
	return true;
}
void cly_server::on(cly_peerListener::Connected,cly_peer* peer)
{
	//wait client request
}
void cly_server::on(cly_peerListener::Disconnected,cly_peer* peer)
{
	peer->set_listener(NULL);
	cly_svrPeerData* data = (cly_svrPeerData*)peer->get_peerdata();
	if(!data->curr_down_hash.empty())
		cly_filemgrSngl::instance()->release_file(data->curr_down_hash);
	if (m_thread_num>0)
		m_thread_ref[data->thread_i]--;
	data->del();
	m_peers.remove(peer);
	cly_peerRecyclingSngl::instance()->put_peer(peer);
}
void cly_server::on(cly_peerListener::Data,cly_peer* peer,char* buf,int len)
{
	cly_fileinfo* fi;
	clyp_head_t			head;
	cly_svrPeerData* data = (cly_svrPeerData*)peer->get_peerdata();
	cl_ptlstream ps(buf,len,len);
	if(0!=ps>>head)
		return;
	switch(head.cmd)
	{
	case CLYP_REQ_SUBTABLE:
		{
			clyp_rsp_subtable_t rsp;
			string name;
			ps >> name; //group_id:peer_name
			data->group_id = cl_util::atoi(cl_util::get_string_index(name,0,":").c_str());
			rsp.bitsize = 0;
			if(NULL==(fi=cly_filemgrSngl::instance()->get_readyinfo(head.hash)))
			{
				if(NULL==(fi=cly_filemgrSngl::instance()->get_downinfo(head.hash)))
				{
					rsp.finitype = -1;
				}
				else
				{
					rsp.finitype = 0;
					strcpy(rsp.subhash,fi->sh.subhash.c_str());
					rsp.bitsize = fi->sh.bt_ok.get_bitsize();
					memcpy(rsp.bt_buf,fi->sh.bt_ok.buffer(),(rsp.bitsize+7)>>3);
				}
			}	
			else
			{
				rsp.finitype = 1;
				strcpy(rsp.subhash,fi->sh.subhash.c_str());
			}
			cly_peer::send_ptl_packet(peer,CLYP_RSP_SUBTABLE,head.hash,rsp,1024);
		}
		break;
	case CLYP_REQ_BLOCKS:
		{
			//有数据的直接回复块数据,否则回复cancel
			unsigned int i=0;
			clyp_blocks_t req;
			clyp_blocks_t rsp;
			if(0!=ps>>req)
				break;
			rsp.num = 0;
			if(NULL==(fi=cly_filemgrSngl::instance()->get_readyinfo(head.hash)))
			{
				if(NULL==(fi=cly_filemgrSngl::instance()->get_downinfo(head.hash)))
				{
					memcpy(rsp.indexs,req.indexs,req.num*sizeof(uint32));
					rsp.num = req.num;
				}
				else
				{
					for(i=0;i<req.num;++i)
					{
						if(fi->bt_fini[req.indexs[i]])
							data->add_block(head.hash,req.indexs[i],req.offsets[i],false,"");
						else
							rsp.indexs[rsp.num++]=req.indexs[i];
					}
				}
			}	
			else
			{
				for(i=0;i<req.num;++i)
					data->add_block(head.hash,req.indexs[i],req.offsets[i],true,fi->fullpath);
			}
			if(rsp.num)
			{
				if(-1==cly_peer::send_ptl_packet(peer,CLYP_CANCEL_BLOCKS,head.hash,rsp,1024))
					break;
			}
			on(cly_peerListener::Writable(),peer);
		}
		break;
	case CLYP_CANCEL_BLOCKS:
		{
			unsigned int i=0;
			clyp_blocks_t req;
			if(0!=ps>>req)
				break;
			for(i=0;i<req.num;++i)
				data->remove_block(head.hash,req.indexs[i]);
		}
		break;
	default:
		DEBUGMSG("#*** cly_server:: unknow packet(cmd=%d)\n",(int)head.cmd);
		break;
	}
	assert(0==ps.length());
}
void cly_server::on(cly_peerListener::Writable,cly_peer* peer)
{
	clyp_block_t b;
	cly_svrPeerData* data = (cly_svrPeerData*)peer->get_peerdata();
	while(!data->bs.empty())
	{
		data->last_active_tick = GetTickCount();
		cly_svrBlockInfo_t& bi = data->bs.front();
		if(bi.state == CLY_SVR_BLOCK_INIT)
		{
			assert(!bi.block);
			if(!bi.bready)
			{
				//引用
				if(bi.hash != data->curr_down_hash)
				{
					if(!data->curr_down_hash.empty())
						cly_filemgrSngl::instance()->release_file(data->curr_down_hash);
					data->curr_down_hash = bi.hash;
					cly_filemgrSngl::instance()->refer_file(data->curr_down_hash);
				}
				bi.block = cly_filemgrSngl::instance()->read_block(bi.hash,bi.index);
				if(bi.block) bi.state = CLY_SVR_BLOCK_OK;
				else bi.state = CLY_SVR_BLOCK_FAIL;
			}
			else
			{	
				////先引用
				data->refer();
				bi.state = CLY_SVR_BLOCK_READING;
				//DEBUGMSG("ADD_READ_BLOCK1=%d\n",bi.index);
				if (m_thread_num>0)
					m_queues[data->thread_i].AddMessage(new cl_Message(1,new taskinfo_t(bi.index,bi.hash,bi.path,data))); 
				else
					read_block(data,bi.index, bi.hash, bi.path);
			}
		}
		//如果是ready,预读下一块
		if(data->bs.size()>1)
		{
			list<cly_svrBlockInfo_t>::iterator it=data->bs.begin();
			it++;
			cly_svrBlockInfo_t& bi2 = *it;
			if(bi2.bready && CLY_SVR_BLOCK_INIT==bi2.state)
			{
				data->refer();
				bi2.state = CLY_SVR_BLOCK_READING;
				//DEBUGMSG("ADD_READ_BLOCK2=%d\n",bi2.index);
				if (m_thread_num > 0)
					m_queues[data->thread_i].AddMessage(new cl_Message(1, new taskinfo_t(bi2.index, bi2.hash, bi2.path, data)));
				else
					read_block(data,bi2.index, bi2.hash, bi2.path);
			}
		}

		if(CLY_SVR_BLOCK_READING == bi.state)
		{
			//DEBUGMSG("wr.");
			break;
		}
		if(CLY_SVR_BLOCK_FAIL == bi.state)
		{
			//cancel
			clyp_blocks_t req;
			string hash = bi.hash;
			req.indexs[0] = bi.index;
			req.num = 1;
			data->pop_front_block();
			if(0!=cly_peer::send_ptl_packet(peer,CLYP_CANCEL_BLOCKS,hash.c_str(),req,1024))
				break;
			continue;
		}
		assert(CLY_SVR_BLOCK_OK==bi.state);
		while(1)
		{
			int btmp = bi.block->wpos - (int)bi.pos;
			//if((int)bi.pos>=bi.block->wpos)
			if(btmp<=0)
			{
				data->pop_front_block();
				break;
			}
			b.index = bi.index;
			b.offset = bi.pos;
			b.size = CL_MIN(CLYP_MAX_BLOCKDATA_SIZE - 100, btmp);
			b.pdata = bi.block->buf + bi.pos;
			bi.pos += b.size;
			if(0!=cly_peer::send_ptl_packet(peer,CLYP_RSP_BLOCK_DATA,bi.hash.c_str(),b,CLYP_MAX_BLOCKDATA_SIZE))
				return;
		}
	}
}
void cly_server::on_timer(int e)
{
}
int cly_server::get_allspeed(int& allspeed,list<string>& ls)
{
	char buf[1024];
	cly_peer* peer;
	cly_svrPeerData *data;
	UAC_sendspeed_t inf;
	string hash;
	allspeed = 0;
	int dataready = 0;
	int speed0 = 0, smallcache = 0, lowspeed = 0;
	ls.push_back("aspKB-setkb(lostR-rercvR-freecache-wind_pks/low/maxseg-dataready)(rercv,toresnd,resnd,allsnd),second_maxttl-|HASH|IP");
	for(list<cly_peer*>::iterator it=m_peers.begin();it!=m_peers.end();++it)
	{
		peer = *it;
		
		data = (cly_svrPeerData*)peer->get_peerdata();
		peer->get_channel()->uac_getsendspeed(inf);
		hash = data->curr_ready_hash;
		if(hash.empty()) hash = data->curr_down_hash;
		if(hash.empty())
			continue;
		allspeed += inf.speedB;
		if (data->bs.empty())
			dataready = 0;
		else if (data->bs.front().state == CLY_SVR_BLOCK_OK)
			dataready = 2;
		else
			dataready = 1;
		//总速度KB-（丢包率-重发误判率-接收端空缓冲），
		//（重收包量，超时重发量，重发量，所有发量),秒内最大ttl-|
		sprintf(buf,"%d-%d(.%d-.%d-%d-%d/%d/%d-%d),(%d-%d-%d-%d),%d-|%s|%s:%d"
			, (int)(inf.speedB >> 10),(int)(inf.setspeedB>>10), (inf.resend_pks-inf.other_rerecv_pks) * 100 / (inf.allsend_pks + 1)
			, inf.other_rerecv_pks*100/(inf.resend_pks+1), inf.other_wind_pks,inf.wind_pks,inf.low_line,inf.max_seg_num, dataready
			, inf.other_rerecv_pks, inf.resend_timeo_pks,inf.resend_pks,inf.allsend_pks
			, inf.second_maxttl_ms
			, hash.c_str(), cl_net::ip_htoas(peer->get_channel()->get_hip()).c_str(), (int)peer->get_channel()->get_hport()
		);
		ls.push_back(buf);
		if (0 == inf.speedB)speed0++;
		if (inf.other_wind_pks < 30) smallcache++;
		if (inf.speedB > 0 && inf.speedB < 30000) lowspeed++;
	}
	sprintf(buf, "ALL=%d|speed0=%d|smallcache=%d|lowspeed30KB=%d", (int)m_peers.size(),speed0, smallcache, lowspeed);
	ls.push_back(buf);
	return 0;
}
void cly_server::read_block(cly_svrPeerData* data, int index, const string& hash, const string& path)
{
	cl_memblock *block;
	int size;
	int n = 0;
	ssize64_t pos;

	if (hash != data->curr_ready_hash)
	{
		data->file.close();
		data->fsize = 0;
		data->curr_ready_hash = hash;
		if (0 == data->file.open(path.c_str(), F64_READ))
		{
			data->fsize = data->file.get_file_size();
			//DEBUGMSG("OPEN_READY(%d)= %s\n",e,path.c_str());
		}
	}
	//read block
	size = cly_fileinfo::get_block_real_size(data->fsize, index);
	block = cl_memblock::allot(CLY_FBLOCK_SIZE);
	block->buflen = size;
	pos = index*(ssize64_t)CLY_FBLOCK_SIZE;
	if (pos == data->file.seek(pos, SEEK_SET))
	{
		while (block->wpos<size)
		{
			if (0 >= (n = data->file.read(block->buf + block->wpos, size - block->wpos)))
				break;
			block->wpos += n;
		}
	}
	if (block && (block->rpos != 0 || size != block->wpos))
	{
		block->release();
		block = NULL;
	}
	//DEBUGMSG("READ_BLOCK_OK(t=%d) = %d \n",e,index);
	data->update_block(hash, index, block);
	data->release();
}
