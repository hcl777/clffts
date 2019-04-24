#include "cl_fhash.h"
#include "cl_sha1.h"
#include "cl_crc32.h"
#include "cl_util.h"
#include "cl_md5_2.h"

#ifdef _WIN32
	#pragma warning(disable:4996)
#endif

char g_cl_fhash_symbol[64] = {0,};

void cl_fhash_symbol_check_init()
{
	int i = 0;
	if(g_cl_fhash_symbol[0]) return;
	g_cl_fhash_symbol[i++] = 48; //0x30; 0
	for(;i<10;++i)
		g_cl_fhash_symbol[i] = g_cl_fhash_symbol[i-1]+1;
	g_cl_fhash_symbol[i++] = 65; //0x41; A 
	for(;i<36;++i)
		g_cl_fhash_symbol[i] = g_cl_fhash_symbol[i-1]+1;
	g_cl_fhash_symbol[i++] = 97; //0x61; a
	for(;i<62;++i)
		g_cl_fhash_symbol[i] = g_cl_fhash_symbol[i-1]+1;
	g_cl_fhash_symbol[62] = 61; //0x3d; =
	g_cl_fhash_symbol[63] = 46;//0x2e; .
}
unsigned int cl_fhash_symbol_to_u32(const char strhash[5])
{
	unsigned int n,c;
	n = 0;
	for(int i=0;i<5;++i)
	{
		c=strhash[i];
		if(c>=48 && c<58)
			n += ((c-48)<<(i*6));
		else if(c>=65 && c<91)
			n += ((c-65+10)<<(i*6));
		else if(c>=97 && c<123)
			n += ((c-97+36)<<(i*6));
		else if(61==c)
			n += (62<<(i*6));
		else if(46==c)
			n += (63<<(i*6));
	}
	return n;
}
void cl_fhash_symbol_to_string(unsigned int n,char outstr[5])
{
	cl_fhash_symbol_check_init();
	//取低30位.
	for(int i=0;i<5;++i)
		outstr[i] = g_cl_fhash_symbol[(n>>(i*6))&0x3f];
}
unsigned int cl_fhash_sha1_to_u32(SHA1_CONTEXT* hd)
{
#define F1(x,y,z)   ( z ^ ( x & ( y ^ z ) ) )
#define rol(x,n) ( ((x) << (n)) | ((x) >> (32-(n))) )
	u32 n;
	n = F1(hd->h0,hd->h1,hd->h2);
	n += F1(hd->h1,hd->h3,n);
	n += rol(hd->h4,5);
	n += rol(n,25);
	n &= 0x3fffffff; //低30位.
	return n;
}

