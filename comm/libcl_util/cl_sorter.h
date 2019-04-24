#pragma once


/*
功能: 
	将数组arr[]进行排序,ascending=true 表示升序.false 表降序.
	采用"冒泡(bubble)排序算法".
	即执行一次循环,将最大值移到后面.记录最后交换过的位置.下次再循环到上次交换位置.
*/
template<typename T>
void cl_sort_bubble(T arr[],int n,bool ascending=true)
{
	int i=0,j,k=n-1;
	T tmp;
	for(j=k;j>0;j=k)
	{
		k = 0;
		for(i=0;i<j;++i)
		{
			if((ascending && arr[i]>arr[i+1]) ||((!ascending) && arr[i]<arr[i+1]))
			{
				tmp = arr[i];
				arr[i] = arr[i+1];
				arr[i+1] = tmp;
				k = i;
			}
		}
	}
}
