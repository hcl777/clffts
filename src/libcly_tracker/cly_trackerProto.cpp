#include "cly_trackerProto.h"
#include "cl_unicode.h"
#include "cl_util.h"
#include <string.h>

int g_cly_lang_utf8 = 0;
#define _XML_FINE_LIST_LOOP_CHILD_PRE(tagpath,attri_name,attri_val) \
	cl_xmlNode *n; \
	n = xml.find_first_node_attri(tagpath,attri_name,attri_val); \
	if(n) \
	{ \
		n = n->child(); \
		while(n) \
		{ 

#define _XML_FINE_LIST_LOOP_CHILD_END \
			n=n->next(); \
		} \
		return 0; \
	} \
	return -1;


#define _XML_FINE_SEG_LOOP_CHILD_PRE(tagpath,attri_name,attri_val) \
	cl_xmlNode *n; \
	const char *a; \
	int	  ret=-1; \
	n = xml.find_first_node_attri(tagpath,attri_name,attri_val); \
	if(n) \
	{ \
		n = n->child(); \
		while(n) \
		{  \
			a = n->attri.get_attri("n");\
			if(a)\
			{

#define _XML_FINE_SEG_LOOP_CHILD_END \
			} \
			n=n->next(); \
		} \
		ret = 0; \
	} 

#define _XML_SEG_ATTRI_IF_INT(s) if(0==strcmp(a,#s)) seg.s = cl_util::atoi(n->get_data());
#define _XML_SEG_ATTRI_ELSE_IF_INT(s) else if(0==strcmp(a,#s)) seg.s = cl_util::atoi(n->get_data());
#define _XML_SEG_ATTRI_ELSE_IF_INT64(s) else if(0==strcmp(a,#s)) seg.s = cl_util::atoll(n->get_data());
#define _XML_SEG_ATTRI_IF_STR(s) if(0==strcmp(a,#s)) {seg.s = n->get_data(); if(0==g_cly_lang_utf8) cl_unicode::UTF_8ToGB2312(seg.s,seg.s.c_str(),seg.s.length());}
#define _XML_SEG_ATTRI_ELSE_IF_STR(s) else if(0==strcmp(a,#s)) {seg.s = n->get_data();if(0==g_cly_lang_utf8) cl_unicode::UTF_8ToGB2312(seg.s,seg.s.c_str(),seg.s.length());}


//*************************************************************************************
int cly_xml_get_seg_error(cl_xml& xml,cly_seg_error_t& seg) 
{
	seg.code = -1;
	_XML_FINE_SEG_LOOP_CHILD_PRE("root/seg","id","error")

	_XML_SEG_ATTRI_IF_INT(code)
	_XML_SEG_ATTRI_ELSE_IF_STR(message)

	_XML_FINE_SEG_LOOP_CHILD_END

	if(0!=seg.code)
		DEBUGMSG("#*** SEG_ERROR(%d,%s) !\n",seg.code,seg.message.c_str());
	
	return ret;
}
int cly_xml_get_seg_config(cl_xml& xml,cly_seg_config_t& seg)
{
	memset((void*)&seg,0,sizeof(seg));
	_XML_FINE_SEG_LOOP_CHILD_PRE("root/seg","id","config")
		
	_XML_SEG_ATTRI_IF_INT(peer_id)
	_XML_SEG_ATTRI_ELSE_IF_INT(group_id)
	_XML_SEG_ATTRI_ELSE_IF_INT(peer_type)
	_XML_SEG_ATTRI_ELSE_IF_INT(used)
	_XML_SEG_ATTRI_ELSE_IF_INT(limit_share_speediKB)
	_XML_SEG_ATTRI_ELSE_IF_INT(limit_share_speedKB)
	_XML_SEG_ATTRI_ELSE_IF_INT(limit_down_speedKB)

	_XML_SEG_ATTRI_ELSE_IF_INT(timer_keepaliveS)
	_XML_SEG_ATTRI_ELSE_IF_INT(timer_getddlistS)
	_XML_SEG_ATTRI_ELSE_IF_INT(timer_reportprogressS)

	_XML_FINE_SEG_LOOP_CHILD_END
	return ret;
}
int cly_xml_get_seg_keepalive_ack(cl_xml& xml, cly_seg_keepalive_ack_t& seg)
{
	_XML_FINE_SEG_LOOP_CHILD_PRE("root/seg", "id", "keepalive_ack")
	_XML_SEG_ATTRI_IF_INT(used)
	_XML_FINE_SEG_LOOP_CHILD_END
	return ret;
}
int cly_xml_get_seg_clyt_fileinfo(cl_xml& xml,clyt_fileinfo_t& seg)
{
	memset((void*)&seg,0,sizeof(seg));
	_XML_FINE_SEG_LOOP_CHILD_PRE("root/seg","id","fileinfo")
		
	_XML_SEG_ATTRI_IF_INT(file_id)
	_XML_SEG_ATTRI_ELSE_IF_STR(hash)
	_XML_SEG_ATTRI_ELSE_IF_STR(name)
	_XML_SEG_ATTRI_ELSE_IF_STR(subhash)
	_XML_SEG_ATTRI_ELSE_IF_INT(peer_num)

	_XML_FINE_SEG_LOOP_CHILD_END
	return ret;
}

