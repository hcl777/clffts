
#include "cly_jni.h"
#include "cljni_util.h"

#ifdef ANDROID
#include "cly_d.h"
#include "cl_RDBFile64.h"

#ifdef __cplusplus
extern "C" {
#endif


//******************************************************************************************************

JavaVM* ju_vm=NULL;
jclass ju_cls=NULL;
#define JCLASS_PATH "android/cly/Down"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	DEBUGMSG("# jni_onload():\n");
	ju_vm = vm;

	//检查自定义的类必须在java主线程才可以成功
	JNIEnv*  env;
	jclass tmp;
	ju_vm->GetEnv((void**)&env,JNI_VERSION_1_6);
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
bool g_bcly_init = false;
cly_config_t g_conf;

JNIEXPORT jint JNICALL Java_android_cly_Down_init(JNIEnv *env, jobject thiz,jstring confpath, jstring jpeer_name)
{
	if(g_bcly_init)
		return 1;
	cl_net::socket_init();
	
	string path = ju_jstringTostring(env,confpath);
	string name = ju_jstringTostring(env,jpeer_name);
	if(0!=cly_config_load(g_conf,path.c_str()))
	{
		CLLOG1("*** load xml config faild! \n");
		return -1;
	}
	if (name.length() > 10 && name.length() < 32)
		g_conf.peer_name = name;

	////test dump
	//char *p = NULL;
	//p[25] = 12;
	//delete[] p;
	//p = new char[125];
	//p[1024] = 56;
	//delete[] p;
	//memset(p, 0, 2048);

	if(0!=clyd::instance()->init(&g_conf))
		return -1;
	g_bcly_init = true;
	return 0;
}

JNIEXPORT jint JNICALL Java_android_cly_Down_fini(JNIEnv *env, jobject thiz)
{
	if(!g_bcly_init)
		return 0;

	clyd::instance()->fini();
	clyd::destroy();

	cl_net::socket_fini();
	g_bcly_init = false;
	return 0;
}
JNIEXPORT jint JNICALL Java_android_cly_Down_rdbcheck_1size_1ok(JNIEnv *env, jobject thiz, jstring jpath)
{
	string path = ju_jstringTostring(env, jpath);
	return cl_ERDBFile64::check_size_ok(path.c_str());
}

#ifdef __cplusplus
}
#endif

#endif

//=======================================================================================================
//c call java api
void jni_fire_download_wrong(const string& hash, int i)
{
#ifdef ANDROID
	string func_name = "ccj_download_wrong";
	if (NULL == ju_cls)
		return;
	int ret = ju_call_java_I_SI(ju_vm, ju_cls, func_name.c_str(), hash.c_str(), i);
	CLLOG1(" %d=%s(%s,%d) \n", ret, func_name.c_str(), hash.c_str(), i);
#endif
}
void jni_fire_download_fini(const string& hash,const string& path,int result)
{
#ifdef ANDROID
	string func_name = "ccj_download_fini";
	if(NULL==ju_cls)
		return;
	int ret = ju_call_java_I_SSI(ju_vm,ju_cls,func_name.c_str(),hash.c_str(),path.c_str(),result);
	CLLOG1(" %d=%s(%s,%s,%d) \n",ret,func_name.c_str(),hash.c_str(),path.c_str(),result);
#endif
}
void jni_fire_rdbread_begin()
{
#ifdef ANDROID
	string func_name = "ccj_rdbread_begin";
	if (NULL == ju_cls)
		return;
	ju_call_java_I_V(ju_vm, ju_cls, func_name.c_str());
#endif
}