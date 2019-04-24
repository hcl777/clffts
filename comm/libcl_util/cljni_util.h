#pragma once

#include "cl_basetypes.h"

#ifdef ANDROID

//#ifdef __cplusplus
//extern "C" {
//#endif
//
//
string ju_jstringTostring(JNIEnv* env,jstring jstr) ;
int ju_call_java_I_V(JavaVM* ju_vm,jclass ju_cls,const char* func_name);
int ju_call_java_I_I(JavaVM* ju_vm,jclass ju_cls,const char* func_name,int i);
int ju_call_java_I_SI(JavaVM* ju_vm,jclass ju_cls,const char* func_name,const char* str,int i);
string ju_call_java_S_V(JavaVM* ju_vm,jclass ju_cls,const char* func_name);
int ju_call_java_I_SSI(JavaVM* ju_vm,jclass ju_cls,const char* func_name,const char* str1,const char* str2,int i);

//#ifdef __cplusplus
//}
//#endif

#endif //ANDROID

