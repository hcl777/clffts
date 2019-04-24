#pragma once

namespace cl
{
//cmd格式: exe p1 p2 ...; 其中exe 为第0个参数.

//获取命令行参数个数
int cle_cmd_argc(const char* cmd);

//获取命令行第i个参数的起始位置和长度,beg指向cmd内存,不别分配内存
int cle_cmd_argv_i(const char* cmd,int i,const char** pbeg,int* length);


//此接口会阻塞直到cmd执行完成
int cle_system(const char* cmd);

int cle_shell(const char* cmd);

};

