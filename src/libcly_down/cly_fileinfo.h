#pragma once
#include "cl_memblock.h"
#include "cl_RDBFile64.h"
#include "cl_bittable.h"
#include "cl_ntypes.h"
#include "cly_checkhash.h"

enum {FTYPE_VOD=0,FTYPE_DOWNLOAD,FTYPE_SHARE,FTYPE_DISTRIBUTION};

//按100K分块
#define CLY_FBLOCK_SIZE 102400


/***********************
cly_subhash 功能:
load strhash.
***********************/
class cly_subhash
{
public:
	string			subhash;
	int				subsize; //必须是CLY_FBLOCK_SIZE的整倍数.否则校验取消甘小块重新下载不好处理.
	size64_t		size;
	int				blocks;
	int				block_times; //小分块的倍数
	unsigned int	*h;
	unsigned int	*fini; //记录每个subchunk 已经下载了多少个block ,创建任务时全面检测一遍
	cl_bittable		bt_ok; //是否已经较验了子块成功.

public:
	cly_subhash(void):subsize(0),size(0),blocks(0),block_times(0),h(NULL),fini(NULL){}
	~cly_subhash(void){ if(h) delete[] h; if(fini) delete[] fini;}
	
	int load_strhash(const string& strhash);
	void reset();
private:
};

/************************
说明:
1.读写文件数据
2.读写下载中的信息
************************/

class cly_fileinfo
{
public:
	typedef cl_CriticalSection Mutex;
	typedef map<unsigned int,cl_memblock*> FileBlockMap;
	typedef FileBlockMap::iterator FileBlockMapIter;

	int				ref;
	string			fhash;
	cly_subhash		sh; //这里包括大小
	//size64_t		size;
	string			name,fullpath; //fullpath为全路径，当fullpath不存在，则用name查找一遍，name可能带子路径。
	string			info_path;
	int				ftype;
	bool			bready;
	string			url; //记录一个主URL，这个URL不保存到本地，只是开始下载时产生
	string			ctime; //创建时间
	string			mtime; //最后使用时间

	//unsigned int	begin_block; //指定从第几块开始下载
	//unsigned int	blocks; //一共需要下载块数
	unsigned int	block_gap; //下载缺口,指从block_offset块起，第1个出现未下载的块号,快速分配任务用

	cl_bittable		bt_fini;
	cl_bittable		bt_memfini;
	size64_t		req_offset;
	size64_t		last_req_offset;

	size64_t		rcvB[5]; //0：服务器的流量，1客户端的流量,其它:保留
	
	FileBlockMap	fbmp;
	cl_ERDBFile64	_file;
	bool			bsave_original; //指定保存为原始文件，外部分指定，此标志不保存

	int				no; //序号,共享表用
	string			http_url; //http下载用
public:
	cly_fileinfo(void);
	~cly_fileinfo(void);

	//void refer(){ref++;}
	//void release();

	bool is_write_disk() const;
	bool open_file(int mode,int rdbftype=RDBF_AUTO);
	void close_file();


	bool is_finished()const { return (sh.size>0 && sh.size!=UINT64_INFINITE && bt_fini.is_setall());}
	bool is_memfinished() const{ return (sh.size>0 && sh.size!=UINT64_INFINITE && bt_memfini.is_setall());}
	
	//bool is_allow_pause();

	//下载时用，当download分配一个下载任务时，获取block地址
	cl_memblock* get_download_block(unsigned int index);
	//int write_block_data(unsigned int index,int len,int offset,const char* buf);
	//共享时使用
	cl_memblock* read_block(unsigned int index);
	int read_data(ssize64_t pos,char* buf,int size);

	int set_subhash(const string& subhash);

	int on_block_done(unsigned int index);
	void on_check_subhash_result(const cly_checkhash_info_t& inf);
	void delete_infofile();

	//优先用fullpath，fullpath找不到再用name匹配搜索
	static cly_fileinfo *load_downinfo_i(const string& name,const string& fullpath);
	static cly_fileinfo *load_downinfo_i_by_infopath(const string& name , const string& real_fullpath, const string& infopath);
	static int save_downinfo_i(cly_fileinfo* fi);
	static unsigned int get_block_real_size(size64_t fsize,unsigned int index);
private:
	void check_sub_done(int n);
	//写入文件
	int write_block_to_file(unsigned int index);
	void try_free_memcache();
private:
	Mutex m_mt;
};

//test:
void cly_fileinfo_test();
