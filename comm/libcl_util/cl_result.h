#pragma once
#include "cl_basetime.h"
#include "cl_synchro.h"

//当函数返回 cl_result* 时，调用者不再等结果时必须release()

template<typename T>
class cl_result
{
public:
	T	val;
private:
	//初始化为2，默认要两次释放
	cl_result(void):m_ref(2),m_bset(false) {}
	~cl_result(void){}

public:
	static cl_result<T>* new_instance(){ return new cl_result<T>();}
	//成功表示值已经设置
	bool wait(int msec=-1)
	{
		DWORD end_tick = GetTickCount()+msec;
		while(!m_bset)
		{
			Sleep(10);
			if(-1!=msec && _timer_after(GetTickCount(),end_tick))
				break;
		}
		return m_bset;
	}
	void set() {m_bset = true;}
	void refer() {cl_TLock<cl_SimpleMutex> l(m_mt);m_ref++;}
	void release()
	{
		bool bdef = false;
		{
			cl_TLock<cl_SimpleMutex> l(m_mt);
			if(--m_ref<1)
				bdef = true;
		}
		if(bdef) delete this;
	}
private:
	cl_SimpleMutex m_mt;
	int m_ref;
	bool m_bset;
};

