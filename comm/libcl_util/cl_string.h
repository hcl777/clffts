#pragma once

#include <string.h>


class cl_string
{
public:
	cl_string(const char* sz=0);
	cl_string(const cl_string& str);
	~cl_string(void);
	static size_t npos;
public:
	cl_string& operator=(const char* sz);
	cl_string& operator=(const cl_string& str);
	cl_string operator+(const char* sz) const;
	cl_string operator+(const cl_string& str) const;
	cl_string& operator+=(const char* sz);
	cl_string& operator+=(const cl_string& str);
	bool replace(size_t pos,size_t len,const char* buf,size_t buflen=(size_t)-1); //len 为要替换原来串的长度
	bool operator==(const char* sz) const;
	bool operator==(const cl_string& str) const;
	bool operator!=(const char* sz) const;
	bool operator!=(const cl_string& str) const; 
	bool operator<(const char* sz) const;
	bool operator>(const char* sz) const;
	bool operator<(const cl_string& str) const;
	bool operator>(const cl_string& str) const;
	size_t find(const char* sz,size_t pos=0) const;
	size_t find(const cl_string& str,size_t pos=0) const;
	size_t rfind(const char s) const;
	cl_string substr(size_t pos,size_t len=(size_t)-1) const;
	cl_string& erase(size_t pos,size_t len=(size_t)-1);
	size_t length() const;
	bool empty() const;
	const char* c_str() const {if(!m_data) return &m_tmp; return m_data;}
	const char& at(size_t i) const;
	const char& operator[](size_t i) const;
private:
	bool mem_ok(size_t size,bool useold=false);
private:
	char *m_data;
	size_t m_size;
	char m_tmp;
};

typedef cl_string string;