void cl_hx2str(unsigned char digest[], int size, char outs[])
{
	for (int i = 0; i<size; ++i)
		sprintf(outs + 2 * i, "%02x", digest[i]);
}
//返回40字节
int cl_fhash_sha1_file(const char* path,char strhash[41])
{
	int i=0;
	cl_ERDBFile64 file;
	size64_t size,readSize;
	unsigned char *buf;
	const int BUFSIZE = 64<<10; //16384
	readSize = 0;
	if(0!=file.open(path,F64_READ,RDBF_AUTO))
	{
		return -1;
	}
	size = file.seek(0,SEEK_END);
	file.seek(0,SEEK_SET);
	buf = new unsigned char[BUFSIZE];
	//********************
	SHA1_CONTEXT ctx;
	sha1_init (&ctx);
	while (readSize<size) 
	{
		if(0>=(i=file.read((char*)buf,BUFSIZE)))
		{
			//perror("#:read() failed:");
			break;
		}

		sha1_write(&ctx, buf, i);
		readSize+=i;
	}

	sha1_final(&ctx);
	//********************
	delete[] buf;
	file.close();

	if(readSize!=size)
		return -2;

	for(i=0;i<20;++i)
		sprintf(strhash+2*i,"%02x",ctx.buf[i]);
	strhash[40] = '\0';
	return 0;
}
int cl_fhash_crc32_file(const char* path, unsigned int& nhash)
{
	int i = 0;
	cl_ERDBFile64 file;
	size64_t size, readSize;
	unsigned char *buf;
	const int BUFSIZE = 64 << 10; //16384
	readSize = 0;
	if (0 != file.open(path, F64_READ, RDBF_AUTO))
	{
		return -1;
	}
	size = file.seek(0, SEEK_END);
	file.seek(0, SEEK_SET);
	buf = new unsigned char[BUFSIZE];
	//********************
	nhash = CL_CRC32_FIRST;
	while (readSize<size)
	{
		if (0 >= (i = file.read((char*)buf, BUFSIZE)))
		{
			//perror("#:read() failed:");
			break;
		}

		nhash = cl_crc32_write(nhash, buf, i);
		readSize += i;
	}

	//********************
	delete[] buf;
	file.close();

	if (readSize != size)
		return -2;

	return 0;
}
//返回5字节字符
int cl_fhash_fileblock(const char* path,int block_size,int index,unsigned int& nhash)
{
	int n=0;
	cl_ERDBFile64 file;
	size64_t size,readSize;
	size64_t pos,bsize;
	unsigned char *buf;
	const int BUFSIZE = 64<<10; //16384
	readSize = 0;
	assert(block_size>0);
	if(0!=file.open(path,F64_READ,RDBF_AUTO))
	{
		return -1;
	}
	size = file.get_file_size();
	pos = block_size*(size64_t)index;
	if(pos>=size)
		return -2;
	bsize = block_size;
	if(pos+bsize>size)
		bsize = size-pos;
	file.seek(pos,SEEK_SET);
	buf = new unsigned char[BUFSIZE];
	//********************
	//SHA1_CONTEXT ctx;
	//sha1_init (&ctx);
	nhash = CL_CRC32_FIRST;
	while (readSize<bsize) 
	{
		n = BUFSIZE;
		if((size64_t)n>bsize-readSize)
			n = (int)(bsize-readSize);
		if(0>=(n=file.read((char*)buf,n)))
		{
			//perror("#:read() failed:");
			break;
		}

		//sha1_write(&ctx, buf, n);
		nhash = cl_crc32_write(nhash,buf,n);
		readSize+=n;
	}

	//sha1_final(&ctx);
	//********************
	delete[] buf;
	file.close();

	if(readSize!=bsize)
		return -3;
	//nhash = cl_fhash_sha1_to_u32(&ctx);
	nhash &= 0x3fffffff; //低30位.
	return 0;
}
int cl_fhash_file_sbhash(const char* path,int nfactor,char strhash[1024])
{
	int block_size,blocks;
	unsigned int nhash;
	int i=0, n=0;
	cl_ERDBFile64 file;
	size64_t size,readSize;
	size64_t pos,bsize;
	unsigned char *buf;
	const int BUFSIZE = 64<<10; //16384
	const int MINBLOCK_SIZE = 10<<20;
	if(0!=file.open(path,F64_READ,RDBF_AUTO))
	{
		return -1;
	}
	size = file.get_file_size();
	if(size<=0) return -2;

	//计算block
	block_size = (int)((size+99)/100);
	if(block_size < MINBLOCK_SIZE)
		block_size = MINBLOCK_SIZE;
	if(nfactor>0)
	{
		if(block_size < nfactor)
			block_size = nfactor;
		n = block_size % nfactor;
		block_size -= n; //必须是nfactor的整倍数
	}

	blocks = (int)((size-1) / block_size + 1);
	sprintf(strhash,"%lld+%d+a:",size,block_size);
	char *p = strhash+strlen(strhash);

	buf = new unsigned char[BUFSIZE];
	for(i=0;i<blocks;++i)
	{
		pos = block_size*(size64_t)i;
		bsize = block_size;	
		if(pos+bsize>size)
			bsize = size-pos;

		file.seek(pos,SEEK_SET);
		//********************
		//SHA1_CONTEXT ctx;
		//sha1_init (&ctx);
		nhash = CL_CRC32_FIRST;
		readSize = 0;
		while (readSize<bsize) 
		{
			n = BUFSIZE;
			if(readSize+n>bsize)
				n = (int)(bsize-readSize);
			if(0!=file.read_n((char*)buf,n))
			{
				//perror("#:read() failed:");
				break;
			}

			//sha1_write(&ctx, buf, n);
			nhash = cl_crc32_write(nhash,buf,n);
			readSize+=n;
		}
		//sha1_final(&ctx);
		//********************
		if(readSize!=bsize)
			break;
		//nhash = cl_fhash_sha1_to_u32(&ctx);
		nhash &= 0x3fffffff; //低30位.
		cl_fhash_symbol_to_string(nhash,p+i*5);
	}
	delete[] buf;
	file.close();

	if(i!=blocks)
		return -1;
	p[i*5] = '\0';

	//加校验
	nhash = cl_crc32_write(CL_CRC32_FIRST,(unsigned char*)strhash,strlen(strhash));
	nhash &= 0x7fffffff;
	sprintf(p+i*5,"#%d",nhash);
	return 0;
}
long long cl_fhash_getfsize_from_sbhash(const char* strhash)
{
	long long size = 0;
	if(1!=sscanf(strhash,"%lld",&size))
		return 0;
	return size;
}

