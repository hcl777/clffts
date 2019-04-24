#include "cly_fileinfo.h"
//#include "cly_filemgr.h"
#include "cl_fhash.h"
#include "cl_util.h"
#include "cly_config.h"
#include "cl_checksum.h"
#include "cl_crc32.h"
#include "cly_error.h"
#include "cly_jni.h"


//***************************************************
//class cly_subhash
int cly_subhash::load_strhash(const string& strhash)
{
	string hash,crc;
	subhash = strhash;
	hash = cl_util::get_string_index(strhash,0,"#");
	//校验
	unsigned int ncrc = atoi(cl_util::get_string_index(strhash,1,"#").c_str());
	unsigned int nhash = cl_crc32_write(CL_CRC32_FIRST,(unsigned char*)hash.c_str(),hash.length());
	nhash &= 0x7fffffff;
	if(hash.empty()||ncrc!=nhash)
		return -1;

	size = cl_util::atoll(cl_util::get_string_index(hash,0,"+").c_str());
	subsize = atoi(cl_util::get_string_index(hash,1,"+").c_str());
	string str = cl_util::get_string_index(hash,1,":");
	if(0==size ||0==subsize ||0!=subsize%CLY_FBLOCK_SIZE)
	{
		reset();
		return -1;
	}
	blocks = (int)((size-1)/subsize+1);
	if((int)str.length()!=blocks*5)
	{
		reset();
		return -1;
	}
	block_times = subsize/CLY_FBLOCK_SIZE;
	const char* p = str.c_str();
	h = new unsigned int[blocks];
	fini = new unsigned int[blocks];
	for(int i=0;i<blocks;++i)
	{
		h[i] = cl_fhash_symbol_to_u32(p+i*5);
		fini[i] = 0;
	}
	return 0;
}
void cly_subhash::reset()
{
	subhash = "";
	subsize = 0;
	size = 0;
	blocks = 0;
	block_times = 0;
	if(h) 
	{
		delete[] h;
		h = NULL;
		delete[] fini;
		fini = NULL;
	}
}

//***************************************************
cly_fileinfo::cly_fileinfo(void)
:ref(0)
,ftype(FTYPE_VOD)
,bready(false)
//,begin_block(0)
//,blocks(0)
,block_gap(0)
,req_offset(0)
,last_req_offset(0)
, bsave_original(false)
,no(0)
{
	memset(rcvB,0,sizeof(size64_t)*5);
}

cly_fileinfo::~cly_fileinfo(void)
{
	FileBlockMapIter it;
	for(it=fbmp.begin();it!=fbmp.end();++it)
		it->second->release();
	fbmp.clear();
}

bool cly_fileinfo::is_write_disk() const
{
	return (FTYPE_VOD!=ftype || cly_pc->cache_vod_disk);
}

bool cly_fileinfo::open_file(int mode,int rdbftype/*=RDBF_AUTO*/)
{
	if(_file.is_open())
		return true;
	//不保存硬盘的，只要新创建文件的才会不实际打开文件
	//因为有可能要读共享数据
	if((mode&F64_TRUN) && !is_write_disk())
		return true;
	return (0==_file.open(fullpath.c_str(),mode,rdbftype));
}
void cly_fileinfo::close_file()
{
	_file.close();
}

