
#ifdef MAIN_LINUX
#if defined(__GNUC__) && defined(__linux__)

#include <stdio.h>
#include <stdlib.h>
#include "cl_util.h"
#include "cl_net.h"
#include "cly_d.h"
#include "cl_unicode.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

//�����Ǽ��ֳ������źţ�
//SIGHUP=1 �����ն��Ϸ����Ľ����ź�.
//SIGINT=2   �����Լ��̵��ж��ź� ( ctrl + c ) .
//SIGKILL=9 �����źŽ��������źŵĽ��� . ���ܲ���
//SIGTERM=15��kill ����� ���ź�.
//SIGCHLD=17����ʶ�ӽ���ֹͣ��������ź�. 
//SIGSTOP=19�����Լ��� ( ctrl + z ) ����Գ����ִֹͣ���ź�.. ���ܲ���
//SIGSEGV=11: �δ��� 
#define SIG_CHILD_EXIT 99

/*
SIGSEGV core dump����
��1��gcc -g ����     
ulimit -c 20000      
֮�����г���
��core dump      
���gdb -c core <exec file>      
�������ջ
��2��ʹ��strace execfile�����г��򣬳���ʱ����ʾ�Ǹ�ϵͳ���ô� 
 */

int			g_exiting_state;
int			g_i_sig_kill = 0;
bool		g_is_child = false;
int			g_kill_child_sig = 0;
pid_t		*g_pids = NULL;
int			g_pidnum = 0;
cly_config_t g_conf;

typedef struct tagPCThreadData
{
	int i;
	cly_config_t* pc;
}PCThreadData_t;

void sig_handler_exit(int sig);
void kill_all_child();
void* _thread_child(void* p);
void child_root(cly_config_t* pc);

int main(int argc,char** argv)
{
	string appdir = cl_util::get_module_dir();
	cl_unicode::setlocal_gb2312();
	//-v version:
	if(cl_util::string_array_find(argc,argv,"-h")>0
		||cl_util::string_array_find(argc,argv,"--help")>0
		||cl_util::string_array_find(argc,argv,"-v")>0)
	{
		cly_print_help();
		return 0;
	}

	//load config:
	string errmsg;

	if(0!=cly_config_load(g_conf))
	{
		printf("*** load xml config faild! \n");
		return -1;
	}
	//down_path
	int argi = 0;
	string down_path;
	if(0<(argi=cl_util::string_array_find(argc,argv,"--down_path")))
	{
		if(argi+1<argc)
			down_path = argv[argi+1];
		cl_util::str_replace(down_path,"#","|");
		cl_util::str_replace(down_path,"@",">");
	}
	if(!down_path.empty())
		g_conf.down_path = down_path;

	//-c  check config:
	if(cl_util::string_array_find(argc,argv,"-c")>0)
	{
		char buf[2048];
		cly_config_sprint(&g_conf,buf,2048);
		printf("%s",buf);
		return 0;
	}

	//singleton:
	cl_net::socket_init();
	PSL_HANDLE pslh = cl_util::process_single_lockname_create();
	if(0==pslh)
	{
		printf("*** %s is runing! \n",cl_util::get_module_name().c_str());
		return 0;
	}

	//linux run:
	cl_util::write_tlog((appdir+"run.log").c_str(),1024,"++++++main[pid:%d] run  in %d process .",getpid(),g_conf.pcls.size());
	char pidbuf[32];
	sprintf(pidbuf,"%d",(int)getpid());
	cl_util::write_buffer_to_file(pidbuf,strlen(pidbuf),(cl_util::get_module_path()+".pid").c_str());
		
	//��ʼ���źźţ���������ֹʱkill�������ӳ���
	signal(SIGHUP,sig_handler_exit);
	signal(SIGINT,sig_handler_exit);
	signal(SIGTERM,sig_handler_exit);
	
	//�����޸�core�Ĵ�СΪ500K
	//�����Գ�����ִ����Ч
	//system("ulimit -c 500000");
	
	pthread_t hthread=0;
	g_pidnum = g_conf.pcls.size();
	g_pids = new int[g_pidnum];
	memset(g_pids,0,g_pidnum*sizeof(int));
	PCThreadData_t *td;
	int i = 0;
	for(list<cly_config_t*>::iterator it=g_conf.pcls.begin();it!=g_conf.pcls.end();++it)
	{
		hthread = 0;
		td = new PCThreadData_t();
		td->i = i++;
		td->pc = *it;
		if(0 == pthread_create(&hthread,NULL,_thread_child,(void*)td))
		{
			pthread_detach(hthread);
		}
		Sleep(300);
	}

	//�������յ��˳�����ʱ��1
	while(0==g_exiting_state)
		Sleep(300);

	//g_i_sig_kill��ʾ���ⲿ�źŵ��µ���ֹ���ȴ�kill���ӽ��̣���Ϊ2��
	while(1==g_i_sig_kill)
		Sleep(100);
	//0��ʾ��س��������˳���Ҫ������kill�ӽ���
	if(0==g_i_sig_kill)
		kill_all_child();
	delete[] g_pids;

	if(2==g_exiting_state)
	{
		//��������
		sleep(1);
		char *exec_argv[1];
		exec_argv[0] = NULL;
		execv(cl_util::get_module_path().c_str(),exec_argv);
	}

	//end
	cly_config_free(g_conf);
	cl_net::socket_fini();
	cl_util::process_single_lockname_close(pslh);
	return 0;
}


