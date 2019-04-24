#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN		// 从 Windows 头中排除极少使用的资料
#include <windows.h>

class cl_CriticalSection
{
public:
	cl_CriticalSection()
	{
		::InitializeCriticalSection(&cs);
	}
	~cl_CriticalSection()
	{
		::DeleteCriticalSection(&cs);
	}
	void lock()
	{
		::EnterCriticalSection(&cs);
	}
	void unlock()
	{
		::LeaveCriticalSection(&cs);
	}

private:
	cl_CriticalSection(const cl_CriticalSection*);
	cl_CriticalSection& operator=(const cl_CriticalSection&);

private:
	CRITICAL_SECTION cs;
};
typedef cl_CriticalSection cl_SimpleMutex;
typedef cl_CriticalSection cl_RecursiveMutex;

class cl_Semaphore
{
public:
	cl_Semaphore() 
	{
		//no signle init;
		h = CreateSemaphore(NULL,0,MAXLONG,NULL);
	}
	~cl_Semaphore() 
	{
		CloseHandle(h);
	}
	int signal() 
	{
		return ReleaseSemaphore(h,1,NULL);
	}
	bool wait() 
	{
		return WAIT_OBJECT_0 == WaitForSingleObject(h,INFINITE) ;
	}
	bool wait(unsigned int millis) 
	{
		return WAIT_OBJECT_0 == WaitForSingleObject(h,millis);
	}


private:
	cl_Semaphore(const cl_Semaphore&);
	cl_Semaphore& operator=(const cl_Semaphore&);
private:
	HANDLE h;
};

#else
//linux:
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

class cl_SimpleMutex
{
public:
	cl_SimpleMutex(void) { pthread_mutex_init(&m_mt,NULL);}
	~cl_SimpleMutex(void) { pthread_mutex_destroy(&m_mt);}
public:
	int lock(){ return pthread_mutex_lock(&m_mt);}
	int unlock(){ return pthread_mutex_unlock(&m_mt);}

private:
	cl_SimpleMutex(const cl_SimpleMutex&);
	cl_SimpleMutex& operator= (const cl_SimpleMutex&);
private:
	pthread_mutex_t m_mt;
};

class cl_RecursiveMutex
{
public:
	cl_RecursiveMutex(void) { 
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE); //PTHREAD_MUTEX_RECURSIVE_NP
		pthread_mutex_init(&m_mt,&attr);
		pthread_mutexattr_destroy(&attr);
	}
	~cl_RecursiveMutex(void) { pthread_mutex_destroy(&m_mt);}
public:
	int lock(){ return pthread_mutex_lock(&m_mt);}
	int unlock(){ return pthread_mutex_unlock(&m_mt);}

private:
	cl_RecursiveMutex(const cl_RecursiveMutex&);
	cl_RecursiveMutex& operator= (const cl_RecursiveMutex&);
private:
	pthread_mutex_t m_mt;
};

typedef cl_RecursiveMutex cl_CriticalSection;

#ifndef INFINITE
#define INFINITE            0xFFFFFFFF  // Infinite timeout
#endif

#ifndef _OS
class cl_Semaphore
{
public:
	//不进行共享(参数2为0),初值为无信号(参数3为0)
	cl_Semaphore(void) {sem_init(&m_sem,0,0);}
	~cl_Semaphore(void) {sem_destroy(&m_sem);}
public:
	int signal() { return sem_post(&m_sem);}
	bool wait() { return 0==sem_wait(&m_sem);}
	bool trywait() {return 0==sem_trywait(&m_sem);} //非阻塞,-1表未获得信号,0获得信号
	bool wait(unsigned int millis) {
		if(INFINITE==millis) 
			return wait(); 
		else 
			return trywait(); //todo: 暂时只支持只要不无限等待，即try
	}
private:
	cl_Semaphore(const cl_Semaphore&);
	cl_Semaphore& operator= (const cl_Semaphore&);
private:
	sem_t m_sem;
};
#endif //_OS
#endif


template<class T>
class cl_TLock
{
public:
	cl_TLock(T& at)
		: t(at)
	{
		t.lock();
	}
	~cl_TLock()
	{
		t.unlock();
	}
private:
	cl_TLock& operator=(const cl_TLock&);
private:
	T& t;
};
