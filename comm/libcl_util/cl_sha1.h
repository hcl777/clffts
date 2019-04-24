#pragma once


//**************************
typedef unsigned int u32;
typedef struct {
	u32  h0,h1,h2,h3,h4;
	u32  nblocks;
	unsigned char buf[64];
	int  count;
	u32 x[16]; //临时计算用
} SHA1_CONTEXT;

void sha1_init( SHA1_CONTEXT *hd );
void sha1_write( SHA1_CONTEXT *hd, const unsigned char *inbuf, int inlen);
void sha1_final(SHA1_CONTEXT *hd);
//**************************


//**************************
//以下是简单包装运算
//sOutStr和sOutBuf不能同时为空，sOutStr最小41长度，sOutBuf最小20长度，iSleepMSec表示每运算一长度时休息一下
int Sha1_BuildFile(const char* sFile,char* sOutStr,char* sOutBuf,int iSleepMSec=-1,bool pmsg=true);  
int Sha1_BuildBuffer(const char* buf,int bufLen,char* sOutStr,char* sOutBuf);
int Sha1_Buf2Str(char* sOutStr, const char* sInBuf);
int Sha1_Str2Buf(char* sOutBuf, const char* sInStr);
