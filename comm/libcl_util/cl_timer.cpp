#include "cl_timer.h"
#include <assert.h>

//*******************************QueueDelay*******************************
cl_deleyQueue::cl_deleyQueue()
:m_last_tick(0)
,m_size(0)
{
	m_head.remain_us = (ULONGLONG)-1l;
}
cl_deleyQueue::~cl_deleyQueue()
{
	assert(0==m_size);
	assert(&m_head==m_head.next);
}

ULONGLONG cl_deleyQueue::get_remain_us()
{
	return m_head.next->remain_us;
}
void cl_deleyQueue::add_node(TNode_t* node)
{
	assert(node==node->next);
	synchronize();
	node->remain_us = node->us;
	TNode_t *next = m_head.next;
	while(next!=&m_head && node->remain_us > next->remain_us)
	{
		node->remain_us -= next->remain_us;
		next = next->next;
	}
	if(next!=&m_head)
		next->remain_us -= node->remain_us;
	//inset befor next;
	assert(next->pre->next==next);
	node->next = next;
	node->pre = next->pre;
	next->pre = node->pre->next = node;
	m_size++;
}
void cl_deleyQueue::del_node(TNode_t* node)
{
	assert(node!=node->next);
	if(node->next!=&m_head)
		node->next->remain_us += node->remain_us;
	node->pre->next = node->next;
	node->next->pre = node->pre;
	node->pre = node->next = node;
	m_size--;
}
cl_deleyQueue::TNode_t* cl_deleyQueue::get_zero_delay()
{
	if(m_head.next->remain_us==0)
		return m_head.next;
	return NULL;
}
void cl_deleyQueue::synchronize()
{
	ULONGLONG tick = GetUTickCount();
	//重置
	if(tick<m_last_tick)
	{
		m_last_tick = tick;
		return;
	}
	ULONGLONG tmp = tick - m_last_tick;
	m_last_tick = tick;
	TNode_t *next = m_head.next;
	while(next!=&m_head && tmp > next->remain_us)
	{
		tmp -= next->remain_us;
		next->remain_us = 0;
		next = next->next;
	}
	if(next!=&m_head)
		next->remain_us -= tmp;
}

//*******************************cl_timer**************************************

cl_timer::cl_timer(void)
{
	m_nl_size = 2048;
	m_nl = new TNode_t[m_nl_size];
	m_cursor = 0;
	m_is_free = false;
}

cl_timer::~cl_timer(void)
{
	assert(0==m_cursor);
	delete[] m_nl;
}

void cl_timer::handle_root()
{
	Lock l(m_mt);
	//DWORD tick = GetUTickCount();
	//int i=0;
	//while(i<m_cursor)
	//{
	//	for(;i<m_cursor;++i)
	//	{
	//		if(m_nl[i].next_us<=tick)
	//		{
	//			//DEBUGMSG("#...cl_timer(tick:%d / nextt:%d / h:%d / id:%d / us:%d)\n",(unsigned int)tick,(unsigned int)m_nl[i].next_us,(unsigned int)m_nl[i].h,m_nl[i].e,m_nl[i].us);
	//			m_is_free = false;
	//			m_nl[i].h->on_timer(m_nl[i].e);
	//			//如果回调过程中有删除,跳出重跑
	//			if(m_is_free)
	//				break;
	//			m_nl[i].next_us += m_nl[i].us;
	//		}
	//	}
	//}
	synchronize();
	TNode_t *node;
	while((node=get_zero_delay()))
	{
		node->h->on_timer(node->e); //on_timer()中可能执行del_node(),并且再执行add_node()
		if(node->h)
		{
			del_node(node);
			add_node(node);
		}
	}

}
int cl_timer::register_timer(cl_timerHandler *h,int e,DWORD ms)
{
	return register_utimer(h,e,((ULONGLONG)ms)*1000l);
}
int cl_timer::register_utimer(cl_timerHandler *h,int e,ULONGLONG us)
{
	Lock l(m_mt);
	if(NULL==h || us<=0)
		return -1;
	int i = find(h,e);
	if(-1==i)
		i = allot();
	else
		del_node(&m_nl[i]);
	if(-1==i)
		return -1;
	m_nl[i].h = h;
	m_nl[i].e = e;
	m_nl[i].us = us;
	//m_nl[i].next_us = GetUTickCount()+us;
	add_node(&m_nl[i]);
	return 0;
}
int cl_timer::unregister_timer(cl_timerHandler *h,int e)
{
	Lock l(m_mt);
	int i = find(h,e);
	if(-1!=i)
	{
		del_node(&m_nl[i]);
		free(i);
		return 0;
	}
	else
		return -1;
}
int cl_timer::unregister_all(cl_timerHandler *h)
{
	Lock l(m_mt);
	int i=0;
	while(i<m_cursor)
	{
		for(;i<m_cursor;++i)
		{
			if(m_nl[i].h==h)
			{
				del_node(&m_nl[i]);
				free(i);//m_cursor值有改变，并且此是i不能递增
				break;
			}
		}
	}
	return 0;
}

int cl_timer::find(cl_timerHandler *h,int e)
{
	for(int i=0;i<m_cursor;++i)
	{
		if(m_nl[i].h==h && m_nl[i].e==e)
			return i;
	}
	return -1;
}
int cl_timer::allot()
{
	if(m_cursor>=m_nl_size)
		return -1;
	return m_cursor++; //注意测试是否为预定结果
}
void cl_timer::free(int i)
{
	m_is_free = true;
	if(i!=m_cursor-1)
	{
		m_nl[i] = m_nl[m_cursor-1];
		m_nl[i].next->pre = m_nl[i].pre->next = &m_nl[i]; //链节点也要移动
	}
	m_nl[m_cursor-1].reset();
	m_cursor--;
}

