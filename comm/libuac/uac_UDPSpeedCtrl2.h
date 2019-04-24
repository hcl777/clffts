#pragma once

#include <assert.h>
#include "uac_basetypes.h"
#include "uac_UDPConfig.h"
#include "uac_Speedometer.h"
#include "uac_UDPProtocol.h"
#include "uac_statistics.h"

//本算法只控制发送窗口，不计算速度
namespace UAC
{
#define MIN_WIN 6
#define SPEED2_CYCLE_TIMEOUT_US 250000

//********************************************************************
	
enum {LEVEL_WIN=0,LEVEL_SPEED};
class UDPSpeedCtrl2
{
public:
	UDPSpeedCtrl2(void);
	~UDPSpeedCtrl2(void);

//发送速度周期
#define CSP_LEN 64
	typedef struct tagCycleSpeedNode
	{
		char state;//
		unsigned int refer_speedB;
		unsigned int real_speedB;
		unsigned short num;
		unsigned int recv_speedB;
		unsigned short recv_num;
		int lost_rate;
		unsigned int msec; //周期用时

		tagCycleSpeedNode(void){reset();}
		void reset()
		{
			state = 0;
			refer_speedB = 0;
			real_speedB = 0;
			num = 0;
			recv_speedB = 0;
			recv_num = 0;
			lost_rate = 0;
			msec = 0;
		}
	} CycleSpeedNode_t;
	typedef struct tagCycleSpeed
	{
		unsigned int	speedB;
		unsigned int	const_speedB;
		unsigned short	max_lose_rate;
		unsigned int	bandwidth;		//如果非0,则为参考带宽

		unsigned char	speed_seq;
		ULONGLONG		begin_tick;
		ULONGLONG		end_tick;
		unsigned short	num;
		unsigned int	sizeB;
		CycleSpeedNode_t csn[CSP_LEN];	
		ULONGLONG		last_ack_utick;

		enum {CSP_FREE=0,CSP_SENDING,CSP_SENDFINI,CSP_RECVFINI};
		tagCycleSpeed(void)
		{
			bandwidth=0;
			speedB = 102400; //初始化100KB速度
			if(bandwidth > 0) speedB = bandwidth;
			if(g_uac_conf.limit_sendspeed_i > 0 && speedB > g_uac_conf.limit_sendspeed_i)
				speedB = g_uac_conf.limit_sendspeed_i;
			const_speedB = g_uac_conf.const_send_speedB;
			max_lose_rate = g_uac_conf.max_lose_rate;
			speed_seq = 0; //0不使用
			begin_tick = end_tick = 0;
			num = 0;
			sizeB = 0;
			last_ack_utick = 0;
		}
		int befor(int curr,int n)
		{
			int k = curr;
			int i=0;
			for(k=curr;k>0 && i<n;--k,++i);
			if(i==n) return k;
			for(k=CSP_LEN-1,i++;k>curr && i<n;--k,++i);
			return k;
		}
		void on_send(ULONGLONG utick,int size)
		{
			end_tick = utick;
			sizeB += size;	
			num++;
		}
		unsigned int get_lost_rate(int max)
		{
			//取最后3个发送速度的平均.
			int curr = speed_seq-1;
			if(curr<0) curr = CSP_LEN-1;
			int i,n=0;
			unsigned int all = 0;
			for(i=curr;i>=0 && n<max;--i)
			{
				if(csn[i].state==CSP_RECVFINI)
				{
					++n;
					all += csn[i].lost_rate;
				}
			}
			for(i=CSP_LEN-1;i>curr&&n<max;--i)
			{
				if(csn[i].state==CSP_RECVFINI)
				{
					++n;
					all += csn[i].lost_rate;
				}
			}
			if(n>0)
				return (unsigned int)(all/n);
			return 0;
		}

		void on_ack_speed(unsigned char i,unsigned short num,unsigned int spB) 
		{
			assert(i<CSP_LEN);
			if(i>=CSP_LEN) return;
			unsigned char ipre = 0xff,n;
			if(CSP_SENDFINI==csn[i].state)
			{
				//查找前一个有效数据:
				for(n=i-1;n>0;n--)
				{
					if(csn[n].state == CSP_RECVFINI)
					{
						ipre = n;
						break;
					}
				}
				if(0xff==ipre)
				{
					for(n = CSP_LEN-1;n>i;n--)
					{
						if(csn[n].state == CSP_RECVFINI)
						{
							ipre = n;
							break;
						}
					}
				}

				csn[i].state = CSP_RECVFINI;
				csn[i].refer_speedB = speedB;
				csn[i].recv_num = num;
				csn[i].recv_speedB = spB;
				if(csn[i].recv_num>csn[i].num)
				{
					//会出现此情况.包乱序到达时出现.
					UACLOG("#*** syc recv_num>send_num %d>%d \n",csn[i].recv_num,csn[i].num);
					csn[i].recv_num = csn[i].num;
				}
				if(0xff==ipre)
				{
					csn[i].lost_rate = 100-csn[i].recv_num*100/csn[i].num;
				}
				else
				{
					csn[i].lost_rate = 100-(csn[i].recv_num+csn[ipre].recv_num)*100/(csn[i].num+csn[ipre].num);
				}

				//UACLOG("#rcvn/n_ref,real,rcv__lr(%d/%d_%d,%d,%d)__%d%%\n",
				//	(int)csn[i].recv_num,(int)csn[i].num
				//	,(csn[i].refer_speedB>>10),(csn[i].real_speedB>>10)
				//	,(csn[i].recv_speedB>>10),csn[i].lost_rate);
				
				//UACLOG("--tick:num_r/s(%d: %d / %d) -- rcv/real/ref KB =%d / %d / %d \n"
				//	,csn[i].msec,(int)csn[i].recv_num,(int)csn[i].num
				//	,spB>>10,csn[i].real_speedB>>10,csn[i].refer_speedB>>10);

				chang_speed(i);
			}
		}
		void chang_speed(unsigned char i)
		{
			//定速时
			if(const_speedB>0)
			{
				speedB = const_speedB;
				return;
			}

			//调整发送速度,接收数据量太小的忽略
			if(csn[i].num<3 || csn[i].recv_num<2) return;
			int rate = csn[i].lost_rate;
			unsigned int tmpspeed = 0;

			if(bandwidth==0)
			{
				if(speedB<csn[i].recv_speedB)
				{
					speedB = (unsigned int)(csn[i].recv_speedB*1.2); //升为最近实际接收速度的1.2倍
				}
				else
				{
					//真实发送速度小于参考发送的65%,不调整
					if(csn[i].real_speedB < (unsigned int)(csn[i].refer_speedB*0.65))
						return;
					if(rate>max_lose_rate)
					{
						speedB = (unsigned int)(speedB*0.9); //丢包率25%降8%
						//UACLOG("-");
					}
					else
					{
						if(/*csn[i].recv_speedB>(unsigned int)(csn[i].real_speedB*0.95)
							&&*/speedB<(unsigned int)(csn[i].real_speedB*1.5))
						{
							if(rate==0)
								speedB += 50000;//(unsigned int)(speedB*0.3);
							else if(rate<max_lose_rate/3)
								speedB += 30000;//(unsigned int)(speedB*0.1); //低于1/3提速
							else if(rate<max_lose_rate*3/4)
								speedB += 10000;//(unsigned int)(speedB*0.04); //低于3/4提速
							else
								return;
							//UACLOG("+");
						}
					}
				}
			}
			else
			{
				//有参考带宽的情况:
				tmpspeed = (unsigned int)(bandwidth*1.25);
				//真实发送速度小于参考发送的65%,不调整
				if(csn[i].real_speedB < (unsigned int)(csn[i].refer_speedB*0.65))
					return;
				if(speedB<csn[i].recv_speedB)
				{
					speedB = (unsigned int)(csn[i].recv_speedB*1.2); //升为最近实际接收速度的1.2倍
				}
				else if(rate>max_lose_rate)
				{
					if(speedB>tmpspeed/3)
						speedB = (unsigned int)(speedB*0.9); //丢包率25%降8%
					//UACLOG("-");
				}
				else
				{
					if(speedB < tmpspeed)
					{
						if(rate==0)
							speedB += (tmpspeed-speedB)/2;
						else if(rate<max_lose_rate/3)
							speedB += (tmpspeed-speedB)/3;
						else if(rate<max_lose_rate*3/4)
							speedB += (tmpspeed-speedB)/4;
					}
				}
			}

			
			if(speedB<30000) speedB=30000; //最小发送速度30KB
			//限速
			if(g_uac_conf.limit_sendspeed_i > 0 && speedB > g_uac_conf.limit_sendspeed_i)
				speedB = g_uac_conf.limit_sendspeed_i;
		}

		//执行每次发送完成后调用
		void on_tick(ULONGLONG utick)
		{
			
			if(begin_tick + SPEED2_CYCLE_TIMEOUT_US<utick)
			{
				//一个周期结束
				int i = speed_seq;
				if(CSP_SENDING==csn[i].state)
				{
					unsigned int t = (unsigned int)((end_tick-begin_tick)/1000);
					//离上一次发送2秒以内才考虑要上次数据
					if(sizeB>0 && t>1 && end_tick+2000000>utick) 
					{
						csn[i].num = num;
						csn[i].real_speedB = (unsigned int)(sizeB*(1000/(double)t));
						csn[i].state = CSP_SENDFINI;
						csn[i].msec = t;
						
					}
					else
					{
						csn[i].num = 0;
						csn[i].real_speedB = 0;
						csn[i].state = CSP_FREE;
					}
				}
				else
				{
					csn[i].state = CSP_FREE;
				}
				
				//前两周期发送过数据且本周期也发.而且超过500ms没有收到过数据.则要降速.
				if(last_ack_utick + 500000 < utick && num>10)
				{
					int prei = befor(i,3);
					if(csn[prei].num > 10 && speedB>60000)
					{
						//降速.
						speedB = speedB*2/3;
					}
				}
				

				//
				speed_seq++;
				if(speed_seq>=CSP_LEN) speed_seq = 1;
				csn[speed_seq].reset();
				csn[speed_seq].state = CSP_SENDING;

				num = 0;
				begin_tick = end_tick = utick;
				sizeB = 0;
			}
		}
	}CycleSpeed_t;

public:
	