int cly_xml_get_list(cl_xml& xml,list<string>& ls,const char* idval)
{
	_XML_FINE_LIST_LOOP_CHILD_PRE("root/list","id",idval)
	ls.push_back(n->get_data());
	_XML_FINE_LIST_LOOP_CHILD_END
}
int cly_xml_get_seg_fchsum(cl_xml& xml,cly_seg_fchsum_t& seg)
{
	memset((void*)&seg,0,sizeof(seg));
	_XML_FINE_SEG_LOOP_CHILD_PRE("root/seg","id","fini_chsum")

	_XML_SEG_ATTRI_IF_INT(ff_num)
	_XML_SEG_ATTRI_ELSE_IF_INT64(ff_checksum)

	_XML_FINE_SEG_LOOP_CHILD_END
	return ret;
}
int cly_xml_get_seg_ddlist(cl_xml& xml,list<string>& ls)
{
	//前两行为taskid和strformat
	cl_xmlNode *n; 
	string str;
	n = xml.find_first_node_attri("root/list","id","ddlist"); 
	if(n) 
	{
		ls.push_back(n->attri.get_attri("taskid"));
		str = n->attri.get_attri("fields");
		if(0==g_cly_lang_utf8) 
			cl_unicode::UTF_8ToGB2312(str,str.c_str(),str.length());
		ls.push_back(str);
		n = n->child(); 
		while(n) 
		{
			str = n->get_data();
			if(0==g_cly_lang_utf8) 
				cl_unicode::UTF_8ToGB2312(str,str.c_str(),str.length());
			ls.push_back(str);
			n=n->next(); 
		} 
		return 0; 
	} 
	return -1;
}

int cly_xml_get_seg_task(const char* xmlstr,string& task_name,list<string>& ls_hids,list<string>& ls_files)
{
	cl_xml xml;

	if(0!=xml.load_string(xmlstr))
		return -1;
	cl_xmlNode* n = xml.find_first_node_attri("root/p","n","task_name");
	if(n) 
		task_name = n->get_data();
	cly_xml_get_list(xml,ls_hids,"hids"); 
	cly_xml_get_list(xml,ls_files,"files"); 
	return 0;
}
int cly_xml_get_report_progress(const char* xmlstr,int& peer_id,list<string>& ls)
{
	cl_xml xml;
	peer_id = 0;
	int retcode = 0;

	if(0!=xml.load_string(xmlstr))
		return -1;
	else
	{
		_XML_FINE_SEG_LOOP_CHILD_PRE("root/seg","id","client")
		if(0==strcmp(a,"peer_id")) peer_id = atoi(n->get_data());
		_XML_FINE_SEG_LOOP_CHILD_END
		retcode = ret;
	}
	if(peer_id<=0)
		return -1;
	
	retcode = 0;
	string str;
	cl_xmlNode *n = xml.find_first_node_attri("root/list","id","progress"); 
	if(n) 
	{
		ls.push_back(n->attri.get_attri("fields"));
		n = n->child(); 
		while(n) 
		{
			ls.push_back(n->get_data());
			n=n->next(); 
		} 
		return 0; 
	} 
	return retcode;
}

//*****************************************************************************

void cly_tp_rspxml_add_seg_list(char* xmlbuf,const char* idname,list<string>& ls)
{
	sprintf(xmlbuf+strlen(xmlbuf),"	<list id=\"%s\" len=\"%d\">\r\n ",idname,ls.size());
	for(list<string>::iterator it=ls.begin();it!=ls.end();++it)
		sprintf(xmlbuf+strlen(xmlbuf),"		<p>%s</p>\r\n",(*it).c_str());
	sprintf(xmlbuf+strlen(xmlbuf),"	</list>\r\n ");
}
