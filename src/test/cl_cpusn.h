#pragma once
#include "cl_basetypes.h"

class cl_cpusn
{
public:
	cl_cpusn();
	~cl_cpusn();
public:
	bool get_sn();

private:
	void Executecpuid(DWORD eax); // 用来实现cpuid

	DWORD m_eax;   // 存储返回的eax
	DWORD m_ebx;   // 存储返回的ebx
	DWORD m_ecx;   // 存储返回的ecx
	DWORD m_edx;   // 存储返回的edx
};

