#pragma once
#include "uac_Speedometer.h"
#include "uac_Singleton.h"
#include "uac_Timer.h"
#include "uac_ntypes.h"
#include "uac.h"

namespace UAC
{

class statistics : public TimerHandler
{
public:
	statistics(void);
	~statistics(void);
public:
	int init();
	void fini();
	virtual void on_timer(int e);

	unsigned int get_max_sendsize();
	unsigned int get_max_app_recvsize();
	int get_statspeed(UAC_statspeed_t& ss);
	void on_sendfaild(int err)
	{
		m_sendfaild_count++;
		m_sendfaild_errs[m_sendfaild_i++] = err;
		m_sendfaild_i %= 8;
	}
private:
	unsigned int get_max_limit(int second_limit,int last);
public:
	Speedometer<uint64> m_sendspeed; //总发
	Speedometer<uint64> m_valid_sendspeed; //有效发
	Speedometer<uint64> m_recvspeed; //总收
	Speedometer<uint64> m_valid_recvspeed; //有效收
	Speedometer<uint64> m_app_recvspeed; //APP收
	int		m_sendfaild_count;
	int		m_sendfaild_errs[8];
	int		m_sendfaild_i;
private:
	DWORD		m_last_tick;
};
typedef Singleton<statistics> statisticsSngl;

}

