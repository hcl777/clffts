#include "cl_lottery.h"


#ifdef _WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

	// 4482 4267 4018 4800 4311 4312 4102
	#pragma warning(disable:4996)


cl_lottery::cl_lottery(void)
{
}


cl_lottery::~cl_lottery(void)
{
}

void cl_print_lottery()
{
	int income = 0;  //Ԥ������
	int bet_num = 0; //ÿ����ע��Ŀ
	int pre_investment = 0; //�����Ѿ�Ͷ���ʱ�
	int bet; //ÿע�ʱ�
	int i;
	int n; //Ԥ�ƶ������ڿ�����
	while(1)
	{
		printf("\n[���뷽��]:����40���������ֱ������Ȼ�������Ȼ�����ص�1�ڿ�ʼ.\n");
		income = 0;
		bet_num = 0;
		pre_investment = 0;
		while(0==income)
		{
			printf("������Ԥ������(Ԫ����");
			scanf("%d",&income);
		}
		while(bet_num<1 ||bet_num>39)
		{
			printf("������ÿ����ע������");
			scanf("%d",&bet_num);
		}
		n = (int)(49/double(bet_num) * 2);
		printf("���������Ԥ��%d�ڣ�%.1f��)�ڿ����أ������������Ͷ�������\n",n,(float)(n/(double)12));
		printf("%26s %15s %15s %15s\n","ÿע","����Ͷ","�ۼ���Ͷ","��������");
		for(i=0;i<n+10;++i)
		{
			bet = (income + pre_investment)/(40-bet_num);
			pre_investment += bet_num*bet;
			printf("|[��%3d��] =�� %10dԪ || %10dԪ || %10dԪ || %10dԪ |\n",i+1,bet,bet*bet_num,pre_investment,bet*40-pre_investment);
		}
	}
}

void cl_print_lottery2()
{
	char buf[1024];
	while(1)
	{
		printf("\n 49�������ʼ���ʵ�飺\n");
		printf("������һ�������������ָ���");
		memset(buf,0,1024);
		gets_s(buf,1024);
		
	}
}
#endif

