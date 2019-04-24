#pragma once
#include "cl_rlist.h"

#include <stdlib.h>


//cycle list
#define cl_cyclist list

template<class T>
class cl_cyclist
{
public:
	cl_cyclist(void);
	cl_cyclist(const cl_cyclist<T>& ls);
	~cl_cyclist(void);
	typedef struct tagDataType{
		T data;
		rlist_head_t l;
	}DataType;

	class iterator
	{
	public:
		iterator(){}
		~iterator(){}
	public:
		//前缀++it
		iterator& operator++(){
			ptr = rlist_entry(ptr->l.next,DataType,l);
			return *this;
		}
		//后缀it++
		iterator operator++(int){
			iterator it=*this;
			ptr = rlist_entry(ptr->l.next,DataType,l);
			return it;
		}
		//前缀--it
		iterator& operator--(){
			ptr = rlist_entry(ptr->l.prev,DataType,l);
			return *this;
		}
		//后缀it--
		iterator operator--(int){
			iterator it=*this;
			ptr = rlist_entry(ptr->l.prev,DataType,l);
			return it;
		}
		bool operator==(const iterator& it);
		bool operator!=(const iterator& it);
		T& operator*()
		{
			return ptr->data;
		}

	public:
		DataType *ptr;
	};

public:
	iterator begin()
	{
		iterator it;
		it.ptr = rlist_entry(m_head->l.next,DataType,l);
		return it;
	}
	iterator end()
	{
		iterator it;
		it.ptr = m_head;
		return it;
	}
	iterator erase(iterator it)
	{
		if(it.ptr==m_head)
			return it;
		else
		{
			DataType *ptr = it.ptr;
			it.ptr = rlist_entry(it.ptr->l.next,DataType,l);
			rlist_del(&ptr->l);
			delete ptr;
			m_size--;
			return it;
		}
	}

	void clear();
	void delete_clear(); //执行delete 再clear()
	bool empty() const
	{
		return rlist_empty(&m_head->l);
	}
	unsigned int size() const { return m_size;}

	void push_back(const T& val);
	void push_front(const T& val);
	T& front();
	T& back();
	void pop_front();
	void remove(const T& val);
	void swap(cl_cyclist<T>& ls)
	{
		DataType *head = ls.m_head;
		unsigned int size = ls.m_size;
		ls.m_head = m_head;
		ls.m_size = m_size;
		m_head = head;
		m_size = size;
	}
	bool insert(const iterator& it,const T& val)
	{
		DataType *pd = new DataType();
		if(pd)
		{
			pd->data = val;
			rlist_add_tail(&pd->l,&it.ptr->l); //在此并不检查it是否属于自己的连，将数据插在it的前面
			m_size++;
			return true;
		}
		else
			return false;
	}

	cl_cyclist<T>& operator=(const cl_cyclist<T>& ls);
	void sort(bool ascending = true);//true 表示从小到大

private:
	DataType *m_head;
	unsigned int m_size;
};


/////////////////////////////////////////////////////////////////////////////


template<class T>
bool cl_cyclist<T>::iterator::operator ==(const iterator& it)
{
	return (ptr==it.ptr);
}
template<class T>
bool cl_cyclist<T>::iterator::operator !=(const iterator& it)
{
	return (ptr!=it.ptr);
}

//***********************************************
template<class T>
cl_cyclist<T>::cl_cyclist(void)
{
	m_head = new DataType();
	m_size = 0;
	rlist_init(&m_head->l);
}

template<class T>
cl_cyclist<T>::cl_cyclist(const cl_cyclist<T>& ls)
{
	m_head = new DataType();
	m_size = 0;
	rlist_init(&m_head->l);
	operator=(ls);
}

template<class T>
cl_cyclist<T>::~cl_cyclist(void)
{
	clear();
	delete m_head;
}