bool cl_get_fhash_from_hashfile(const string& path,string& hash,string& subhash)
{
	char buf[2048]={0,};
	string str;
	FILE *fp = fopen(path.c_str(),"rb");
	if(fp)
	{
		str = fgets(buf,2048,fp);
		fclose(fp);
	}
	if(!str.empty())
	{
		hash = cl_util::get_string_index(str,0,"|");
		subhash = cl_util::get_string_index(str,1,"|");
		unsigned int ncrc = atoi(cl_util::get_string_index(str,2,"|").c_str());
		unsigned int nhash = CL_CRC32_FIRST;
		nhash = cl_crc32_write(nhash,(unsigned char*)hash.c_str(),hash.length());
		nhash = cl_crc32_write(nhash,(unsigned char*)subhash.c_str(),subhash.length());
		nhash &= 0x7fffffff;
		if(!hash.empty() && !subhash.empty() && nhash==ncrc)
			return true;

	}
	return false;
}
bool cl_put_fhash_to_hashfile(const string& path,const string& hash,const string& subhash)
{
	char buf[2048];
	unsigned int nhash = CL_CRC32_FIRST;
	nhash = cl_crc32_write(nhash,(unsigned char*)hash.c_str(),hash.length());
	nhash = cl_crc32_write(nhash,(unsigned char*)subhash.c_str(),subhash.length());
	nhash &= 0x7fffffff;
	sprintf(buf,"%s|%s|%d",hash.c_str(),subhash.c_str(),nhash);
	FILE *fp = fopen(path.c_str(),"wb+");
	if(fp)
	{
fputs(buf, fp);
fclose(fp);
return true;
	}
	return false;
}
bool cl_get_buffer_from_hashfile(const string& path, string& buffer)
{
	char buf[2048] = { 0, };
	FILE *fp = fopen(path.c_str(), "rb");
	if (fp)
	{
		buffer = fgets(buf, 2048, fp);
		fclose(fp);
		if (buffer.empty())
			return true;
	}
	return false;
}
string cl_fhash_sbhash_to_mainhash(const string& subhash)
{
	char shash[48];
	string hash;
	hash = cl_md5_buffer((unsigned char*)subhash.c_str(), subhash.length(), shash);
	return hash;
}

//间隔skipsize采样blocksize进行md5
//hash = fhsb(blocksize)s(skipsize)f(size)i(blocks).md5
int cl_fhash_sample(const char* path, char hash[128], int blocksize, int skipsize)
{
	//unsigned int nhash = CL_CRC32_FIRST;
	int i = 0, n = 0;
	cl_ERDBFile64 file;
	size64_t size;
	size64_t pos;
	unsigned char *buf;
	char smd5[33];
	unsigned char digest[16];
	CL_MD5_CTX ctx;
	cl_md5_init(&ctx);
	DWORD readtick = 0, md5tick = 0;
	DWORD lasttick, tick;
	if (0 != file.open(path, F64_READ, RDBF_AUTO))
	{
		return -1;
	}
	size = file.get_file_size();
	if (size <= 0) return -2;

	pos = 0;
	buf = new unsigned char[blocksize];
	while (pos < size)
	{
		n = blocksize;
		if (pos + n > size)
			n = (int)(size - pos);
		lasttick = GetTickCount();
		if (0 != file.read_n((char*)buf, n))
			break;
		tick = GetTickCount();
		readtick += (tick - lasttick);
		lasttick = tick;
		//nhash = cl_crc32_write(nhash, buf, n);
		//cl_md5_update(&ctx, (unsigned char*)&nhash, 4);
		cl_md5_update(&ctx, (unsigned char*)buf, n);
		tick = GetTickCount();
		md5tick += (tick - lasttick);

		pos += n;
		i++;
		if (pos >= size)
			break;

		if (pos + skipsize + blocksize > size)
			pos = CL_MAX(pos, size - blocksize);
		else
			pos += skipsize;
		if (pos != (size64_t)file.seek(pos, SEEK_SET))
			break;
	}
	delete[] buf;
	file.close();
	cl_md5_final(&ctx, digest);
	cl_md5_hx2str(digest, smd5);

	if (pos != size)
		return -3;

	printf("fhs times: size=%lld, readsec=%.2f,md5sec=%.2f \n", size, (float)(readtick / (float)1000), (float)(md5tick / (float)1000));
	sprintf(hash, "fhsb%ds%df%lldi%d.%s", blocksize, skipsize, size, i, smd5);
	return 0;
}

