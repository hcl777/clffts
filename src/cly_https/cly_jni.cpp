#include "cly_jni.h"

#include "cljni_util.h"
#include "cly_https.h"
#include "cl_net.h"



#ifdef ANDROID

#ifdef __cplusplus
extern "C" {
#endif
//******************************************************************************************************

JavaVM* ju_vm = NULL;
jclass ju_cls = NULL;
#define JCLASS_PATH "android/cly/Https"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	DEBUGMSG("# jni_onload():\n");
	ju_vm = vm;

	//检查自定义的类必须在java主线程才可以成功
	JNIEnv*  env;
	jclass tmp;
	ju_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	tmp = env->FindClass(JCLASS_PATH);
	ju_cls = (jclass)env->NewGlobalRef(tmp);

	////env->DeleteLocalRef(ju_cls); //正常程序退出时执行这个释放
	return JNI_VERSION_1_6;
}


//=======================================================================================================
//java call c api
//如果函数名带下划线的话，C函数的下划线后面要加多"1",
//如java的包为android.cly, 类名为Down，函数名为int init(String path),
//则C的函数名为JNIEXPORT jint JNICALL Java_android_cly_Down_init(JNIEnv *env, jobject thiz,jstring path)

cly_https http;
JNIEXPORT jint JNICALL Java_android_cly_Https_init(JNIEnv *env, jobject thiz, jint port,jint threadnum)
{
	cl_net::socket_init();
	return http.open((unsigned short)port, threadnum);
}

JNIEXPORT jint JNICALL Java_android_cly_Https_fini(JNIEnv *env, jobject thiz)
{
	http.close();

	cl_net::socket_fini();
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif

void jni_fire_rdbread_begin()
{
#ifdef ANDROID
	string func_name = "ccj_rdbread_begin";
	if (NULL == ju_cls)
		return;
	ju_call_java_I_V(ju_vm, ju_cls, func_name.c_str());
#endif
}

