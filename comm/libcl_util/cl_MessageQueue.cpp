#include "cl_MessageQueue.h"

#include <assert.h>


cl_MessageQueue::cl_MessageQueue(void)
{
	m_size=0;
	rlist_init(&m_head);
}

cl_MessageQueue::~cl_MessageQueue(void)
{
	ClearMessage();
	assert(0==m_size);
}

cl_Message* cl_MessageQueue::GetMessage(unsigned int millsec/*=INFINITE*/)
{
	if(0==millsec && m_size==0)
		return NULL;
	if(!m_sem.wait(millsec))
		return NULL;
	{
		Lock l(m_mt);
		if(rlist_empty(&m_head))
		{
			return NULL;
		}
		cl_Message *msg=NULL;
		MessageNode *node = NULL;
		node = rlist_entry(m_head.next,MessageNode,leaf);
		msg = node->msg;
		rlist_del(m_head.next);
		delete node;
		m_size--;

		return msg;
	}
}
void cl_MessageQueue::AddMessage(cl_Message *msg)
{
	{
		Lock l(m_mt);
		MessageNode *node = new MessageNode(msg);
		rlist_add_tail(&node->leaf,&m_head);
		m_size++;
	}
	m_sem.signal();
}
void cl_MessageQueue::SendMessage(cl_Message *msg)
{
	cl_Semaphore sem;
	msg->psem = &sem;
	AddMessage(msg);
	sem.wait();
}
void cl_MessageQueue::ClearMessage(void (*CLEAR_MESSAGE_FUNC)(cl_Message*)/* = 0*/)
{
	{
		Lock l(m_mt);
		MessageNode *node;
		rlist_head_t *pos,*next;
		rlist_for_each_safe(pos,next,&m_head)
		{
			node = rlist_entry(pos,MessageNode,leaf);
			if(CLEAR_MESSAGE_FUNC)
				CLEAR_MESSAGE_FUNC(node->msg);
			delete node; //这里删除链leaf，只要不再使用pos就是安全的，在后面rlist_init()  ok
			m_size--;
		}
		rlist_init(&m_head);
	}
	m_sem.signal();
}

