#pragma once

//ip packet analysis. IP 包分析


struct cl_ip_iaddr
{
	int protocol;
	unsigned int snip;
	unsigned int dnip;
	unsigned short snport;
	unsigned int dnport;
	int pk_size;
};


//big endian
//20 byte
struct cl_ip_header
{
  unsigned char ihl:4, version:4;	//低4B:首部长度(单位为32bit,即4字节)，高4B:版本 
  unsigned char tos;		//服务类型
  unsigned short tot_len;	//包总长度，含IP头+IP包数据体
  unsigned short id;		//唯一标识，ip层用一个从0开始递增的数
  unsigned short frag_off;	//低13位：偏移，高3B:标志(0,DF,MF）DF=1表示不支持路由分片，MF=1表示分片包
  unsigned char ttl;		//生存时间
  unsigned char protocol;	//协议：1-ICMP，2-IGMP 6-TCP，17-UDP,88-IGRP,89-OSPF
  unsigned short check;		//IP首部检验和
  unsigned int saddr;		//源地址IPV4
  unsigned int daddr;		//目的地址
  /*The options start here. */
};
//20 byte
struct cl_tcp_header
{
  unsigned short source; //源端口
  unsigned short dest;	//目标端口
  unsigned int seq;	//发送序列号
  unsigned int ack_seq;	//ACK序列号
  //以下假设是LITTLE_ENDIAN
  //低4B 保留，高4B:TCP头长, ...高2B保留
  unsigned short res1:4, doff:4, fin:1, syn:1, rst:1, psh:1,
                 ack:1, urg:1,res2:2; //
  unsigned short window;	//发送窗口(接收方缓冲大小)
  unsigned short check; //检验和,(首部+数据)的校验和
  unsigned short urg_ptr; //紧张指针，指向紧急数据的起始位置
  //选项；
  //(kind，length) = (0,1结束，不用) ,（1，1无操作，填充),（2(MTU),4） ，(3(窗口扩大因子),4），(4（SACK),2），(8（时间戳),10）
};
//8 byte
struct cl_udp_header
{
  unsigned short source; //源端口
  unsigned short dest;	//目标端口
  unsigned short len;	//数据长度,包UDP头长度
  unsigned short chsum;	// 校验和
};

//***************************************************************
class cl_ipa
{
private:
	cl_ipa(void){}
	~cl_ipa(void){}
public:
	static int get_ip_iaddr(cl_ip_iaddr& ipi,const char* buf,int size);
	static void dump_ippacket(const char* buf,int size);
	static void dump_ippacket2(const char* buf,int size);

};

