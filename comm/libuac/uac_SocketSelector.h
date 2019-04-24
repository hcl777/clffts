#pragma once
#include "uac_Socket.h"
#include "uac_Singleton.h"
#include <assert.h>

class UAC_SocketAcceptor
{
public:
	virtual ~UAC_SocketAcceptor(void){}
	virtual bool uac_attach_socket(UAC_SOCKET fd,const UAC_sockaddr& addr)=0;
};
class UAC_SocketSelector
{
public:
	UAC_SocketSelector(void);
	~UAC_SocketSelector(void);

	typedef struct tag_Node
	{
		UAC_Socket* s;
		char rw_mask;
		tag_Node(void):s(0),rw_mask(0){}
	}Node_t;
public:
	int register_socket(UAC_Socket* s,char rw_mask);
	int unregister_socket(UAC_Socket* s,char rw_mask);
	void set_acceptor(UAC_SocketAcceptor* apt) {m_apt=apt;}

	//循环调用
	int handle_readwrite(); //返回1时表示有可读的连接未执行接收 (限速用，上层见1时考虑Sleep一下以免高CPU)
	void handle_accept();
private:
	Node_t* m_chs;
	int m_regnum; //
	UAC_SocketAcceptor* m_apt;
	UAC_fd_set m_rset,m_wset;
	UAC_sockaddr _client_addr;
};
typedef UAC::Singleton<UAC_SocketSelector> UAC_SocketSelectorSngl;

