#include "cl_ntp.h"

#include "cl_net.h"

#define NTP_PORT               123               /*NTP专 用端口号字符串*/
#define TIME_PORT              37               /* TIME/UDP端 口号 */
#define NTP_SERVER_IP       "210.72.145.44" /*国家授时中心 IP*/
#define NTP_PORT_STR        "123"          /*NTP专用端口号字 符串*/
#define NTPV1                "NTP/V1"      /*协议及其版本号*/
#define NTPV2                "NTP/V2"
#define NTPV3                "NTP/V3"
#define NTPV4                "NTP/V4"
#define TIME                "TIME/UDP"
 
#define NTP_PCK_LEN 48
#define LI 0
#define VN 3
#define MODE 3
#define STRATUM 0
#define POLL 4
#define PREC -6
 
//(unsigned int)2208988800UL
#define JAN_1970 0x83aa7e80 /* 1900年～1970年之间的时间秒数 */
#define SECONDS_1900_1970 0x83aa7e80

#define NTPFRAC(x)     (4294 * (x) + ((1981 * (x)) >> 11))
#define USEC(x)         (((x) >> 12) - 759 * ((((x) >> 10) + 32768) >> 16))



typedef struct _ntp_time
{
    unsigned int coarse;
    unsigned int fine;
} ntp_time;

struct ntp_packet
{
     unsigned char leap_ver_mode;
     unsigned char startum;
     char poll;
     char precision;
     int root_delay;
     int root_dispersion;
     int reference_identifier;
     ntp_time reference_timestamp;
     ntp_time originage_timestamp;
     ntp_time receive_timestamp;
     ntp_time transmit_timestamp;
};
/*构建NTP协议包*/
int construct_packet(char *packet)
{
     char version = 1;
     long tmp_wrd;
     //int port;
     time_t timer;
	 char protocol[32];
     strcpy(protocol, NTPV3);
     /*判断协议版本*/
     if(!strcmp(protocol, NTPV1)||!strcmp(protocol, NTPV2)
           ||!strcmp(protocol, NTPV3)||!strcmp(protocol, NTPV4))
     {
          memset(packet, 0, NTP_PCK_LEN);
          //port = NTP_PORT;
          /*设置16字节的包头*/
          version = protocol[6] - 0x30;
          tmp_wrd = htonl((LI << 30)|(version << 27)
                |(MODE << 24)|(STRATUM << 16)|(POLL << 8)|(PREC & 0xff));
          memcpy(packet, &tmp_wrd, sizeof(tmp_wrd));
         
          /*设置Root Delay、Root Dispersion和Reference Indentifier */
          tmp_wrd = htonl(1<<16);
          memcpy(&packet[4], &tmp_wrd, sizeof(tmp_wrd));
          memcpy(&packet[8], &tmp_wrd, sizeof(tmp_wrd));
          /*设置Timestamp部分*/
          time(&timer);
          /*设置Transmit Timestamp coarse*/
          tmp_wrd = htonl(JAN_1970 + (long)timer);
          memcpy(&packet[40], &tmp_wrd, sizeof(tmp_wrd));
          /*设置Transmit Timestamp fine*/
          tmp_wrd = htonl((long)NTPFRAC(timer));
          memcpy(&packet[44], &tmp_wrd, sizeof(tmp_wrd));
          return NTP_PCK_LEN;
     }
     else if (!strcmp(protocol, TIME))/* "TIME/UDP" */
     {
          //port = TIME_PORT;
          memset(packet, 0, 4);
          return 4;
     }
     return 0;
}
int construct_ntp_packet(const char* data,int size,ntp_packet* pk)
{
	if(size<48) return -1;
	pk->leap_ver_mode = data[0];
    pk->startum = data[1];
    pk->poll = data[2];
    pk->precision = data[3];
    pk->root_delay = ntohl(*(int*)&(data[4]));
    pk->root_dispersion = ntohl(*(int*)&(data[8]));
    pk->reference_identifier = ntohl(*(int*)&(data[12]));
    pk->reference_timestamp.coarse = ntohl(*(int*)&(data[16]));
    pk->reference_timestamp.fine = ntohl(*(int*)&(data[20]));
    pk->originage_timestamp.coarse = ntohl(*(int*)&(data[24]));
    pk->originage_timestamp.fine = ntohl(*(int*)&(data[28]));
    pk->receive_timestamp.coarse = ntohl(*(int*)&(data[32]));
    pk->receive_timestamp.fine = ntohl(*(int*)&(data[36]));
    pk->transmit_timestamp.coarse = ntohl(*(int*)&(data[40]));
    pk->transmit_timestamp.fine = ntohl(*(int*)&(data[44]));
	return 0;
}
 
int cl_ntp_get_sec1970(const char* svr)
{
	SOCKET sock;
	sockaddr_in addr,from_addr;
	socklen_t fromlen = sizeof(addr);
	string strip = svr;
	char data[NTP_PCK_LEN * 8];
	int data_len;
	char buf[2048];
	int ret = 0;
	
	if(strip.empty())
		strip = "us.ntp.org.cn";
	string ip = cl_net::ip_explain_ex(strip.c_str());
	sock = socket(AF_INET,SOCK_DGRAM,0);
	if(sock == INVALID_SOCKET)
		return 0;

	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(NTP_PORT);
	addr.sin_addr.s_addr = inet_addr(ip.c_str());

	cl_net::sock_set_timeout(sock,2000);

  //  if (!(data_len = construct_packet(data)))
  //  {
		//closesocket(sock);
  //      return 0;
  //  }

	//经测试，只发第1个字节为0x1B就可以获得时间
	data_len = NTP_PCK_LEN;
	memset(data,0,NTP_PCK_LEN);
	data[0] = 0x1b;

	//最多试4次
	for(int i=0;i<4;++i)
	{
		if(sendto(sock,data,data_len,0,(sockaddr*)&addr,sizeof(addr))<0)
			break;
		memset(&from_addr,0,sizeof(from_addr));
		ret = recvfrom(sock,buf,2048,0,(sockaddr*)&from_addr,&fromlen);
		if(ret>0)
			break;
	}
	closesocket(sock);
	if(ret<NTP_PCK_LEN) return 0;

	ntp_packet ntp_pk;
	construct_ntp_packet(buf,ret,&ntp_pk);
	//获得的是1900时间，转为1970时间 
	return ntp_pk.transmit_timestamp.coarse - SECONDS_1900_1970;
}

