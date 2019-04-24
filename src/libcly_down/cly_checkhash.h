#pragma once
#include "cl_thread.h"
#include "cl_synchro.h"
#include "cl_basetypes.h"
#include "cl_timer.h"

typedef struct tag_cly_checkhash_info
{
	string			hash;
	int				index;
	ULONGLONG		pos;
	ULONGLONG		size;
	unsigned int	res_subhash;
}cly_checkhash_info_t;

class cly_checkhash : public cl_thread
	,public cl_timerHandler
{
public:
	cly_checkhash(void);
	virtual ~cly_checkhash(void);
	typedef cl_CriticalSection Mutex;
public:
	int run();
	void end();
	virtual int work(int e);
	virtual void on_timer(int e);
	
	void add_check(const cly_checkhash_info_t& inf);
private:
	int crc32_fileblock(cly_checkhash_info_t& inf);
private:
	bool m_brun;
	Mutex m_mt;
	list<cly_checkhash_info_t> m_ls,m_ls2;
};

typedef cl_singleton<cly_checkhash> cly_checkhashSngl;
