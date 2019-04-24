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
	int	nak_count;			//Ԥ�ƶ�ʧ����
	ULONGLONG last_send_tick; //���ڼ�¼��ʼ���Ͱ�ʱ�䣬���յ�ACKʱ������ͳ�Ƹð�һ��������ʱ����
	memblock *block;//��head
	tagUDPSSendPacket(void) : send_num(0),send_count(0),nak_count(0),last_send_tick(0),block(0) {}
}UDPSSendPacket_t;

typedef struct tagUDPSRecvPacket
{
	int recv_num;
	ULONGLONG recv_utick; //����ʱ��
	int ack_count; //�ظ�ACK����,����յ��ظ���,���ƻ���0
	memblock *block;//����head
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
	unsigned int lost_rate; //%�ֱ�
	unsigned int rerecv_rate; //%�ֱ�
	unsigned int seq_range; //�������seq����Сseq�Ĳ�.
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
//��ʱ�㷨:
ÿ1ms���ڼ���һ�δ��������.
������1�������ʴ���5%,��ά��4�볬ʱ����.
*******************************/

typedef struct tagUDPSSendWin
{
	int win_low_line;//�����½磬�Է�δȷ�ϵ��½磬���Է���Ҫ����һ������
	unsigned int ack_sequence;
	unsigned int nak_num;
	int send_num;//�´η����°�ʹ�õ����к�
	int real_send_num;//��ʵ�Ѿ����͵���λ�ã������ϲ�෢���ݴ����
	unsigned int ttl; 
	UDPSpeedCtrl2 spctrl;
	
	ArrRuler<unsigned int,10> ttls; //��·TTL
	
	unsigned int resend_timeout_mod; //�ش�����

	UAC_sendspeed_t ss;

	unsigned int	second_allsendB; //�뷢��
	unsigned int	second_maxttl; //�������TTL
	unsigned int	last_rerecv_pks; //���ڼ���Է���������

	unsigned char not_more_data_count; //�޳������ӷ�������ʱ����ʱ�ط����ڱ��

	
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
	int					win_low_line;//�����½磬δȷ�ϵ��½�
	int					recv_win_num; //���Ǽ�����δӦ�ò���յ�����
	bool				brecv_win_num_changed; //��Իظ�ACK,���մ�����ʱΪtrue,�ظ����ΪFALSE
	unsigned int		max_recv_win_num; //�����Խ��ն��ٸ���
	int					max_sequence_num; //�յ����������
	int					rerecv_num;//�����˶��ٸ���
	unsigned int		other_ttlms; //�Է���,Ŀǰֻ��¼�������ϲ�鿴.
	unsigned int		other_maxttlms; //���ʱ�������TTL

	//*************************************
	//ACK:
	UDPSAck_t			ack;	//
	unsigned int		ack_sequence; //
	unsigned int		last_ack_tick;
	unsigned int		unack_recv_num; //δ�ظ�ACKǰ�յ��İ���
	int					last_ack_win_low_line; //��Իظ�ACK,
	int					resend_ack_win_low_line_count; //�ظ��ظ�ͬһ��low_line����,���仯�������յ����µİ�ʱ��0
	////�ṹ����ACK�ظ�����ttl����û��ACK�ɻظ�ʱ����ظ�һ�����������ٻظ������������Ų�˳�����
	struct {
		ULONGLONG utick;
		unsigned int seq;
	}					last_pack;  
	//=====================================

	SecondRecvInfo_t			sri;
	Speedometer<unsigned int>	speed;
	uint64						recv_sizeB;		//һ�����˶�������

	//*************************************
	//�ٶ�ͳ������,��ͬ��ŵİ�ͳ���ٶ�,�ٶȼ��㶪����һ����,���ݰ�����һ����
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
			//ע�� seq=0 ��Ч,����
			if(seq!=speed_seq)
			{
				//ͳ���ٶ�Ȼ���ʼ��
				unsigned int t = (unsigned int)((end_tick-begin_tick)/1000);
				//����һ�ν��ռ������2��,������һ�ε����ݲſ���Ҫ
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
				num = 1; //ͳ�ƽ�������ʱ����
				sizeB = 0; //��1���������ٶ�ͳ��
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
	bool				m_bsend_timer; //�Ƿ����˷��Ͷ�ʱ������û���ݿɷ�ʱȡ����ʱ
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
