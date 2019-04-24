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
	int income = 0;  //预期收益
	int bet_num = 0; //每期下注数目
	int pre_investment = 0; //本轮已经投入资本
	int bet; //每注资本
	int i;
	int n; //预计多少期内可中特
	while(1)
	{
		printf("\n[特码方案]:赔率40，连买多期直到中特然后抽利，然后又重第1期开始.\n");
		income = 0;
		bet_num = 0;
		pre_investment = 0;
		while(0==income)
		{
			printf("请输入预期收益(元）：");
			scanf("%d",&income);
		}
		while(bet_num<1 ||bet_num>39)
		{
			printf("请输入每期下注个数：");
			scanf("%d",&bet_num);
		}
		n = (int)(49/double(bet_num) * 2);
		printf("【结果】：预计%d期（%.1f月)内可中特，以下是连输的投入情况：\n",n,(float)(n/(double)12));
		printf("%26s %15s %15s %15s\n","每注","本期投","累计已投","中特收益");
		for(i=0;i<n+10;++i)
		{
			bet = (income + pre_investment)/(40-bet_num);
			pre_investment += bet_num*bet;
			printf("|[第%3d期] =》 %10d元 || %10d元 || %10d元 || %10d元 |\n",i+1,bet,bet*bet_num,pre_investment,bet*40-pre_investment);
		}
	}
}

void cl_print_lottery2()
{
	char buf[1024];
	while(1)
	{
		printf("\n 49个数博彩几率实验：\n");
		printf("请输入一组数，“，”分隔：");
		memset(buf,0,1024);
		gets_s(buf,1024);
		
	}
}
#endif

