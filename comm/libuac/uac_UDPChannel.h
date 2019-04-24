#pragma once
#include "uac_Channel.h"
#include "uac_Timer.h"
#include "uac_UDPConnector.h"
#include "uac_Speedometer.h"
#include "uac_UDPSpeedCtrl2.h"
#include "uac_cyclist.h"
#include "uac_rbtmap.h"

namespace UAC
{

typedef struct tagUDPSSendPacket
{
	int send_num;
	int send_count;
	int	nak_count;			//预计丢失计数
	ULONGLONG last_send_tick; //用于记录开始发送包时间，当收到ACK时，可以统计该包一个来回用时多少
	memblock *block;//带head
	tagUDPSSendPacket(void) : send_num(0),send_count(0),nak_count(0),last_send_tick(0),block(0) {}
}UDPSSendPacket_t;

typedef struct tagUDPSRecvPacket
{
	int recv_num;
	ULONGLONG recv_utick; //接收时间
	int ack_count; //回复ACK次数,如果收到重复包,将计划置0
	memblock *block;//不带head
	tagUDPSRecvPacket(void) : recv_num(0),recv_utick(0),ack_count(0),block(0) {}
}UDPSRecvPacket_t;
//
//typedef struct tagResendRate
//{
//	unsigned int valid_send_size;
//	unsigned int resend_size;
//	tagResendRate(void) : valid_send_size(0),resend_size(0){}
//	void reset(){valid_send_size=0;resend_size=0;}
//	unsigned int rate() 
//	{
//		if(valid_send_size)
//			return resend_size*100/valid_send_size;
//		if(resend_size)
//			return 100;
//		return 0;
//	}
//}ResendRate_t;

typedef struct tag_SecondSendInfo
{
	//UAC_sendspeed_t si;
	unsigned int speedB;
	unsigned int lost_rate; //%分比
	unsigned int rerecv_rate; //%分比
	unsigned int seq_range; //发送最大seq与最小seq的差.
	unsigned int second_maxttl_ms; //ms

	unsigned int last_sendB;
	tag_SecondSendInfo(void)
	{
		speedB = 0;
		lost_rate = 0;
		rerecv_rate = 0;
		seq_range = 0;
		second_maxttl_ms = 0;

		last_sendB = 0;
	}
	void on_second()
	{
		speedB = last_sendB;
		last_sendB = 0;
	}
}SecondSendInfo_t;
typedef struct tag_SecondRecvInfo
{
	UAC_recvspeed_t si;

	unsigned int last_recvB;
	unsigned int last_recv_num;
	unsigned int last_rerecv_num;
	tag_SecondRecvInfo(void)
	{
		memset(&si,0,sizeof(si));

		last_recvB = 0;
		last_recv_num = 0;
		last_rerecv_num = 0;
	}
	void on_second()
	{
		si.speedB = last_recvB;
		if(last_recv_num)
			si.rerecv_rate = last_rerecv_num*100/last_recv_num;
		else
			si.rerecv_rate = 0;

		last_recvB = 0;
		last_recv_num = 0;
	}
}SecondRecvInfo_t;

/******************************
//超时算法:
每1ms周期计算一次大概重收率.
如果最近1次重收率大于5%,则维持4秒超时翻倍.
*******************************/

typedef struct tagUDPSSendWin
{
	int win_low_line;//窗口下界，对方未确认的下界，即对方想要的下一个数据
	unsigned int ack_sequence;
	unsigned int nak_num;
	int send_num;//下次发送新包使用的序列号
	int real_send_num;//真实已经发送到的位置，可能上层多发，暂存队列
	unsigned int ttl; 
	UDPSpeedCtrl2 spctrl;
	
	ArrRuler<unsigned int,10> ttls; //回路TTL
	
	unsigned int resend_timeout_mod; //重传因子

	UAC_sendspeed_t ss;

	unsigned int	second_allsendB; //秒发量
	unsigned int	second_maxttl; //秒内最大TTL
	unsigned int	last_rerecv_pks; //用于计算对方秒重收量

	unsigned char not_more_data_count; //无持续增加发送数据时，超时重发周期变短

	
	void reset()
	{
		win_low_line=0;
		ack_sequence=0;
		nak_num=0;
		send_num=0;
		real_send_num=-1;
		ttl = 100000; //

		resend_timeout_mod=3;

		second_allsendB = 0;
		second_maxttl = 0;
		last_rerecv_pks = 0;

		not_more_data_count = 0;
	}
	tagUDPSSendWin(void){ reset();}
	void on_second()
	{
		ss.speedB = second_allsendB; 
		ss.second_maxttl_ms = second_maxttl/1000; 
		ss.wind_pks = real_send_num - win_low_line + 1;
		ss.second_rerecv_pks = ss.other_rerecv_pks - last_rerecv_pks;

		second_allsendB = 0;
		second_maxttl = ttl;
		last_rerecv_pks = ss.other_rerecv_pks;

		if (resend_timeout_mod < 5 && ss.second_rerecv_pks>0)
			resend_timeout_mod++;
	}
}UDPSSendWin_t;

typedef struct tagUDPSRecvWin
{
	//recv win
	int					win_low_line;//窗口下界，未确认的下界
	int					recv_win_num; //考虑减掉被未应用层接收的数量
	bool				brecv_win_num_changed; //相对回复ACK,接收窗更新时为true,回复后变为FALSE
	unsigned int		max_recv_win_num; //最多可以接收多少个包
	int					max_sequence_num; //收到的最大包序号
	int					rerecv_num;//重收了多少个包
	unsigned int		other_ttlms; //对方的,目前只记录来方便上层查看.
	unsigned int		other_maxttlms; //最近时间内最大TTL

	//*************************************
	//ACK:
	UDPSAck_t			ack;	//
	unsigned int		ack_sequence; //
	unsigned int		last_ack_tick;
	unsigned int		unack_recv_num; //未回复ACK前收到的包数
	int					last_ack_win_low_line; //相对回复ACK,
	int					resend_ack_win_low_line_count; //重复回复同一个low_line计数,当变化或者有收到更新的包时置0
	////结构用于ACK回复计算ttl，当没有ACK可回复时，则回复一个，有则不能再回复这个，否则序号不顺序递增
	struct {
		ULONGLONG utick;
		unsigned int seq;
	}					last_pack;  
	//=====================================

	SecondRecvInfo_t			sri;
	Speedometer<unsigned int>	speed;
	uint64						recv_sizeB;		//一共收了多少数据

	//*************************************
	//速度统计周期,相同编号的包统计速度,速度计算丢掉第一个包,数据包含第一个包
	struct tagCycleSpeed{
		unsigned char	speed_seq;
		ULONGLONG		begin_tick;
		ULONGLONG		end_tick;
		unsigned short	num;
		unsigned int	sizeB;
		unsigned char	last_speed_seq;
		unsigned short	last_num;
		unsigned int	last_speedB;

		tagCycleSpeed(void)
			:speed_seq(0)
			,begin_tick(0)
			,end_tick(0)
			,num(0)
			,sizeB(0)
			,last_speed_seq(0)
			,last_num(0)
			,last_speedB(0)
		{}
		void on_recv(ULONGLONG utick,unsigned char seq,unsigned int size)
		{
			//注意 seq=0 无效,空闲
			if(seq!=speed_seq)
			{
				//统计速度然后初始化
				unsigned int t = (unsigned int)((end_tick-begin_tick)/1000);
				//离上一次接收间隔不到2秒,这样上一次的数据才考虑要
				if(speed_seq>0 && sizeB>0 && t>1 && end_tick+2000000>utick)
				{
					last_speed_seq = speed_seq;
					last_num = num;
					last_speedB = (unsigned int)(sizeB*(1000/(double)t));
				}
				else
				{
					last_speed_seq = 0;
					last_num = 0;
					last_speedB = 0;
				}
				begin_tick = end_tick = utick;
				speed_seq = seq;
				num = 1; //统计接收数据时包含
				sizeB = 0; //第1个包不作速度统计
			}
			else if(0!=seq)
			{
				end_tick = utick;
				num++;
				sizeB += size;
			}
		}
	}	csp; //cycle speed
	//=====================================

	void reset() 
	{ 
		win_low_line=0;
		last_ack_win_low_line=-1;
		resend_ack_win_low_line_count=0;
		brecv_win_num_changed = true;
		max_recv_win_num = g_uac_conf.max_recv_win_num;
		recv_win_num = max_recv_win_num;
		last_ack_tick = 0;
		ack_sequence=0;
		max_sequence_num = -1;
		rerecv_num=0;
		other_ttlms = 0;
		other_maxttlms = 0;
		ack.size = 0;
		unack_recv_num=0;
		recv_sizeB = 0;
		last_pack.utick = 0;
		last_pack.seq = 0;
	}
	tagUDPSRecvWin(void) {reset();}
}UDPSRecvWin_t;


//***********************************************************************************

class UDPChannel : public Channel,public UDPChannelHandler
	,public TimerHandler
{

public:
	UDPChannel(UDPConnector* ctr,int idx);
	virtual ~UDPChannel(void);
	typedef list<UDPSSendPacket_t> SendList;
	typedef SendList::iterator SendIter;
	typedef map<int,UDPSRecvPacket_t> RecvMap;
	typedef RecvMap::iterator RecvIter;
public:
	//Channel
	virtual int attach(SOCKET s,sockaddr_in& addr);
	virtual int connect(const char* ip,unsigned short port,int nattype=0);
	virtual int connect(unsigned int ip,unsigned short port,int nattype=0);
	virtual int disconnect();
	virtual int send(memblock *b,bool more=false);  //-1:false; 0:send ok; 1:put int sendlist
	virtual int recv(char *b,int size){assert(false);return 0;}
	virtual int set_bandwidth(int size);
	virtual int set_recvbuf(int size);
	virtual UAC_sendspeed_t& get_sendspeed(){return m_send_win.ss;}
	virtual UAC_recvspeed_t& get_recvspeed(){return m_recv_win.sri.si;}
	void update_recv_win_num(uint32 cache_block_num);

	//UDPChannelHandler
	virtual int handle_connected();
	virtual int handle_disconnected();
	virtual int send_to(char *buf,int len,ULONGLONG utick);
	virtual int handle_send(bool roolcall=false);
	virtual int handle_recv(memblock* b);

	UDPChannelHandler* get_udpchannel_handler();

	//TimerHandler
	virtual void on_timer(int e);
private:
	virtual void reset();
	void on_connected();
	void on_disconnected();
	void on_data(memblock* b);
	void on_writable();

	int handle_recv_data(PTLStream& ps,memblock* b);
	int handle_recv_ack(PTLStream& ps);
	void send_ack(DWORD tick,ULONGLONG utick);
	void on_timer_ack();
	void on_timer_second();

private:
	UDPConnector*		m_ctr;
	//int m_smore; //recode last fire writable if call send();
	UDPSSendWin_t		m_send_win;
	UDPSRecvWin_t		m_recv_win;
	SendList			m_send_list;
	RecvMap				m_recv_map;
	bool				m_bsend_timer; //是否设了发送定时，用于没数据可发时取消定时
	unsigned int		m_send_list_max_size;
	unsigned int		m_begin_tick;
	unsigned char		m_des_nattype;

	char *_tmp_cmdbuf;
	PTLStream _tmp_ps,_tmp_recv_ps;
	UDPSConnHeader_t _tmp_head;
	uchar		_tmp_cmd,_tmp_recv_cmd;
	UDPSData_t _tmp_send_data,_tmp_recv_data;
	UDPSAck_t _tmp_ack;
};

}

