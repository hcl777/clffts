#pragma once
#include "cl_basetypes.h"
#include "cl_ntypes.h"
#include "cly_fileinfo.h"

typedef struct tagDownAutoInfo
{
	string hash;
	string name;  //��Ϊ�����������ʱ������·�����ڼ�������ʱ��ѡ��·����������name���ݴ�·��
	string path;
	uint64 size;
	int ftype;
	int priority;
	int state; //0:stop,1:downing,2:queue,
	string createtime;// = time(NULL)
	int faileds; //����ʧ�ܵĴ���
	int progress;//��ɵ�ǧ�ֱ�
	int speed;//�ٶ� B/S
	int downtick; //�����ۼ�ʱ��
	int zerospeedtick; //0�ٶ�ʱ���ۼ�

	int streak_faileds; //��������ʧ�ܴ���,��һ�γɹ�����0
	unsigned int last_changed_time; //����������ʱ��

	int save_original; //��0����ԭ�ļ�
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
	//�����������ȼ���ֵ,ʱ��t���ڵݼ�streak_faileds;
	//Ŀ���õȼ�+ǧ�ֱ�+����ʧ�ܴ���+ʱ��ο� �ۺϼƷ�.
	int score(unsigned int t)
	{
		if(t>0 && streak_faileds>0)
		{
			int  i = timer_distance(t,last_changed_time)/1800; //ÿ��Сʱ�ݼ�һ��
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