	void on_recv_ack(unsigned int tick,ULONGLONG utick)
	{
		csp.last_ack_utick = utick;
		if(LEVEL_WIN==level)
		{
			//初始阶段采用快速增长速度
			if(send_win<other_recv_win_num && last_change_win_tick+10<tick)
			{
				last_change_win_tick = tick;
				send_win++;
				//UACLOG("+");
			}
		}
	}
	
	int get_max_send_packnum(ULONGLONG utick)
	{
		if(csp.begin_tick == 0) return 1;
		int num,num1;
		unsigned int speed = csp.speedB;
		unsigned int sendsize;
		if(0==csp.const_speedB && 0==csp.bandwidth && LEVEL_WIN==level)
		{
			speed = (unsigned int)(send_win*(mtu+UDPHEAD_LENGTH) / (ttlus/(double)1000000));
			//限速
			if(g_uac_conf.limit_sendspeed_i > 0 && speed > g_uac_conf.limit_sendspeed_i)
				speed = g_uac_conf.limit_sendspeed_i;
			//限最小
			if (g_uac_conf.limit_min_sendspeed_i > 0 && speed < g_uac_conf.limit_min_sendspeed_i)
				speed = g_uac_conf.limit_min_sendspeed_i;
		}
		
		unsigned int t = (unsigned int)(utick-csp.begin_tick);
		if(t>SPEED2_CYCLE_TIMEOUT_US+1000000)
			return 1; //超期只发1个包
		num1 = statisticsSngl::instance()->get_max_sendsize()/mtu;
		sendsize = (unsigned int)(speed * (t / (double)1000000));
		num = (int)(sendsize / mtu + 1);
		if(num>csp.num)
			num -= csp.num;
		else
			num = 0;
		if(num>num1) num = num1;
		return num;
	}

	int get_win()
	{
		if(0==csp.const_speedB && LEVEL_WIN==level)
			return UAC_MIN(other_recv_win_num,send_win);
		else
			return UAC_MIN(other_recv_win_num, g_uac_conf.max_send_win_num);
	}

public:
	int				level;
	unsigned int	other_recv_win_num;
	unsigned int	send_win; //
	unsigned int	ttlus;//包的往返平均时间，微秒
	unsigned int	mtu;
	CycleSpeed_t	csp;

	unsigned int	last_change_win_tick;
	unsigned int	last_send_tick;
};

}

