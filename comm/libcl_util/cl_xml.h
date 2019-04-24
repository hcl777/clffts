#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#ifdef _WIN32
#pragma warning(disable:4996)
//windows 版本，如果XML过大，递归调用过深，会导致大量堆栈开销。VC环境0默认1M,正常如果处理过大XML，建议“堆栈保留大小为10M”。
#endif

#define CLXML_SAFEDELETE_ARRAY(a) if(a) {delete[] a;a=NULL;}

class cl_xml;


class cl_xmlAtrri
{
	friend class cl_xmlNode;
private:
	typedef struct tagANode
	{
		char* name;
		char* val;
		tagANode *next;
		tagANode(void):name(NULL),val(NULL),next(NULL){}
	}ANode_t;

	cl_xmlAtrri(void)
	{
	}

	~cl_xmlAtrri(void)
	{
		clear();
	}
	void clear();
public:
	bool empty() const {return NULL==head.next;}
	int load_string(const char* szAttri);
	const char* get_attri(const char* name);
	int set_attri(const char* name,const char* val);
	bool check_attri(const char* name,const char* val);
	int size();
	int to_string(char* buf,int buflen);
private:
	ANode_t head;
};

class cl_xmlNode
{
	friend class cl_xml;
private:
	cl_xmlNode(void):szTag(NULL),szData(NULL),pparent(NULL),pchild(NULL),pnext(NULL) {}
	~cl_xmlNode(void){
		//以下因为ecos执行delete时 将以下代码内联到其它函数中，可能导致代码栈溢出。所以放在独立函数中
		//CLXML_SAFEDELETE_ARRAY(szTag)
		//CLXML_SAFEDELETE_ARRAY(szData)
		clear();
	}
public:
	void clear();
	const char* get_tag(){return szTag;}
	const char* get_data(){return szData;};
	int set_data(const char* data)
	{
		char *ptr = NULL;
		if(data)
		{
			ptr = new char[strlen(data)+1];
			if(!ptr)
				return -1;
			strcpy(ptr,data);
		}
		CLXML_SAFEDELETE_ARRAY(szData)
		szData = ptr;
		return 0;
	}

	cl_xmlNode* next(){return pnext;}
	cl_xmlNode* parent(){return pparent;}
	cl_xmlNode* child() {return pchild;}

public:
	cl_xmlAtrri attri;
private:
	char *szTag;
	char *szData;
	cl_xmlNode *pparent,*pchild,*pnext;
};

class cl_xml
{
public:
	cl_xml(void);
	~cl_xml(void);
	static bool string_english_only(const char* str);
	void clear();
	int load_file(const char* path);
	int load_string(const char* szXML);
	int save_file(const char* path);
	int size();
	int to_string(char* szXML,int maxSize);
	int set_xml_head(const char* head);
	cl_xmlNode *find_first_node(const char* tagPath,cl_xmlNode* start=NULL);
	//查找同路径中对应的属性值所在节点
	cl_xmlNode *find_first_node_attri(const char* tagPath,const char* attri_name,const char* attri_val,cl_xmlNode* start=NULL);
	//查找节点data拷到buf中.返回buf,不管有无节点,只要空值就使用默认
	char* find_first_node_data(cl_xmlNode* start,const char* tagPath,const char* val_default,char buf[],int bufsize);
	const char* get_node_data(const char* tagPath,cl_xmlNode* start=NULL);
	int delete_node(cl_xmlNode *node);
	cl_xmlNode *add_node(const char* path,const char* data=NULL);
	cl_xmlNode *add_child(cl_xmlNode *node,const char* tag,const char* data=NULL);
private:
	void delete_node_root(cl_xmlNode* head,bool delnext);
	int load_next(const char* szMin,const char* szMax,cl_xmlNode* head);
	int load_child(const char* szMin,const char* szMax,cl_xmlNode* head);
	const char* find_tag_end(const char* tag,const char* szMin,const char* szMax);
	int size_count(cl_xmlNode* head,int step);
	int format_string(char* szXML,cl_xmlNode* head,int step);
	cl_xmlNode *find_node(const char* tag,int tagLen,const char* childTag,cl_xmlNode *node);
private:
	cl_xmlNode m_head;
	char m_szXMLHead[256];
};
void cl_xml_test();