void sig_handler_exit(int sig)
{
	if(0==g_exiting_state)
	{
		g_exiting_state = 1;
		if(!g_is_child)
		{
			g_i_sig_kill = 1;
			kill_all_child();
		
			cl_util::write_tlog("./run.log",1024,"------main(pid:%d) signal(%d) exit !!!",getpid(),sig);
			g_i_sig_kill = 2;
		}
		else
		{
			//fini
			clyd::instance()->fini();
			clyd::destroy();

			cl_util::write_tlog("./run.log",1024,"--child(pid:%d) signal(%d) exit!",getpid(),sig);

			g_kill_child_sig = sig;

		}
		printf("%d: %s sig_handler_exit(%d) end!! \n",getpid(),g_is_child?"child":"parent",sig);
	}
}

void kill_all_child()
{
	printf("%d: kill all child  ... \n",getpid());
	for(int i=0;i<g_pidnum;++i)
		kill(g_pids[i],SIGTERM);
}

void* _thread_child(void* p)
{
	PCThreadData_t * td = (PCThreadData_t*)p;
	cly_config_t* pc = td->pc;
	int i = td->i;
	delete td;
	pid_t pid = 0;
	while(!g_exiting_state)
	{
		pid = fork();
		if(pid<0)
			break;
		if(pid>0)
		{
			g_pids[i] = pid;
			waitpid(pid,NULL,0);
			if(!g_exiting_state)
			{
				printf(" pc end!!! try restart...! \n");
				cl_util::write_tlog("./run.log",1024,"*** child process(pid:%d) end and restart.", (int)pid);
				Sleep(300);
			}
			else
			{
				break;
			}
		}
		else if(0==pid)
		{
			g_is_child = true;
			child_root(pc);
			break;
		}
	}
	return 0;
}
void child_root(cly_config_t* pc)
{
	signal(SIGHUP,sig_handler_exit);
	signal(SIGINT,sig_handler_exit);
	signal(SIGTERM,sig_handler_exit);

	cl_util::write_tlog("./run.log",1024,"++p(pid:%d) run...",getpid());
	//init
	clyd::instance()->init(pc);

	while(!g_kill_child_sig) 
		Sleep(500);

	//����Ӧ���ͷ�����,��Ϊfork�������ݶ��ڴ�
	cly_config_free(g_conf);
}

#endif

#endif