//************************************************************************************
//cl_fhash_sbhash
const int cl_fhash_sbhash::BUFSIZE = 64 << 10;
cl_fhash_sbhash::cl_fhash_sbhash(const char* filepath, int nfactor)
	:m_result(0)
	,m_nfactor(nfactor)
	,m_fsize(0)
	,m_read_size(0)
	,m_readtick(0)
	,m_hashtick(0)
{
	memset(m_strhash, 0, 1024);
	if (0 != m_file.open(filepath, F64_READ, RDBF_AUTO) || (m_fsize = m_file.get_file_size()) <= 0)
	{
		m_result = -1;
	}
	else
	{
		for (int i = 0; i < 5; ++i)
		{
			block_t* b = new block_t();
			b->buf = new char[BUFSIZE];
			b->size = 0;
			m_fe.put_empty_node(b);
		}
		activate(2);
	}
}
cl_fhash_sbhash::~cl_fhash_sbhash() 
{ 
	wait(); 
	block_t* b;
	while ((b = m_fe.get_empty_node()))
	{
		delete[] b->buf;
		delete b;
	}
	while ((b = m_fe.get_full_node()))
	{
		delete[] b->buf;
		delete b;
	}
}

int cl_fhash_sbhash::work(int e)
{
	DWORD tick;
	if (0 == e)
	{
		block_t *b;
		const int MINBLOCK_SIZE = 10 << 20;
		int block_size, blocks;
		size64_t size,pos, bsize,readSize;
		int i = 0, n = 0;
		size = m_fsize;

		//计算block
		block_size = (int)((size + 99) / 100);
		if (block_size < MINBLOCK_SIZE)
			block_size = MINBLOCK_SIZE;
		if (m_nfactor>0)
		{
			if (block_size < m_nfactor)
				block_size = m_nfactor;
			n = block_size % m_nfactor;
			block_size -= n; //必须是m_nfactor的整倍数
		}

		blocks = (int)((size - 1) / block_size + 1);
		sprintf(m_strhash, "%lld+%d+a:", size, block_size);
		m_sbp = m_strhash + strlen(m_strhash);
		for(i=0;i<blocks;++i)
		{
			
			pos = block_size * (size64_t)i;
			bsize = block_size;
			if (pos + bsize>size)
				bsize = size - pos;

			assert(pos == m_read_size);
			m_file.seek(pos, SEEK_SET);
			readSize = 0;
			while (readSize<bsize)
			{
				while (0 == m_fe.get_empty_size()) Sleep(0);//避免点锁
				while (NULL == (b = m_fe.get_empty_node())) Sleep(0);
				n = BUFSIZE;
				if (readSize + n>bsize)
					n = (int)(bsize - readSize);
				tick = GetTickCount();
				if (0 != m_file.read_n(b->buf, n))
				{
					//perror("#:read() failed:");
					m_fe.put_empty_node(b);
					break;
				}
				m_readtick += GetTickCount() - tick;
				b->size = n;
				
				readSize += n;
				m_read_size += n;
				m_fe.put_full_node(b);
			}
			if (readSize != bsize)
				break;
			while (NULL == (b = m_fe.get_empty_node())) Sleep(0);
			b->size = 0;
			m_fe.put_full_node(b);
		}
		if (m_read_size != size)
			m_result = -2;
		else
			m_result = 0;
		while (NULL == (b = m_fe.get_empty_node())) Sleep(0);
		b->size = -1;
		m_fe.put_full_node(b);
	}
	else
	{
		block_t *b;
		unsigned int nhash = CL_CRC32_FIRST;
		size64_t read_size = 0;
		int i = 0,size;
		while (1)
		{
			while (0 == m_fe.get_full_size()) Sleep(0);//避免点锁
			while (NULL == (b = m_fe.get_full_node())) Sleep(0);
			size = b->size;
			if (size > 0)
			{
				tick = GetTickCount();
				nhash = cl_crc32_write(nhash, (unsigned char*)b->buf, size);
				m_hashtick += GetTickCount() - tick;
				read_size += size;
			}
			else if (0 == size)
			{
				nhash &= 0x3fffffff; //低30位.
				cl_fhash_symbol_to_string(nhash, m_sbp + i * 5);
				nhash = CL_CRC32_FIRST;
				i++;
			}
			m_fe.put_empty_node(b);
			if (-1 == size)
			{
				break;
			}
		}

		m_sbp[i * 5] = '\0';

		//加校验
		nhash = cl_crc32_write(CL_CRC32_FIRST, (unsigned char*)m_strhash, strlen(m_strhash));
		nhash &= 0x7fffffff;
		sprintf(m_sbp + i * 5, "#%d", nhash);
	}
	return 0;
}
int cl_fhash_file_sbhash_T(const char* path, int nfactor, char strhash[1024])
{
	cl_fhash_sbhash t(path, nfactor);
	int percentage = 0;
	while (1000 != (percentage = t.get_percentage()))
	{
		//printf("\r(%.2f%% sec=rh(%.3f,%.3f))", percentage/(float)10, t.m_readtick / (float)1000, t.m_hashtick / (float)1000);
		Sleep(500);
	}
	printf("(%.2f%% sec=rh(%.3f,%.3f))\n", percentage/(float)10,t.m_readtick / (float)1000, t.m_hashtick / (float)1000);
	return t.get_result(strhash);
}
//************************************************************************************

