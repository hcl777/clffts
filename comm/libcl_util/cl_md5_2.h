#pragma once
 
//本md5程序版本比cl_md5要快很多.
//100MB运算:
//win c++代码: md5=1.5秒,sha1=1.5秒
//linux c++代码: md5=1.7秒. sha1=1.9秒
//linux自带: md5=1秒, sha1=1.5秒.

typedef struct
{
    unsigned int count[2];
    unsigned int state[4];
    unsigned char buffer[64];   

}CL_MD5_CTX;
                                           
void cl_md5_init(CL_MD5_CTX *context);
void cl_md5_update(CL_MD5_CTX *context,unsigned char *input,unsigned int inputlen);
void cl_md5_final(CL_MD5_CTX *context,unsigned char digest[16]);

void cl_md5_hx2str(unsigned char digest[16],char outstr[33]);
char* cl_md5_buffer(unsigned  char* buf,int size,char outstr[33]);
