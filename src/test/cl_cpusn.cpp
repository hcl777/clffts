#include "cl_cpusn.h"


//wmic cpu get processorid

cl_cpusn::cl_cpusn()
{
}


cl_cpusn::~cl_cpusn()
{
}
bool cl_cpusn::get_sn()
{
	WORD serial[6];
	Executecpuid(1); // 执行cpuid，参数为 eax = 1
	bool isSupport = m_edx & (1 << 18); // edx是否为1代表CPU是否存在序列号
	if (false == isSupport) // 不支持，返回false
	{
		printf("*** get fail!\n");
		//return false;
	}
	memcpy(&serial[4], &m_eax, 4); // eax为最高位的两个WORD

	Executecpuid(3); // 执行cpuid，参数为 eax = 3
	memcpy(&serial[0], &m_ecx, 4); // ecx 和 edx为低位的4个WORD
	memcpy(&serial[2], &m_edx, 4); // ecx 和 edx为低位的4个WORD

	for (int i = 0; i < 6; i++)
	{
		printf("%04x", serial[i]);
	}
	return true;
}

void cl_cpusn::Executecpuid(DWORD veax)
{
	// 因为嵌入式的汇编代码不能识别 类成员变量
	// 所以定义四个临时变量作为过渡
	DWORD deax = 0;
	DWORD debx = 0;
	DWORD decx = 0;
	DWORD dedx = 0;

#ifndef ANDROID
	__asm
	{
		mov eax, veax; 将输入参数移入eax
		cpuid; 执行cpuid
		mov deax, eax; 以下四行代码把寄存器中的变量存入临时变量
		mov debx, ebx
		mov decx, ecx
		mov dedx, edx
	}
#endif

	m_eax = deax; // 把临时变量中的内容放入类成员变量
	m_ebx = debx;
	m_ecx = decx;
	m_edx = dedx;
}


