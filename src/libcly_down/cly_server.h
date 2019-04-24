#pragma once
#include "uac_SocketSelector.h"
#include "cl_singleton.h"
#include "cly_peer.h"
#include "cly_serverData.h"
#include "cl_thread.h"
#include "cl_MessageQueue.h"
/*
˵����
�������ݣ������Ѿ������������ļ����ö��߳�Ԥ�����ݵķ�ʽ�����������е���ԭ�̶߳���
ͬһ��readyfile ��֤��ͬһ���߳�������������ݣ���һ��ʼ��ѡ��ͬһ���̡߳�
*/




class cly_server : public cl_thread
	,public UAC_SocketAcceptor
	,public cl_timerHandler
	,public cly_peerListener

{
	typedef cl_SimpleMutex Mutex;
	typedef cl_TLock<Mutex> Lock;
	typedef struct tag_taskinfo
	{
		int					index;//ָ��ҪԤ���Ŀ�
		string				hash; //���ڼ����ļ����
		string				path; //
		cly_svrPeerData		*data;
		tag_taskinfo(int i,const string& h,const string& apath,cly_svrPeerData* d)
			:index(i)
			,hash(h)
			,path(apath)
			,data(d)
		{}
	}taskinfo_t;
	

public:	
	cly_server(void);
	virtual ~cly_server(void);
public:
	int init(int max_peernum,int thread_num);
	void fini();
	void stop_all();
	void on_delete_file(const string& hash);
	virtual int work(int e);
	int get_peer_num()const { return m_peers.size(); }

	virtual bool uac_attach_socket(UAC_SOCKET fd,const UAC_sockaddr& addr);
	
	virtual void on(cly_peerListener::Connected,cly_peer* peer);
	virtual void on(cly_peerListener::Disconnected,cly_peer* peer);
	virtual void on(cly_peerListener::Data,cly_peer* peer,char* buf,int len);
	virtual void on(cly_peerListener::Writable,cly_peer* peer);

	virtual void on_timer(int e);

	int get_allspeed(int& allspeed,list<string>& ls);
private:
	void read_block(cly_svrPeerData* data, int index, const string& hash, const string& path);
private:
	int					m_max_peernum;
	list<cly_peer*>		m_peers;
	cl_MessageQueue		*m_queues;
	int					*m_thread_ref;
	int					m_thread_num;

};
typedef cl_singleton<cly_server> cly_serverSngl;
