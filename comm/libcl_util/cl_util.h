#pragma once
#include "cl_basetypes.h"

#ifdef _WIN32
typedef HANDLE PSL_HANDLE;
#else
typedef int PSL_HANDLE;
#endif

//typedef struct tag_ifinfoi
//{
//	bool isloopback;
//	bool isup;
//	char name[128];
//	char ip[32];
//	char mask[32];
//	char mac[16];
//}ifinfoi_s;
//
//typedef struct tag_ifinfo
//{
//	int ifcount;
//	ifinfoi_s ifs[10];
//}ifinfo_s;
//

class cl_util
{
public:
	static void linux_register_signal_exit(void (*FUNC)(int)); //只有linux用
	static void wait_exit();
	static void signal_exit();
	static void debug_memleak();
	static int chdir(const char* path);
	static string get_module_path();
	static string get_module_dir();
	static string get_module_name();
	static PSL_HANDLE lockname_create(const char* name);
	static PSL_HANDLE process_single_lockname_create();
	static void process_single_lockname_close(PSL_HANDLE h);
	static int get_system_version();
	static string get_system_version_str();
	static int get_all_volumes(list<string>& ls);
	static bool get_volume_size(const string& volume,ULONGLONG &total,ULONGLONG &used,ULONGLONG &free);
	static int my_create_directory(const string& path);
	static int create_directory_by_filepath(const string& filepath);
	static int file_state(const char* path);
	static bool file_same(const string& path1, const string& path2);
	static bool file_exist(const string& path);
	static int file_delete(const string& path);
	static int file_rename(const string& from,const string& to);

	static bool get_stringlist_from_file(const string& path,list<string>& ls);
	static bool put_stringlist_to_file(const string& path,list<string>& ls);

	static int get_file_modifytime(const char* path);
	static int get_filetimes(const char* path,time_t& ctime,time_t& mtime,time_t& atime);
	static long get_folder_files(const string& dir,
						list<string>& ls_path,
						list<int>& ls_ino,
						const string& suffix = "",
						int enable_samefile = 0,
						bool inherit = true);
	//返回当前层文件夹的所有文件和文件夹.不递归.
	static int get_folder_nodes(const string& dir,list<string>& folders,list<string>& files);
	
	
	static string get_filename(const string& path);
	static string get_filename_prename(const string& path);
	static string get_filename_extension(const string& path);
	static string get_filedir(const string& path);
	static void filepath_split(const string& path,string& dir,string& prename,string& ext);
	static string& filepath_format(string& path);
	static string& dir_add_tail_symbol(string& dir); //尾部加"/"
	static string& dir_del_tail_symbol(string& dir); //尾部去掉"/"
	static string& dir_to_fulldir(string& dir,const string& referdir); //转成绝对路径,referdir必须是绝对路径

	static int atoi(const char* _Str,int nullval=0); //多一步判断NULL时返回0
	static long long atoll(const char* _Str,int nullval=0);
	static char* itoa_buf(char* buf,int i);
	static string itoa(int i);
	static string hex2str_x(const unsigned char* dgst,unsigned int dgst_len);
	
	//*********************************
	static string get_string_index(const string& source,int index,const string& sp);
	static int get_string_index_pos(const string& source,int index,const string& sp);
	static int get_string_index_count(const string& source,const string& sp);
	static string set_string_index(string &source,int index,const string& val, const string& sp);
	static bool get_stringlist_from_string(const string& source,list<string>& ls);
	static char* string_trim(char* sz,char c=' ');
	static string& string_trim(string& str,char c=' ');
	static string& string_trim_endline(string& str);
	static string& str_replace(string& str,const string& str_old,const string& str_new);

	//*********************************
	static string get_disksn(const char* path);
	//static int get_umac(unsigned char umac[]);
	//static string get_mac();
	//static int get_macall(ifinfo_s *inf);
	//static int get_mtu();

	//static int get_server_time(time_t *t);
	
	//static bool is_ip(const char* ip);
	//static bool is_dev(const char* ip);
	//static string ip_explain(const char* s);
	//static string ip_explain_ex(const char* s,int maxTick=5000);
	//static char* ip_htoa(unsigned int ip);
	//static char* ip_ntoa(unsigned int nip);
	//static string ip_htoas(unsigned int ip);
	//static string ip_ntoas(unsigned int nip);
	//static unsigned int ip_atoh(const char* ip);
	//static unsigned int ip_atoh_try_explain_ex(const char* s,int maxTick=5000);

	//static  string get_local_private_ip();
	//static  string get_local_private_ip_ex(int timeout_tick=5000);
	//static  bool is_private_ip(const string& ip) ;

	//**********************************
	static int url_element_split(const string& url,string& server,unsigned short& port,string& cgi);
	static string url_get_name(const string& url);
	static string url_get_cgi(const string& url);
	static string url_get_parameter(const string& url,const string& parameter);
	static string url_get_last_parameter(const string& url,const string& parameter);
	static string url_cgi_get_path(const string& cgi);
	
	static string time_to_time_string(const time_t& _Time);
	static string time_to_datetime_string(const time_t& _Time);
	static string time_to_datetimeT_string(const time_t& _Time); //ISO datetime标准格式
	static string time_to_datetime_string2(const time_t& _Time);
	static string time_to_date_string(const time_t& _Time);

	//一般用于main()中的参数分析,返回str所在数据索引号
	static int string_array_find(int argc,char** argv,const char* str);

	static bool is_write_debug_log();
	static int write_debug_log(const char *strline,const char *path);
	static int write_log(const char *strline,const char *path);
	static int write_tlog(const char *path,int maxsize,const char* fomat,...);
	
	static int write_buffer_to_file(const char *buf,int size,const char *path);
	//如果打开失败-1,读不完整-2,非0返回大小
	static int read_buffer_from_file(char *buf,int size,const char *path);

	//cpuid;fn_id表示找第几号CPU
	static void get_cpuid(unsigned int cpuinfo[4], unsigned int fn_id=0);
};


