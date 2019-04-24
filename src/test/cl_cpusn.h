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
	void Executecpuid(DWORD eax); // ����ʵ��cpuid

	DWORD m_eax;   // �洢���ص�eax
	DWORD m_ebx;   // �洢���ص�ebx
	DWORD m_ecx;   // �洢���ص�ecx
	DWORD m_edx;   // �洢���ص�edx
};

