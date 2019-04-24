#pragma once

//
#include "cl_basetime.h"
#include <stdint.h>

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(expr) typedef char __static_assert_t[ (expr) ]
#endif

#define CL_MAX(x,y) ((x)>(y)?(x):(y))
#define CL_MIN(x,y) ((x)<(y)?(x):(y))

typedef unsigned long long  ULONGLONG;
typedef unsigned long DWORD;

inline void void_printf(const char* _Format, ...){}
#ifdef ANDROID
	#include <jni.h>
	#include <android/log.h>

	#define DEBUGMSG void_printf
	#define CLLOG1 void_printf
	//#define DEBUGMSG(...)  __android_log_print(ANDROID_LOG_DEBUG,"#JNI:",__VA_ARGS__) // 定义LOGD类型
	//#define CLLOG1(...)  __android_log_print(ANDROID_LOG_DEBUG,"#JNI:",__VA_ARGS__) // 定义LOGD类型
#else
	//#define DEBUGMSG void_printf
	#define DEBUGMSG printf
	#define D printf("*--FILE[%s] FUNC[%s] LINE[%d]--*\n",__FILE__,__FUNCTION__,__LINE__);
	#define CLLOG1 printf
#endif

#ifdef _WIN32
	// 4482 4267 4018 4800 4311 4312 4102
	#pragma warning(disable:4996)
	
	char *strcasestr(const char *haystack, const char *needle);

#elif defined(__GNUC__) && defined(__linux__)
	#include <ctype.h>

	#ifndef MAX_PATH
		#define MAX_PATH 512
	#endif

	#define TRUE 1
	#define FALSE 0

	typedef int                 BOOL;
	typedef long                LONG;

	//放入incnet中定义
	//typedef int SOCKET;
	//#define INVALID_SOCKET -1
	//#define SOCKET_ERROR -1
	//#define closesocket(s) close(s)

	#define stricmp strcasecmp
	char* strlwr(char* str);

#endif

//template<typename T, bool flag> struct ReferenceSelector {
//	typedef T ResultType;
//};
//template<typename T> struct ReferenceSelector<T,true> {
//	typedef const T& ResultType;
//};
//
//template<typename T> class IsOfClassType {
//public:
//	template<typename U> static char check(int U::*);
//	template<typename U> static float check(...);
//public:
//	enum { Result = sizeof(check<T>(0)) };
//};
//
//template<typename T> struct TypeTraits {
//	typedef IsOfClassType<T> ClassType;
//	typedef ReferenceSelector<T, ((ClassType::Result == 1) || (sizeof(T) > sizeof(char*)) ) > Selector;
//	typedef typename Selector::ResultType ParameterType;
//};
//
//注意：linux 注释下面语句，有后面的"\"会提示"multi-line comment,所以注意时修改
//#define GETSET(type, name, name2) \//
//protected: type name; \//
//public: TypeTraits<type>::ParameterType Get##name2() const { return name; } \//
//	void Set##name2(TypeTraits<type>::ParameterType a##name2) { name = a##name2; }


#define GETSET(type, name, name2) \
protected: type name; \
public: type& get##name2() { return name; } \
	void set##name2(const type& a##name2) { name = a##name2; }

#define CL_GET(type, name, name2) \
protected: type name; \
public: type& get##name2() { return name; }

#include "cl_string.h"
#include "cl_cyclist.h"
#include "cl_rbtmap.h"



//window 隐藏控制台窗口方法。
// #pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" ) 