int cly_fileinfo::write_block_to_file(unsigned int index)
{
	int ret=-1,n;
	cl_memblock *block;
	assert(!bt_fini[index]);
	FileBlockMapIter it = fbmp.find(index);
	if(it==fbmp.end())
	{
		assert(0);
		return -1;
	}
	block = it->second;
	if(!block)
		return -1;
	//点播不cache到硬盘的不写
	if(is_write_disk())
	{
		if(open_file(F64_RDWR))
		{
			int len = block->length();
			char *p = block->read_ptr();
			ssize64_t pos = index*(ssize64_t)CLY_FBLOCK_SIZE + block->rpos;
			if(pos==_file.seek(pos,SEEK_SET))
			{
				while(len>0)
				{
					if(0>=(n=_file.write(p,len)))
						break;
					len -= n;
					p += n;
				}
				//_file.flush(); //影响性能,改为上层周期flush
			}
			if(len>0)
			{
				//上报错误
			}
			else
				ret = 0;
			//写入文件的就清除
			block->release(); //free未一定删除(引用减)
			fbmp.erase(it); 
			bt_fini.set(index);
			for(;block_gap<(unsigned int)bt_fini.get_bitsize() && bt_fini[block_gap];++block_gap);
		}
	}
	else
	{
		ret = 0;
		//尝试清理缓存
		try_free_memcache();
	}

	return ret;
}
unsigned int cly_fileinfo::get_block_real_size(size64_t fsize,unsigned int index)
{
	if(UINT64_INFINITE==fsize)
		return CLY_FBLOCK_SIZE;
	unsigned int n = (int)((fsize-1)/CLY_FBLOCK_SIZE + 1);
	assert(index < n);
	if (index < n-1)
		return CLY_FBLOCK_SIZE;
	else
		return (int)(fsize - CLY_FBLOCK_SIZE * (size64_t)index);
}
//bool cly_fileinfo::is_allow_pause()
//{
//	return !is_write_disk();
//}
void cly_fileinfo::try_free_memcache()
{
	//删除窗口以外的.
	unsigned int need_i = (unsigned int)(req_offset/CLY_FBLOCK_SIZE);
	int win = cly_pc->cache_win_num;
	for(FileBlockMapIter it=fbmp.begin();(int)fbmp.size()>win && it!=fbmp.end();)
	{
		if(it->first<need_i || it->first>=(need_i+win))
		{
			if(!bt_fini[it->first])
				bt_memfini.set(it->first,false);
			it->second->release(); 
			fbmp.erase(it); 
		}
		else
		{
			++it;
		}
	}
}
cl_memblock* cly_fileinfo::get_download_block(unsigned int index)
{
	cl_TLock<Mutex> l(m_mt);
	cl_memblock* block=NULL;
	FileBlockMapIter it = fbmp.find(index);
	if(it!=fbmp.end())
		block=it->second;
	else
	{
		block = cl_memblock::allot(CLY_FBLOCK_SIZE);
		if(block)
		{
			block->buflen = get_block_real_size(sh.size,index); //借用字段来保存真实块大小
			fbmp[index] = block;
		}
	}
	if(block)
		block->refer();
	return block;
}
//int cly_fileinfo::write_block_data(unsigned int index,int len,int offset,const char* buf)
//{
//	FileBlockMapIter it = fbmp.find(index);
//	if(it!=fbmp.end())
//		return it->second->write(buf,len,offset);
//	cl_memblock *block = cl_memblock::allot(CLY_FBLOCK_SIZE);
//	if(block)
//	{
//		fbmp[index] = block;
//		return block->write(buf,len,offset);
//	}
//	return 0;
//}
cl_memblock* cly_fileinfo::read_block(unsigned int index)
{
	cl_TLock<Mutex> l(m_mt);
	//缓冲中的block的offset一定是从0开始的
	cl_memblock* block=NULL;
	int size = get_block_real_size(sh.size,index);
	FileBlockMapIter it = fbmp.find(index);
	if(it!=fbmp.end())
	{
		block=it->second;
		block->refer();
	}
	else
	{
		//从文件中读,不缓冲到内存
		if(!bready && !bt_fini[index])
			return NULL;
		block = cl_memblock::allot(CLY_FBLOCK_SIZE);
		if(!block) return NULL;
		block->buflen = size;
		int n = 0;
		ssize64_t pos = index*(ssize64_t)CLY_FBLOCK_SIZE;
		if(open_file(F64_RDWR))
		{
			if(pos==_file.seek(pos,SEEK_SET))
			{
				while(block->wpos<size)
				{
					if(0>=(n=_file.read(block->buf + block->wpos,size - block->wpos)))
						break;
					block->wpos += n;
				}
			}
		}
		else
		{
			DEBUGMSG("# ***read block() open file fail! \n");
		}
	}
	if(block && (block->rpos!=0||size!=block->wpos))
	{
		block->release();
		block = NULL;
	}
	return block;
}
int cly_fileinfo::read_data(ssize64_t pos,char* buf,int size)
{
	cl_TLock<Mutex> l(m_mt);
	int n,m,i,readsize;
	cl_memblock* block=NULL;
	FileBlockMapIter it;
	readsize = 0;
	while(size>0)
	{
		n = (int)(pos/CLY_FBLOCK_SIZE);
		it = fbmp.find(n);
		if(it!=fbmp.end())
		{
			block = it->second;
			m = (int)(pos%CLY_FBLOCK_SIZE);
			if(m>=block->wpos)
				break;
			i = block->wpos - m;
			if(i>size) i = size;
			memcpy(buf+readsize,block->buf+m,i);
			readsize += i;
			pos += i;
			size -= i;
			if(block->wpos != block->buflen)
				break; //块不完整则退出
		}
		else
		{
			//文件中读
			if(!bready && !bt_fini[n])
				break;
			if(open_file(F64_RDWR))
			{
				if(pos==_file.seek(pos,SEEK_SET))
				{
					i = CLY_FBLOCK_SIZE;
					if(i>size) i=size;
					i = _file.read(buf+readsize,i);
					if(i<=0)
						break;
					readsize += i;
					pos += i;
					size -= i;
				}
				else
					break;
			}
			else
			{
				break;
			}
		}
	}
	return readsize;
}

