#pragma once
#include "cl_memblock.h"
#include "cl_RDBFile64.h"
#include "cl_bittable.h"
#include "cl_ntypes.h"
#include "cly_checkhash.h"

enum {FTYPE_VOD=0,FTYPE_DOWNLOAD,FTYPE_SHARE,FTYPE_DISTRIBUTION};

//��100K�ֿ�
#define CLY_FBLOCK_SIZE 102400


/***********************
cly_subhash ����:
load strhash.
***********************/
class cly_subhash
{
public:
	string			subhash;
	int				subsize; //������CLY_FBLOCK_SIZE��������.����У��ȡ����С���������ز��ô���.
	size64_t		size;
	int				blocks;
	int				block_times; //С�ֿ�ı���
	unsigned int	*h;
	unsigned int	*fini; //��¼ÿ��subchunk �Ѿ������˶��ٸ�block ,��������ʱȫ����һ��
	cl_bittable		bt_ok; //�Ƿ��Ѿ��������ӿ�ɹ�.

public:
	cly_subhash(void):subsize(0),size(0),blocks(0),block_times(0),h(NULL),fini(NULL){}
	~cly_subhash(void){ if(h) delete[] h; if(fini) delete[] fini;}
	
	int load_strhash(const string& strhash);
	void reset();
private:
};

/************************
˵��:
1.��д�ļ�����
2.��д�����е���Ϣ
************************/

class cly_fileinfo
{
public:
	typedef cl_CriticalSection Mutex;
	typedef map<unsigned int,cl_memblock*> FileBlockMap;
	typedef FileBlockMap::iterator FileBlockMapIter;

	int				ref;
	string			fhash;
	cly_subhash		sh; //���������С
	//size64_t		size;
	string			name,fullpath; //fullpathΪȫ·������fullpath�����ڣ�����name����һ�飬name���ܴ���·����
	string			info_path;
	int				ftype;
	bool			bready;
	string			url; //��¼һ����URL�����URL�����浽���أ�ֻ�ǿ�ʼ����ʱ����
	string			ctime; //����ʱ��
	string			mtime; //���ʹ��ʱ��

	//unsigned int	begin_block; //ָ���ӵڼ��鿪ʼ����
	//unsigned int	blocks; //һ����Ҫ���ؿ���
	unsigned int	block_gap; //����ȱ��,ָ��block_offset���𣬵�1������δ���صĿ��,���ٷ���������

	cl_bittable		bt_fini;
	cl_bittable		bt_memfini;
	size64_t		req_offset;
	size64_t		last_req_offset;

	size64_t		rcvB[5]; //0����������������1�ͻ��˵�����,����:����
	
	FileBlockMap	fbmp;
	cl_ERDBFile64	_file;
	bool			bsave_original; //ָ������Ϊԭʼ�ļ����ⲿ��ָ�����˱�־������

	int				no; //���,�������
	string			http_url; //http������
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

	//����ʱ�ã���download����һ����������ʱ����ȡblock��ַ
	cl_memblock* get_download_block(unsigned int index);
	//int write_block_data(unsigned int index,int len,int offset,const char* buf);
	//����ʱʹ��
	cl_memblock* read_block(unsigned int index);
	int read_data(ssize64_t pos,char* buf,int size);

	int set_subhash(const string& subhash);

	int on_block_done(unsigned int index);
	void on_check_subhash_result(const cly_checkhash_info_t& inf);
	void delete_infofile();

	//������fullpath��fullpath�Ҳ�������nameƥ������
	static cly_fileinfo *load_downinfo_i(const string& name,const string& fullpath);
	static cly_fileinfo *load_downinfo_i_by_infopath(const string& name , const string& real_fullpath, const string& infopath);
	static int save_downinfo_i(cly_fileinfo* fi);
	static unsigned int get_block_real_size(size64_t fsize,unsigned int index);
private:
	void check_sub_done(int n);
	//д���ļ�
	int write_block_to_file(unsigned int index);
	void try_free_memcache();
private:
	Mutex m_mt;
};

//test:
void cly_fileinfo_test();
