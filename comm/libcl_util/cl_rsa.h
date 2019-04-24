#pragma once


/*******************************
rsa算法:
密钥:unsigned int n,e,d. 其中(n,e)为公钥, (n,d)为私钥.
n 是 两个大素数p,q(质数)的剩积. e任选,满足e 与(p-1)*(q-1) 互质.
n的二进制表示时所占用的位数，就是所谓的密钥长度.
d，要求(d*e)mod((p-1)*(q-1))=1 ; (mod 即求余%).

加解密:
RSA加解密的算法完全相同，设A为明文，B为密文，则：
A=B^d mod n；
B=A^a mod n；

明文块的位数必须小于A.
目前一般n为1024 位密钥长度极安全.但加密计算量极大.
公钥指数e普遍选的都是65537（0x10001，5bits）以内，该值是除了1、3、5、17、257之外的最小素数.
********************************/

/****************************************************
cl_rsa32 (n,e,d): 仅供参考.可轻易破解.

*****************************************************/
int cl_rsa32_randkey1(unsigned int& p,unsigned int& q,unsigned int& N,unsigned int& n,unsigned int& e,unsigned int& d);
int cl_rsa32_randkey(unsigned int& n,unsigned int& e,unsigned int& d);
//按16位加密,余数部分不加.osize 必须预先指定为osize>=2size
int cl_rsa32_encrypt(unsigned int n,unsigned int e,const char* buf,int size,char* obuf,int* osize);
//按32位解密,余数部分不加.osize 必须预先指定为osize>=size/2
int cl_rsa32_decrypt(unsigned int n,unsigned int d,const char* buf,int size,char* obuf,int* osize);

//
void cl_rsa32_test();

