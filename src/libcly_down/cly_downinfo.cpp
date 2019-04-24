#include "cly_downinfo.h"


int cly_soucre_str_section(const string& s,uint32& ip,uint16& port,int& ntype,uint32& private_ip,uint16& private_port,int& user_type)
{
	// [ip:port:nattype]-[private_ip:port]-[user_type]

	string str;
	str = cl_util::get_string_index(s,0,"-");
	ip = cl_net::ip_atoh(cl_util::get_string_index(str,0,":").c_str());
	port = (uint16)cl_util::atoi(cl_util::get_string_index(str,1,":").c_str());
	ntype = cl_util::atoi(cl_util::get_string_index(str,2,":").c_str());
		
	str = cl_util::get_string_index(s,1,"-");
	private_ip = cl_net::ip_atoh(cl_util::get_string_index(str,0,":").c_str());
	private_port = (uint16)cl_util::atoi(cl_util::get_string_index(str,1,":").c_str());

	user_type = cl_util::atoi(cl_util::get_string_index(s,2,"-").c_str());
	if(user_type<0) user_type=0;
	user_type = user_type%5; //(0жа4)
	if(0==ntype) ntype = 1;
	return 0;
}
