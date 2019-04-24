#pragma once

#include "cl_rlist.h"
#include "cl_synchro.h"


typedef struct tagMessage
{
	int		cmd;
	void*	data;
	cl_Semaphore*  psem; //如果用sendmsg等执行完，则赋值
	tagMessage(void):cmd(0),data(NULL),psem(0) {}
	tagMessage(int aCmd, void* aData)
		:cmd(aCmd),data(aData),psem(NULL)
	{}
	~tagMessage(void){ if(psem) psem->signal();} //等消息被删除时sendmsg（）才返回
}cl_Message;

class cl_MessageQueue
{
public:
	cl_MessageQueue(void);
	~cl_MessageQueue(void);

	typedef struct tagMessageNode
	{
		cl_Message *msg;
		rlist_head_t leaf;
		tagMessageNode(cl_Message *aMsg) : msg(aMsg) {}
	}MessageNode;

	typedef cl_SimpleMutex Mutex;
	typedef cl_TLock<Mutex> Lock;

public:
	cl_Message* GetMessage(unsigned int millsec=INFINITE);
	void AddMessage(cl_Message *msg);
	void SendMessage(cl_Message *msg); //等消息被处理并且被删除再返回
	void ClearMessage(void (*CLEAR_MESSAGE_FUNC)(cl_Message*) = 0);
	unsigned int Size()const {return m_size;}
private:
	rlist_head_t	m_head;
	int				m_size;
	cl_Semaphore	m_sem;
	Mutex			m_mt;
};

