#pragma once

#if !defined(_OS) && !defined(ANDROID)
const char* cl_hz2py_singleword(const char* sz);

int cl_hz2py_string(const char* str,char* outbuf,int outbuflen);
#endif

