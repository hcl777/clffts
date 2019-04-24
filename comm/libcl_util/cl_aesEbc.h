#pragma once
#include "cl_aes.h"
#include "cl_bstream.h"

#define CL_AES_STX ".claesf1"
typedef struct tag_cl_aesEbcHeader
{
	//char			stx[9] = { CL_AES_STX };
	//int				header_size = 0;
	//long long		file_size = 0;
	//char			hash[41] = { "" }; //实际只有40长度
	//char			crypt_name[16] = { "aes128.ecb" };
	//char			key_url[128] = { "" };
	//uchar			extra[128];


	char			stx[9];
	int				header_size;
	long long		file_size;
	char			hash[41]; //实际只有40长度
	char			crypt_name[16];
	char			key_url[128];
	uchar			extra[128];
	tag_cl_aesEbcHeader(void)
	{
		strcpy(stx, CL_AES_STX);
		header_size = 0;
		file_size = 0;
		memset(hash, 0, 41);
		strcpy(crypt_name, "aes128.ecb");
		memset(key_url, 0, 128);
		memset(extra, 0, 128);
	}
} cl_aesEbcHeader_t;
//using cl_aesEbcHeader_t = tag_cl_aesEbcHeader;

int cl_aesEbc_encrypt_file(const char* frompath, const char* topath,uchar aeskey[16], cl_aesEbcHeader_t& inf);
int cl_aesEbc_decrypt_file(const char* frompath, const char* topath, uchar aeskey[16]);
class cl_aesEbc
{
public:
	cl_aesEbc();
	~cl_aesEbc();


private:
};

