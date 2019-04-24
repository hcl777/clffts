
// cly_whashDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "cly_whash.h"
#include "cly_whashDlg.h"
#include "afxdialogex.h"
#include "cl_winutil.h"
#include "cl_wchar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Ccly_whashDlg �Ի���

#define UM_HASH_FINI WM_USER+1

Ccly_whashDlg::Ccly_whashDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(Ccly_whashDlg::IDD, pParent)
	, m_strExt(_T("mkv,ic2,mp4"))
	, m_strDir(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Ccly_whashDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_EXT, m_strExt);
	DDX_Text(pDX, IDC_EDIT_DIR, m_strDir);
	DDX_Control(pDX, IDC_BTN_ADD, m_btnAdd);
	DDX_Control(pDX, IDC_BTN_START, m_btnStart);
	DDX_Control(pDX, IDC_LIST_D, m_wndList);
	DDX_Control(pDX, IDC_LIST_D2, m_wndFini);
	DDX_Control(pDX, IDC_STATIC_HASH, m_staHash);
}

BEGIN_MESSAGE_MAP(Ccly_whashDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_RCLICK, IDC_LIST_D, &Ccly_whashDlg::OnNMRClickListD)
	ON_BN_CLICKED(IDC_BTN_ADD, &Ccly_whashDlg::OnBnClickedBtnAdd)
	ON_BN_CLICKED(IDC_BTN_START, &Ccly_whashDlg::OnBnClickedBtnStart)
	ON_MESSAGE(UM_HASH_FINI,&Ccly_whashDlg::OnMsgHashFini)
//	ON_WM_CLOSE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// Ccly_whashDlg ��Ϣ�������

BOOL Ccly_whashDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	m_wndList.InsertColumn(0,_T("·��"),0,300);
	//m_wndList.InsertColumn(1,_T("��С"),0,150);
	m_wndFini.InsertColumn(0,_T("·��"),0,300);
	//m_wndList.ShowScrollBar(SB_BOTH,1);
	//m_wndFini.ShowScrollBar(SB_BOTH,1);
	this->activate();

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void Ccly_whashDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR Ccly_whashDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void Ccly_whashDlg::OnNMRClickListD(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	//@TN
		*pResult = 0;
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;
}


void Ccly_whashDlg::OnBnClickedBtnAdd()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	
	UpdateData(TRUE);
	CString path = cl_winutil::get_browse_folder();
	if(path.IsEmpty())
		return ;
	m_strDir = path;
	UpdateData(FALSE);

	string dir,ext,str;
	char buf[1024];
	_W2A(buf,1024,m_strDir);
	dir = buf;
	if(!m_strExt.IsEmpty())
	{
		_W2A(buf,1024,m_strExt);
		ext = buf;
	}
	cl_util::str_replace(ext,",","|");

	list<string> ls;
	list<int>		ls_ino;
	list<string>::iterator it;
	int n,i;
	ls.clear();
	cl_util::get_folder_files(dir,ls,ls_ino,ext);
	m_wndList.DeleteAllItems();

	CString cstr;
	wchar_t wc[1024];
	for(it=ls.begin(),i=0;it!=ls.end();++it,i++)
	{
		_A2W(wc,1024,(*it).c_str());
		n = m_wndList.InsertItem(i,wc);
		//m_wndList.SetItemText(n,1,_T("0"));
	}
}


void Ccly_whashDlg::OnBnClickedBtnStart()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if(m_wndList.GetItemCount()<=0)
		return;
	m_wndFini.DeleteAllItems();
	m_btnAdd.EnableWindow(FALSE);
	m_btnStart.EnableWindow(FALSE);

	job();
}
void Ccly_whashDlg::job()
{
	if(m_wndList.GetItemCount()>0)
	{
		CString path = m_wndList.GetItemText(0,0);
		m_wndList.DeleteItem(0);
		m_staHash.SetWindowText(path);
		m_queue.SendMessage(new cl_Message(1,&path));
	}
	else
	{
		m_staHash.SetWindowText(_T("���!"));
		m_btnAdd.EnableWindow(TRUE);
		m_btnStart.EnableWindow(TRUE);
	}
}
int Ccly_whashDlg::work(int e)
{
	cl_Message *msg;
	CString path;
	while(1)
	{
		msg = m_queue.GetMessage();
		if(0==msg->cmd)
		{
			delete msg;
			break;
		}
		path = *(CString*)msg->data;
		delete msg;

		char spath[1024];
		_W2A(spath,1024,path);
		hash(spath);

		this->PostMessage(UM_HASH_FINI,(WPARAM)new CString(path),0);
	}
	return 0;
}

LRESULT Ccly_whashDlg::OnMsgHashFini(WPARAM p1,LPARAM p2)
{
	CString *path = (CString*)p1;
	m_wndFini.InsertItem(10000,*path);
	delete path;
	job();
	return 0;
}

//***********************

#include "cl_sha1.h"
#include "cl_fhash.h"
int rdb_sha1(const char* path)
{
	string hashpath = path;
	hashpath += ".sha";
	if(cl_util::file_exist(hashpath))
		return 0;

	char strHash[48];
	if(0!=Sha1_BuildFile(path,strHash,NULL,0,false))
		return -1;
	size64_t size = cl_ERDBFile64::get_filesize(path);

	list<string> ls;
	char buf[128];
	sprintf(buf,"%lld",size);
	ls.push_back(strHash);
	ls.push_back(buf);
	if(cl_util::put_stringlist_to_file(hashpath,ls))
		return 0;
	return -1;
}
#define CLY_FBLOCK_SIZE 102400
int rdb_sha3(const char* spath)
{
	string hashpath = spath;
	hashpath += ".sha3";
	if(cl_util::file_exist(hashpath))
		return 0;

	string path = spath;
	string hash,subhash;
	char buf[2048];
	if(0==cl_fhash_file_sbhash(path.c_str(),CLY_FBLOCK_SIZE,buf))
	{
		subhash = buf;
		hash = cl_fhash_sbhash_to_mainhash(subhash);
		cl_put_fhash_to_hashfile(hashpath,hash,subhash);
	}
	printf("hash   :	%s\nsubhash:	%s\n",hash.c_str(),subhash.c_str());
	return 0;
}
int Ccly_whashDlg::hash(const char* path)
{
	rdb_sha1(path);
	rdb_sha3(path);
	return 0;
}


void Ccly_whashDlg::OnDestroy()
{
	__super::OnDestroy();

	// TODO: �ڴ˴������Ϣ����������
	m_queue.SendMessage(new cl_Message(0,0));
	wait();
}
void Ccly_whashDlg::OnOK()
{
}
//void Ccly_whashDlg::OnCancel()
//{
//}