//template<class T>
//cl_cyclist<T>::iterator cl_cyclist<T>::begin()
//{
//	iterator it;
//	it.ptr = rlist_entry(m_head->l.next,DataType,l);
//	return it;
//}
//
//template<class T>
//cl_cyclist<T>::iterator cl_cyclist<T>::end()
//{
//	iterator it;
//	it.ptr = m_head;
//	return it;
//}
//
//template<class T>
//cl_cyclist<T>::iterator cl_cyclist<T>::erase(iterator& it)
//{
//	if(it->ptr==m_head)
//		return it;
//	else
//	{
//		iterator it2;
//		it2.ptr = rlist_entry(it.ptr->l.next,DataType,l);
//		rlist_del(it.ptr->l);
//		delete it.ptr;
//		m_size--;
//		it = it2;
//		return it;
//	}
//}

template<class T>
void cl_cyclist<T>::clear()
{
	DataType *data = NULL;
	rlist_head_t *pos,*next;
	rlist_for_each_safe(pos,next,&m_head->l)
	{
		data = rlist_entry(pos,DataType,l);
		delete data; 
		m_size--;
	}
	rlist_init(&m_head->l);
}
template<class T>
void cl_cyclist<T>::delete_clear()
{
	DataType *data = NULL;
	rlist_head_t *pos,*next;
	rlist_for_each_safe(pos,next,&m_head->l)
	{
		data = rlist_entry(pos,DataType,l);
		delete data->data;
		delete data; 
		m_size--;
	}
	rlist_init(&m_head->l);
}

template<class T>
void cl_cyclist<T>::push_back(const T& val)
{
	DataType *pd = new DataType();
	if(pd)
	{
		pd->data = val;
		rlist_add_tail(&pd->l,&m_head->l);
		m_size++;
	}
}

template<class T>
void cl_cyclist<T>::push_front(const T& val)
{
	DataType *pd = new DataType();
	if(pd)
	{
		pd->data = val;
		rlist_add_head(&pd->l,&m_head->l);
		m_size++;
	}
}
template<class T>
T& cl_cyclist<T>::front()
{
	return rlist_entry(m_head->l.next,DataType,l)->data;
}
template<class T>
T& cl_cyclist<T>::back()
{
	return rlist_entry(m_head->l.prev,DataType,l)->data;
}
template<class T>
void cl_cyclist<T>::pop_front()
{
	if(!rlist_empty(&m_head->l))
	{
		DataType *pd = rlist_entry(m_head->l.next,DataType,l);
		rlist_del(&pd->l);
		m_size--;
		delete pd;
	}
}
template<class T>
void cl_cyclist<T>::remove(const T& val)
{
	DataType *pd = NULL;
	rlist_head_t *pos,*next;
	rlist_for_each_safe(pos,next,&m_head->l)
	{
		pd = rlist_entry(pos,DataType,l);
		if(pd->data == val)
		{
			rlist_del(&pd->l);
			m_size--;
			delete pd; 
		}
	}
}

template<class T>
cl_cyclist<T>& cl_cyclist<T>::operator=(const cl_cyclist<T>& ls)
{
	clear();
	DataType *pd = NULL;
	rlist_head_t *pos;
	rlist_for_each(pos,&ls.m_head->l)
	{
		pd = rlist_entry(pos,DataType,l);
		push_back(pd->data);
	}
	//cl_cyclist<T>& tmp = (cl_cyclist<T>&)ls;
	//cl_cyclist<T>::iterator it;
	//for(it=tmp.begin(); it!=tmp.end();++it)
	//	push_back((*it));
	return *this;
}

template<class T>
void cl_cyclist<T>::sort(bool ascending/* = true*/)
{
	cl_cyclist<T> ls;
	iterator it1, it2;
	swap(ls);
	for (it1 = ls.begin(); it1 != ls.end(); ++it1)
	{
		for (it2 = begin(); it2 != end(); ++it2) 
		{
			if ((ascending && (*it1) < (*it2)) || (!ascending && (*it1) > (*it2)))
				break;
		}
		insert(it2, *it1);
	}
}

//***********************************************
//
//template<class T>
//static void cl_delete_list(cl_cyclist<T>& ls)
//{
//	cl_cyclist<T>::iterator it;
//	for(it=ls.begin();it!=ls.end();++it)
//	{
//		delete (T)(*it);
//	}
//	ls.clear();
//}
//
