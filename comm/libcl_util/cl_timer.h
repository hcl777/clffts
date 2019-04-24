#pragma once
#include "cl_basetypes.h"
#include "cl_singleton.h"
#include "cl_synchro.h"

class cl_timerHandler
{
public:
	cl_timerHandler(void) {}
	virtual ~cl_timerHandler(void){}

	virtual void on_timer(int e){}
};



class cl_deleyQueue
{
public:
	cl_deleyQueue();
	virtual ~cl_deleyQueue();

	typedef struct tagTNode
	{
		cl_timerHandler *h;
		int e; //eventid
		ULONGLONG us; //timeout
		//DWORD next_us;//下一次执行时间
		ULONGLONG remain_us; //剩余等待时间
		tagTNode *next;
		tagTNode *pre;
		tagTNode() {reset();}
		void reset() { h=NULL;e=0;us=0;/*next_us=0;*/remain_us=0;next=pre=this;}
	}TNode_t;

public:
	ULONGLONG get_remain_us();
protected:
	void add_node(TNode_t* node);
	void del_node(TNode_t* node);
	TNode_t* get_zero_delay();
	void synchronize();

protected:
	TNode_t m_head;
	ULONGLONG m_last_tick;
	int m_size;
};

class cl_timer : public cl_deleyQueue
{
public:
	cl_timer(void);
	virtual ~cl_timer(void);
	typedef cl_CriticalSection Mutex;
	typedef cl_TLock<cl_CriticalSection> Lock;

public:
	void handle_root();
	int register_timer(cl_timerHandler *h,int e,DWORD ms);
	int register_utimer(cl_timerHandler *h,int e,ULONGLONG us);
	int unregister_timer(cl_timerHandler *h,int e);
	int unregister_all(cl_timerHandler *h);

private:
	int find(cl_timerHandler *h,int e);
	int allot();
	void free(int i);

private:
	Mutex m_mt;
	TNode_t *m_nl;
	int m_nl_size;
	int m_cursor;
	bool m_is_free;
};

typedef cl_singleton<cl_timer> cl_timerSngl;


