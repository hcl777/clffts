#include "cljni_serialfile.h"


#ifdef ANDROID

#include <jni.h>
#include <android/log.h>
#include "cl_serialfile.h"
#include "cljni_util.h"

#ifdef __cplusplus
extern "C" {
#endif

//jbyte *jb = (jbyte *)rb->bmp;
//	env->GetByteArrayRegion(jba,0,jlen,jb);

//return fd, -1±íÊ¾´íÎó
JNIEXPORT jint JNICALL Java_android_cljni_Serialfile_open(JNIEnv *env,jclass cls
	,jstring jpath,jint speed=9600,jint bsize=8,jint stopb=0,jint parity=0)
{
	string path = ju_jstringTostring(env,jpath);
	int fd = cl_serialfile_open(path.c_str(),speed,bsize,stopb,parity);
	DEBUGMSG("# %d=Serialfile_open(%s,%d,%d,%d,%d)\n",fd,path.c_str(),(int)speed,(int)bsize,(int)stopb,(int)parity);
	return fd;
}
JNIEXPORT jint JNICALL Java_android_cljni_Serialfile_read(JNIEnv *env,jclass cls
	,jint fd,jbyteArray jba,jint len,jint msec)
{
	int n = 0;
	char* buf = new char[len];
	n = cl_serialfile_read(fd,buf,len,msec);
	if(n>0)
	{
		env->SetByteArrayRegion(jba,0,n,(jbyte*)buf);
	}
	delete[] buf;
	DEBUGMSG("# %d=Serialfile_read(%d,%d,%d) \n",n,fd,len,msec);
	return n;
}
JNIEXPORT jint JNICALL Java_android_cljni_Serialfile_write(JNIEnv *env,jclass cls
	,jint fd,jbyteArray jba,jint len)
{
	int n = 0;
	char* buf = new char[len];
	env->GetByteArrayRegion(jba,0,n,(jbyte*)buf);
	n = cl_serialfile_write(fd,buf,len);
	delete[] buf;
	DEBUGMSG("# %d=Serialfile_write(%d,%d) \n",n,fd,len);
	return n;
}

JNIEXPORT jint JNICALL Java_android_cljni_Serialfile_close(JNIEnv *env,jclass cls
	,jint fd)
{
	int n = cl_serialfile_close(fd);
	DEBUGMSG("# %d=Serialfile_close(%d) \n",n,fd);
	return n;
}

#ifdef __cplusplus
}
#endif

#endif
