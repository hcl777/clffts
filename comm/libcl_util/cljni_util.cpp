#include "cljni_util.h"


#ifdef ANDROID

//#ifdef __cplusplus
//extern "C" {
//#endif


//JavaVM* ju_vm=NULL;
//jclass ju_cls=NULL;

//由于不同程序有不同的jni 类路径,所以不要在util中重写JNI_OnLoad,以下只是参考.
//#define JCLASS_PATH "android/p2p/Lspi"
////************************************************************
//JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
//{
//	ju_vm = vm;
//
//	//检查自定义的类必须在java主线程才可以成功
//	JNIEnv*  env;
//	jclass tmp;
//	ju_vm->GetEnv((void**)&env,JNI_VERSION_1_6);
//	tmp = env->FindClass(JCLASS_PATH);
//	ju_cls = (jclass)env->NewGlobalRef(tmp);
//	////env->DeleteLocalRef(ju_cls); //正常程序退出时执行这个释放
//	return JNI_VERSION_1_6;
//}
////*************************************************************


string ju_jstringTostring(JNIEnv* env,jstring jstr)  
{  
	char*  rtn = NULL;  
	jclass clsstring = env->FindClass("java/lang/String");   
	jstring strencode = env->NewStringUTF("GB2312");  
	jmethodID mid = env->GetMethodID(clsstring,"getBytes","(Ljava/lang/String;)[B");   
	jbyteArray barr = (jbyteArray)env->CallObjectMethod(jstr,mid,strencode);  
	jsize alen = env->GetArrayLength(barr);  
	jbyte* ba = env->GetByteArrayElements(barr,JNI_FALSE);  
	if(alen > 0)  
	{  
		rtn = (char*)malloc(alen+1);  //new   char[alen+1];  
		memcpy(rtn,ba,alen);  
		rtn[alen]=0;  
	}  
	env->ReleaseByteArrayElements(barr,ba,0);  
	string stemp(rtn);
	free(rtn);
	env->DeleteLocalRef(clsstring);
	env->DeleteLocalRef(barr);
	return stemp;  
} 
//*************************************************************************
int ju_call_java_I_V(JavaVM* ju_vm,jclass ju_cls,const char* func_name)
{
	jint ret = -1;
	JNIEnv*  env = NULL;
	jmethodID mid = 0;
	ju_vm->AttachCurrentThread(&env, NULL);

	mid = env->GetStaticMethodID(ju_cls,func_name,"()I");
	if(NULL==mid)
	{
		DEBUGMSG("GetStaticMethodID(%s) faild! \n", func_name);
		return -1;
	}
	ret = env->CallStaticIntMethod(ju_cls,mid);
	char buflog[1024];
	sprintf(buflog,"%d=%s()",ret,func_name);
	//cl_util::write_log(buflog,g_logpath.c_str());

	ju_vm->DetachCurrentThread();
	return ret;
}
int ju_call_java_I_I(JavaVM* ju_vm,jclass ju_cls,const char* func_name,int i)
{
	jint ret = -1;
	JNIEnv*  env = NULL;
	jmethodID mid = 0;
	ju_vm->AttachCurrentThread(&env, NULL);

	mid = env->GetStaticMethodID(ju_cls,func_name,"(I)I");
	if(NULL==mid)
	{
		DEBUGMSG("GetStaticMethodID(%s) faild! \n", func_name);
		return -1;
	}
	ret = env->CallStaticIntMethod(ju_cls,mid,i);
	char buflog[1024];
	sprintf(buflog,"%d=%s(%d)",ret,func_name,i);
	//cl_util::write_log(buflog,g_logpath.c_str());

	ju_vm->DetachCurrentThread();
	return ret;
}
int ju_call_java_I_SI(JavaVM* ju_vm,jclass ju_cls,const char* func_name,const char* str,int i)
{
	jint ret = -1;
	JNIEnv*  env = NULL;
	jmethodID mid = 0;
	ju_vm->AttachCurrentThread(&env, NULL);

	jstring jstr = env->NewStringUTF(str);
	mid = env->GetStaticMethodID(ju_cls,func_name,"(Ljava/lang/String;I)I");
	if(NULL==mid)
	{
		DEBUGMSG("GetStaticMethodID(%s) faild! \n", func_name);
		return -1;
	}
	ret = env->CallStaticIntMethod(ju_cls,mid,jstr,i);
	DEBUGMSG("%d=%s(%s,%d) \n",ret,func_name,str,i);

	ju_vm->DetachCurrentThread();
	return ret;
}
string ju_call_java_S_V(JavaVM* ju_vm,jclass ju_cls,const char* func_name)
{
	string ret;
	JNIEnv*  env = NULL;
	jmethodID mid = 0;
	ju_vm->AttachCurrentThread(&env, NULL);

	mid = env->GetStaticMethodID(ju_cls,func_name,"()Ljava/lang/String;");
	if(NULL==mid)
	{
		DEBUGMSG("GetStaticMethodID(%s) faild! \n", func_name);
		return "";
	}
	jstring jstr = (jstring)env->CallStaticObjectMethod(ju_cls,mid);
	ret = ju_jstringTostring(env,jstr);
	//char buflog[1024];
	//sprintf(buflog,"%s=%s()",ret.c_str(),func_name);
	//cl_util::write_log(buflog,g_logpath.c_str());

	ju_vm->DetachCurrentThread();
	return ret;
}
int ju_call_java_I_SSI(JavaVM* ju_vm,jclass ju_cls,const char* func_name,const char* str1,const char* str2,int i)
{
	jint ret = -1;
	JNIEnv*  env = NULL;
	jmethodID mid = 0;
	ju_vm->AttachCurrentThread(&env, NULL);

	jstring jstr1 = env->NewStringUTF(str1);
	jstring jstr2 = env->NewStringUTF(str2);
	mid = env->GetStaticMethodID(ju_cls,func_name,"(Ljava/lang/String;Ljava/lang/String;I)I");
	if(NULL==mid)
	{
		DEBUGMSG("GetStaticMethodID(%s) faild! \n", func_name);
		return -1;
	}
	ret = env->CallStaticIntMethod(ju_cls,mid,jstr1,jstr2,i);
	DEBUGMSG("%d=%s(%s,%s,%d) \n",ret,func_name,str1,str2,i);

	ju_vm->DetachCurrentThread();
	return ret;
}

//#ifdef __cplusplus
//}
//#endif

#endif //ANDROID