//************************************************************************************
//test:

void cl_fhash_test()
{
	string path = "e:/hdfilm/hbtr.mkv";
	unsigned int nhash = 0;
	char strhash[6];
	char str[1024];
	size64_t size = 0;
	int block_size = 0;
	strhash[5] = '\0';
	DWORD tick = GetTickCount();
	if(0==cl_fhash_file_sbhash(path.c_str(),102400,str))
	{
		size = cl_util::atoll(cl_util::get_string_index(str,0,"+").c_str());
		block_size = atoi(cl_util::get_string_index(str,1,"+").c_str());
		printf("%s \n size=%lld,block_size=%d (sec=%.3f)\n",str,size,block_size,(GetTickCount()-tick)/(double)1000);
		tick = GetTickCount();

		cl_fhash_fileblock(path.c_str(),block_size,0,nhash);
		cl_fhash_symbol_to_string(nhash,strhash);
		printf("0: %s (sec=%.3f)\n",strhash,(GetTickCount()-tick)/(double)1000);
		tick = GetTickCount();

		cl_fhash_fileblock(path.c_str(),block_size,2,nhash);
		cl_fhash_symbol_to_string(nhash,strhash);
		printf("2: %s (sec=%.3f)\n",strhash,(GetTickCount()-tick)/(double)1000);
		tick = GetTickCount();

		cl_fhash_fileblock(path.c_str(),block_size,3,nhash);
		cl_fhash_symbol_to_string(nhash,strhash);
		printf("3: %s (sec=%.3f)\n",strhash,(GetTickCount()-tick)/(double)1000);
		tick = GetTickCount();

		cl_fhash_fileblock(path.c_str(),block_size,5,nhash);
		cl_fhash_symbol_to_string(nhash,strhash);
		printf("5: %s (sec=%.3f)\n",strhash,(GetTickCount()-tick)/(double)1000);
		tick = GetTickCount();

		cl_fhash_fileblock(path.c_str(),block_size,40,nhash);
		cl_fhash_symbol_to_string(nhash,strhash);
		printf("40: %s (sec=%.3f)\n",strhash,(GetTickCount()-tick)/(double)1000);
	}
}
