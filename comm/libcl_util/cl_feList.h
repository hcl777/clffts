#pragma once

//双链表,一个链表为已填数据链表full_list,一个为空闲链表empty_list;
//

#include "cl_cyclist.h"
#include "cl_synchro.h"

template<typename FENode>
class cl_feList
{
public:
	cl_feList(void){}
	~cl_feList(void){}

public:
	bool empty()const{ return (m_fulls.empty()&&m_emptys.empty());}
	FENode get_full_node()
	{
		cl_TLock<cl_CriticalSection> l(m_cs);
		if(m_fulls.empty())
			return NULL;
		FENode n = m_fulls.front();
		m_fulls.pop_front();
		return n;
	}
	int get_full_size() const
	{
		return m_fulls.size();
	}
	FENode get_empty_node()
	{
		cl_TLock<cl_CriticalSection> l(m_cs);
		if(m_emptys.empty())
			return NULL;
		FENode n = m_emptys.front();
		m_emptys.pop_front();
		return n;
	}
	int get_empty_size() const
	{
		return m_emptys.size();
	}
	void put_full_node(FENode n)
	{
		cl_TLock<cl_CriticalSection> l(m_cs);
		m_fulls.push_back(n);
	}
	void put_empty_node(FENode n)
	{
		cl_TLock<cl_CriticalSection> l(m_cs);
		m_emptys.push_back(n);
	}

private:
	list<FENode> m_fulls;
	list<FENode> m_emptys;
	cl_CriticalSection m_cs;
};
