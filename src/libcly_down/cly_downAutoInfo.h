#pragma once
#include "cl_basetypes.h"
#include "cl_ntypes.h"
#include "cly_fileinfo.h"

typedef struct tagDownAutoInfo
{
	string hash;
	string name;  //因为不是添加任务时立即配路径，在激活任务时再选择路径，所以用name来暂存路径
	string path;
	uint64 size;
	int ftype;
	int priority;
	int state; //0:stop,1:downing,2:queue,
	string createtime;// = time(NULL)
	int faileds; //下载失败的次数
	int progress;//完成的千分比
	int speed;//速度 B/S
	int downtick; //下载累计时间
	int zerospeedtick; //0速度时间累计

	int streak_faileds; //连续下载失败次数,有一次成功则清0
	unsigned int last_changed_time; //最后任务调整时间

	int save_original; //非0保存原文件
	tagDownAutoInfo()
		:size(0)
		,ftype(FTYPE_DOWNLOAD)
		,priority(0)
		,state(2)
		,createtime("")
		,faileds(0)
		,progress(0)
		,speed(0)
		,downtick(0)
		,zerospeedtick(0)
		,streak_faileds(0)
		,last_changed_time(0)
		, save_original(0)
	{}

	int timer_distance(unsigned int t,unsigned int base)
	{
		return (int)t - (int)base;
	}
	//计算下载优先级分值,时间t用于递减streak_faileds;
	//目采用等级+千分比+连续失败次数+时间参考 综合计分.
	int score(unsigned int t)
	{
		if(t>0 && streak_faileds>0)
		{
			int  i = timer_distance(t,last_changed_time)/1800; //每半小时递减一次
			if(i>0)
			{
				streak_faileds -= i;
				last_changed_time = t;
				if(streak_faileds<0) streak_faileds = 0;
			}
		}
		if(0==priority) priority = 1;
		return 2000/priority + progress - streak_faileds*100;
	}
}cly_downAutoInfo;

typedef list<cly_downAutoInfo*>  DownAutoInfoList;
typedef DownAutoInfoList::iterator DownAutoInfoIter;

