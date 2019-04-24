#pragma once
#include "cl_basetypes.h"

/*
说明：
linux 使用wcstombs() 和 mbstowcs() 实现多字节与unicode的互转。
使用这两个函数与本地setlocale() 字符集有关。例如设置gb2312,则wcstombs表示转为GB2312.
*/

//cl_unicode
class cl_unicode
{
public:
	static void setlocal_gb2312();

	static void UTF_8ToUnicode(wchar_t* pOut,const char *pText);  // 把UTF-8转换成Unicode  

	static void UnicodeToUTF_8(char* pOut,wchar_t* pText);  //Unicode 转换成UTF-8  

	static void UnicodeToGB2312(char* pOut,wchar_t uData);  // 把Unicode 转换成 GB2312    

	static void Gb2312ToUnicode(wchar_t* pOut,const char *gbBuffer);// GB2312 转换成　Unicode  

	static void GB2312ToUTF_8(string& pOut,const char *pText, int pLen);//GB2312 转为 UTF-8  

	static void UTF_8ToGB2312(string& pOut,const char *pText, int pLen);//UTF-8 转为 GB2312 
};
