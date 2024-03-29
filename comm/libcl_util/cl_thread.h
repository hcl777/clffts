#pragma once

class cl_thread;
typedef struct tagThreadData
{
	cl_thread *thr;
	int e;
	tagThreadData(cl_thread *athr,int ae):thr(athr),e(ae){}
}ThreadData_t;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN		// 从 Windows 头中排除极少使用的资料
#include <windows.h>

class cl_thread
{
public:
	cl_thread(void);
	virtual ~cl_thread(void);
public:
	int activate(int n=1);
	int wait(DWORD milliseconds=INFINITE);
	
	virtual int work(int e); //e指明是第几条线程，从0算起
	static DWORD WINAPI _work_T(LPVOID p);
protected:
	HANDLE *m_handle;
	DWORD *m_thrId;
	int m_nThr;
};

#else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

class cl_thread
{
public:
	cl_thread(void);
	virtual ~cl_thread(void);
public:
	int activate(int n=1);
	int wait();
	
	virtual int work(int e);
	static void* _work_T(void* p);
	int get_work_i(); //定义多条线程从0开始编号，0表示第1条，返回的就是这个值
private:
	static int _icreate_state;
protected:
	pthread_t *m_handle;
	int m_nThr;
};

#endif

