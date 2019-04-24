#include "cl_aesEbc.h"
#include "cl_file64.h"


int operator << (cl_bstream& b, const cl_aesEbcHeader_t& i)
{
	b.write(i.stx, 8);
	b << i.header_size;
	b << i.file_size;
	b.write(i.hash, 40);
	b.write(i.crypt_name, 16);
	b.write(i.key_url, 128);
	b.write(i.extra, 128);
	return b.ok();
}
int operator >> (cl_bstream& b, cl_aesEbcHeader_t& i)
{
	b.read(i.stx, 8);
	b >> i.header_size;
	b >> i.file_size;
	b.read(i.hash, 40);
	b.read(i.crypt_name, 16);
	b.read(i.key_url, 128);
	b.read(i.extra, 128);
	i.hash[40] = '\0';
	return b.ok();
}


int cl_aesEbc_encrypt_file(const char* frompath, const char* topath, uchar aeskey[16], cl_aesEbcHeader_t& inf)
{
	char buf[1024];
	cl_file64 from,to;
	long long read_size = 0;
	int i,n;
	if (0 != from.open(frompath, F64_READ) || 0 != to.open(topath, F64_RDWR | F64_TRUN))
		return -1;
	inf.file_size = from.get_file_size(); 
	if (inf.file_size > 4)
	{
		if (0 != from.read_n(buf, 8))
			return -3;
		if (0 == memcmp(buf, CL_AES_STX, 8))
			return 1; //已经是加密的
		from.seek(0, SEEK_SET);
	}
	for (i = 0; i < 16; ++i)
	{
		inf.extra[i] = aeskey[i]+10;
	}
	
	cl_bstream s(1024);
	s << inf;
	s.fitsize32(8);
	if (0 != to.write_n(s.buffer(), s.length()))
		return -4;

	//crypt
	cl_aes aes;
	uchar *p1, *p2;
	const int BUFLEN = 16 * 1024;
	p1 = new uchar[BUFLEN];
	p2 = new uchar[BUFLEN];
	aes.set_key(aeskey, 128);
	while (read_size < inf.file_size)
	{
		n = BUFLEN;
		if (read_size + n > inf.file_size)
			n = (int)(inf.file_size - read_size);
		if (0 != from.read_n((char*)p1, n))
			break;

		for (i = 0; i < n; i += 16)
		{
			aes.encrypt(p1 + i, p2 + i);
		}
		//最后数据加密后i可能大于n
		if (0 != to.write_n((char*)p2, i))
			break;
		read_size += n;
	}
	
	delete[] p1;
	delete[] p2;
	
	return read_size==inf.file_size?0:-5;
}
int cl_aesEbc_decrypt_file(const char* frompath, const char* topath, uchar aeskey[16])
{
	char buf[1024];
	cl_file64 from, to;
	long long read_size = 0;
	long long file_size;
	int		i, n;
	cl_aesEbcHeader_t inf;

	if (0 != from.open(frompath, F64_READ) || 0 != to.open(topath, F64_RDWR | F64_TRUN))
		return -1;
	file_size = from.get_file_size();
	if (file_size < 12)
		return 1;
	if (0 != from.read_n(buf, 12))
		return -1;
	if (0 != memcmp(buf, CL_AES_STX, 8))
		return 1;

	cl_bstream s(buf+8, 4, 4);
	s >> inf.header_size;

	//不检查错误
	if(0 != from.read_n(buf + 12, inf.header_size - 12))
		return -1;
	s.attach(buf, inf.header_size, inf.header_size);
	if (0 != s >> inf)
		return -2;

	//数据必须是16的整倍数
	if (0 != (file_size - inf.header_size) % 16)
		return -3;

	if (aeskey[0] == 0)
	{
		for (i = 0; i < 16; ++i)
		{
			aeskey[i] = inf.extra[i] - 10;
		}
	}

	//crypt
	cl_aes aes;
	uchar *p1, *p2;
	const int BUFLEN = 16 * 1024;
	long long write_size = 0;
	p1 = new uchar[BUFLEN];
	p2 = new uchar[BUFLEN];
	aes.set_key(aeskey, 128);
	read_size = inf.header_size;
	while (read_size < file_size)
	{
		n = BUFLEN;
		if (read_size + n > file_size)
			n = (int)(file_size - read_size);
		if (0 != from.read_n((char*)p1, n))
			break;

		for (i = 0; i < n; i += 16)
		{
			aes.decrypt(p1 + i, p2 + i);
		}
		assert(i == n);
		//最后数据解密后i可能大于n
		if (write_size + i > inf.file_size)
			i = (int)(inf.file_size - write_size);

		if (0 != to.write_n((char*)p2, i))
			break;
		write_size += i;
		read_size += n;
	}

	delete[] p1;
	delete[] p2;

	return write_size == inf.file_size ? 0 : -5;
}
cl_aesEbc::cl_aesEbc()
{
}


cl_aesEbc::~cl_aesEbc()
{
}
