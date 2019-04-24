#pragma once
#include "cl_RDBFile64.h"
#include "cl_string.h"
#include "cl_thread.h"
#include "cl_feList.h"

//************************** 将32位整数转换为5个可视字符 ****************************
//定义64个可视字符，32位整数只取低30位转换为5个可视字符，每个字符代表6位，忽略最高2位
void cl_fhash_symbol_check_init();
unsigned int cl_fhash_symbol_to_u32(const char strhash[5]);
void cl_fhash_symbol_to_string(unsigned int n,char outstr[5]);
//===================================================================================

//将ASCII 转换为16进制可视字符；outs 必须大于2*size
void cl_hx2str(unsigned char digest[], int size, char outs[]);

//计算整个文件的SHA1
int cl_fhash_sha1_file(const char* path,char strhash[41]);

//计算整个文件的CRC32
int cl_fhash_crc32_file(const char* path, unsigned int& nhash);


//**************************** 文件分块hash crc32列表 ****************************
/*
文件块hash; 
sha1: 100m 约1.9秒. 1天=86400秒= 5T = 500个文件.
crc32: 100m 约0.8秒. 1天=86400秒= 10T = 1000个文件左右
*/
//CRC32 计算文件子块
int cl_fhash_fileblock(const char* path,int block_size,int index,unsigned int& nhash);

//CRC32 计算整个文件的所有子块
//最多分100块,生成格式为: size+subsize+a:shash,(a:表示c3c32算法) subsize必须是nfactor的整倍数.
int cl_fhash_file_sbhash(const char* path,int nfactor,char strhash[1024]);


//从文件长CRC32 HASH 中获取大小
long long cl_fhash_getfsize_from_sbhash(const char* strhash);
//从长HASH计算为短HASH MD5
string cl_fhash_sbhash_to_mainhash(const string& subhash);

//缓冲长hash和短hash到文件中
bool cl_get_fhash_from_hashfile(const string& path,string& hash,string& subhash);
bool cl_put_fhash_to_hashfile(const string& path, const string& hash, const string& subhash);
bool cl_get_buffer_from_hashfile(const string& path, string& buffer);
//===================================================================================


//********************************** 采样部分文件数据MD5 ****************************
//间隔skipsize采样blocksize进行md5
//hash = fhsb(blocksize)s(skipsize)f(size)i(blocks).md5
int cl_fhash_sample(const char* path, char hash[128], int blocksize, int skipsize);
//===================================================================================

class cl_fhash_sbhash : public cl_thread
{
private:
	typedef struct tag_block
	{
		char*	buf;
		int		size;
	}block_t;
public:
	cl_fhash_sbhash(const char* filepath, int nfactor); //分块必须是nfactor的整倍数
	virtual ~cl_fhash_sbhash();

	int get_percentage() const { return (int)((m_read_size+1)*1000/(m_fsize+1)); }
	int get_result(char strhash[1024]) { wait(); strcpy(strhash, m_strhash); return m_result; }
	virtual int work(int e);

private:
	static const int BUFSIZE;
	cl_ERDBFile64	m_file;
	int				m_result; // -1=计算失败，0=计算成功
	char			m_strhash[1024];
	char*			m_sbp;
	int				m_nfactor;
	size64_t		m_fsize,m_read_size;
	cl_feList<block_t*>	m_fe;
public:
	unsigned int m_readtick, m_hashtick;
};

//CRC32 计算整个文件的所有子块 ,利用多线程类
int cl_fhash_file_sbhash_T(const char* path, int nfactor, char strhash[1024]);
//test:
void cl_fhash_test();

