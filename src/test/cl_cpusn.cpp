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
	Executecpuid(1); // ִ��cpuid������Ϊ eax = 1
	bool isSupport = m_edx & (1 << 18); // edx�Ƿ�Ϊ1����CPU�Ƿ�������к�
	if (false == isSupport) // ��֧�֣�����false
	{
		printf("*** get fail!\n");
		//return false;
	}
	memcpy(&serial[4], &m_eax, 4); // eaxΪ���λ������WORD

	Executecpuid(3); // ִ��cpuid������Ϊ eax = 3
	memcpy(&serial[0], &m_ecx, 4); // ecx �� edxΪ��λ��4��WORD
	memcpy(&serial[2], &m_edx, 4); // ecx �� edxΪ��λ��4��WORD

	for (int i = 0; i < 6; i++)
	{
		printf("%04x", serial[i]);
	}
	return true;
}

void cl_cpusn::Executecpuid(DWORD veax)
{
	// ��ΪǶ��ʽ�Ļ����벻��ʶ�� ���Ա����
	// ���Զ����ĸ���ʱ������Ϊ����
	DWORD deax = 0;
	DWORD debx = 0;
	DWORD decx = 0;
	DWORD dedx = 0;

#ifndef ANDROID
	__asm
	{
		mov eax, veax; �������������eax
		cpuid; ִ��cpuid
		mov deax, eax; �������д���ѼĴ����еı���������ʱ����
		mov debx, ebx
		mov decx, ecx
		mov dedx, edx
	}
#endif

	m_eax = deax; // ����ʱ�����е����ݷ������Ա����
	m_ebx = debx;
	m_ecx = decx;
	m_edx = dedx;
}


