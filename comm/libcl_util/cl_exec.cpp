#include "cl_exec.h"

#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN		// 从 Windows 头中排除极少使用的资料
	#include <windows.h>
#include <shellapi.h>
#else
#endif


namespace cl
{

int cle_cmd_argc(const char* cmd)
{
	const char* beg = cmd;
	const char* end;
	int n = 0;

	while(1)
	{
		while(*beg==' ')
			beg++;
		if(*beg=='\0') 
			break;
		end = beg+1;
		if(*beg=='\'')
		{
			while(*end!='\'' && *end!='\0')
				end++;
		}
		else if(*beg=='\"')
		{
			while(*end!='\"' && *end!='\0')
				end++;
		}
		else
		{
			while(*end!=' ' && *end!='\0')
				end++;
		}
		if(*end=='\0')
			break;
		beg = end+1;
		n++;
	}
	return n;
}
int cle_cmd_argv_i(const char* cmd,int i,const char** pbeg,int* length)
{
	const char* beg = cmd;
	const char* end;
	int n = 0;

	while(n<i)
	{
		while(*beg==' ')
			beg++;
		if(*beg=='\0') 
			break;
		end = beg+1;
		if(*beg=='\'')
		{
			while(*end!='\'' && *end!='\0')
				end++;
		}
		else if(*beg=='\"')
		{
			while(*end!='\"' && *end!='\0')
				end++;
		}
		else
		{
			while(*end!=' ' && *end!='\0')
				end++;
		}
		if(*end=='\0')
			break;
		beg = end+1;
		n++;
	}
	//再忽略前面的空格
	while(*beg==' ')
			beg++;
	if(n<i || *beg=='\0')
		return -1;

	end = beg+1;
	if(*beg=='\'')
	{
		while(*end!='\'' && *end!='\0')
			end++;
	}
	else if(*beg=='\"')
	{
		while(*end!='\"' && *end!='\0')
			end++;
	}
	else
	{
		while(*end!=' ' && *end!='\0')
			end++;
	}
	*pbeg = beg;
	if(*beg=='\''||*beg=='\"')
		*pbeg = beg+1;
	*length = (int)(end-*pbeg);
	return 0;
}

/////////////////////////////////////////////////
int cle_system(const char* cmd)
{
	return system(cmd);
}

int cle_shell(const char* cmd)
{
	char file[512];
	const char *param=NULL;
	char workdir[512]={0,};
	const char* p=NULL;
	int len = 0;
	if(0!=cle_cmd_argv_i(cmd,0,&p,&len))
		return -1;
	memcpy(file,p,len);
	file[len] = '\0';
	if(NULL==(p=strrchr(file,'/')))
		p = strrchr(file,'\\');
	if(NULL!=p)
	{
		memcpy(workdir,file,(int)(p-file));
		workdir[(int)(p-file)] = '\0';
	}
	cle_cmd_argv_i(cmd,1,&param,&len);

#ifdef _WIN32
	ShellExecuteA(NULL, "open", file, param, workdir, SW_SHOW);
#else
#endif
	return 0;
}

}