int cly_fileinfo::set_subhash(const string& subhash)
{
	if(sh.size==0)
	{
		sh.load_strhash(subhash);
		if(sh.size>0)
		{
			int n = (int)((sh.size-1)/CLY_FBLOCK_SIZE+1);
			bt_fini.alloc(n);
			bt_memfini.alloc(n);
			sh.bt_ok.alloc(sh.blocks);
			int rdbftype = cly_pc->down_encrypt ? RDBF_AUTO : RDBF_BASE;
			if (bsave_original) rdbftype = RDBF_BASE;
			open_file(F64_RDWR|F64_TRUN, rdbftype);
			if(is_write_disk())
			{
				save_downinfo_i(this);
#ifndef ANDROID
				//预分配文件空间大小
				_file.resize(sh.size);
#endif
			}
		}
	}
	if(sh.size==0||sh.subhash != subhash)
		return -1;
	return 0;
}
int cly_fileinfo::on_block_done(unsigned int index)
{
	cl_TLock<Mutex> l(m_mt);
	assert(!bt_fini[index]);
	int n = index/sh.block_times;
	bt_memfini.set(index);
	write_block_to_file(index);
	if(bt_fini[index])
	{
		sh.fini[n]++;
		assert(!sh.bt_ok[n]);
		check_sub_done(n);//检查子hash
		//不再更新信息文件，android反复刷写文件容易出错
		//if(bt_fini.get_setsize()%21==0)
		//	save_downinfo_i(this);
	}
	 
	//DEBUGMSG("# on block done %d \n",index);
	return 0;
}
void cly_fileinfo::check_sub_done(int n)
{
	if(sh.bt_ok[n])
		return ;
	unsigned int bsize = sh.block_times;
	if(n == sh.blocks-1)
		bsize = bt_fini.get_bitsize() - n*sh.block_times;

	if(sh.fini[n] == bsize)
	{
		//sub block 完成
		cly_checkhash_info_t inf;
		inf.hash = fhash;
		inf.index = n;
		inf.pos = n*(size64_t)sh.subsize;
		inf.size = sh.subsize;
		if(n==sh.blocks-1)
			inf.size = sh.size-inf.pos;
		cly_checkhashSngl::instance()->add_check(inf);
	}
	else if(sh.fini[n]>bsize)
	{
		assert(0);
	}
}
void cly_fileinfo::on_check_subhash_result(const cly_checkhash_info_t& inf)
{
	string file_hash;
	int wrong_index = -1;
	{
		cl_TLock<Mutex> l(m_mt);
		if (sh.h[inf.index] == inf.res_subhash)
		{
			sh.bt_ok.set(inf.index);
			DEBUGMSG("#---subblock check ok (%d) \n", inf.index);
		}
		else
		{
			file_hash = fhash;
			wrong_index = inf.index;
			DEBUGMSG("#*******subblock check faild (%d) !!! \n", inf.index);
			int min, max;
			char errmsg[1024];
			min = inf.index * sh.block_times;
			max = (inf.index + 1) * sh.block_times;
			if (max > bt_fini.get_bitsize())
				max = bt_fini.get_bitsize();
			for (int i = min; i < max; ++i)
			{
				bt_fini.set(i, false);
				bt_memfini.set(i, false);
			}
			if ((int)block_gap > min)
				block_gap = min;
			sh.fini[inf.index] = 0;

			sprintf(errmsg, "%s_%d", fhash.c_str(), inf.index);
			//上报错误
			cly_error_report(CLYE_DOWNBLOCK_WRONG, errmsg);
		}
		//cl_bittable::print(bt_fini);
		save_downinfo_i(this);
	}
	if (-1 != wrong_index)
		jni_fire_download_wrong(file_hash, wrong_index);
}
void cly_fileinfo::delete_infofile()
{
	cl_file64::remove_file((info_path+"2").c_str());
	cl_file64::remove_file(info_path.c_str());
}
cly_fileinfo *cly_fileinfo::load_downinfo_i(const string& name,const string& fullpath)
{
	//注意:传入的是相对路径和全路径，如果全路径不存在就相对路径找.
	cly_fileinfo* fi = NULL;
	string infopath;
	string real_fullpath = fullpath;
	if(0==cl_util::file_state(real_fullpath.c_str()))
		real_fullpath = CLYSET->find_exist_fullpath(name);
	if(real_fullpath.empty())
		return NULL;
	infopath = real_fullpath + ".cli";

	if (NULL == (fi = load_downinfo_i_by_infopath(name, real_fullpath, infopath)))
		fi = load_downinfo_i_by_infopath(name, real_fullpath, infopath+"2");
	return fi;
}
cly_fileinfo* cly_fileinfo::load_downinfo_i_by_infopath(const string& name, const string& real_fullpath, const string& infopath)
{
	cly_fileinfo* fi = NULL;
	//unsigned int bitsize = 0;
	char buf[1024] = { 0, };

	cl_memblock* block = cl_memblock::allot(1024000);
	if (!block)
		return NULL;
	if (0 >= (block->wpos = cl_util::read_buffer_from_file(block->buf, block->buflen, infopath.c_str())))
	{
		block->release();
		return NULL;
	}
	unsigned int sum = cl_checksum32((unsigned char*)block->buf, block->wpos);
	cl_bstream bs(block->buf, block->wpos, block->wpos);
	bs.read(buf, 4);
	bs.skipr(4);
	if (0 != sum || 0 != memcmp(buf, "cdi2", 4))
	{
		block->release();
		return NULL;
	}

	fi = new cly_fileinfo();
	fi->info_path = infopath;
	fi->name = name;
	fi->fullpath = real_fullpath;

	bs.read_string(buf, 1024);
	fi->fhash = buf;
	bs >> fi->ftype;
	bs.read_string(buf, 1024);
	fi->ctime = buf;
	bs >> fi->block_gap;
	bs.read_string(buf, 1024);
	fi->sh.load_strhash(buf);
	bs >> fi->sh.bt_ok;
	bs >> fi->bt_fini;
	bs.read_array(fi->sh.fini, fi->sh.blocks);
	bs.read_array(fi->rcvB, 5);

	//检查数据合法性
	bool bok = true;
	if (0 != bs.ok() || 0 != bs.length() || fi->fhash.empty() || 0 == fi->sh.size)
		bok = false;
	if ((int)((fi->sh.size - 1) / CLY_FBLOCK_SIZE + 1) != fi->bt_fini.get_bitsize())
		bok = false;
	if (!fi->open_file(F64_RDWR))
		bok = false;

	fi->bt_memfini = fi->bt_fini;
	if (fi->sh.blocks != fi->sh.bt_ok.get_bitsize())
		fi->sh.bt_ok.alloc(fi->sh.blocks);

	block->release();
	if (!bok)
	{
		delete fi;
		return NULL;
	}
	//检查运算hash
	for (int i = 0; i<fi->sh.blocks; ++i)
		fi->check_sub_done(i);
	return fi;
}
int cly_fileinfo::save_downinfo_i(cly_fileinfo* fi)
{
	int ret=-1;
	fi->_file.flush();
	cl_memblock* block = cl_memblock::allot(1024000);
	if(!block)
		return -2;

	unsigned int sum = 0;
	cl_bstream bs(block->buf,block->buflen,0);
	bs.write("cdi2",4);
	bs.write(&sum,4);
	bs.write_string(fi->fhash.c_str());
	bs << fi->ftype;
	bs.write_string(fi->ctime.c_str());
	bs << fi->block_gap;
	bs.write_string(fi->sh.subhash.c_str());
	bs << fi->sh.bt_ok;
	bs << fi->bt_fini;
	bs.write_array(fi->sh.fini,fi->sh.blocks);
	bs.write_array(fi->rcvB,5);

	block->wpos = bs.length();
	if(0==bs.ok())
	{
		unsigned int sum = cl_checksum32((unsigned char*)block->buf,block->wpos);
		sum = ~sum + 1;
		memcpy(block->buf+4,&sum,4);

		//缓存最后一份
		string path2 = fi->info_path + "2";
		cl_util::file_delete(path2);
		cl_util::file_rename(fi->info_path, path2);
		ret = cl_util::write_buffer_to_file(block->buf,block->wpos,fi->info_path.c_str());
	}
	block->release();
	return ret;
}
//*******************************************************
//test:
void cly_fileinfo_test()
{
	cly_subhash sb;
	if(0!=sb.load_strhash("102500+102400+a:.13adlb1d3"))
	{
		printf("** test fail \n");
	}
	else
	{
		printf("test ok \n");
	}
}
