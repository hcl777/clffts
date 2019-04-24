
// cly_whashDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "cl_thread.h"
#include "cl_MessageQueue.h"

// Ccly_whashDlg �Ի���
class Ccly_whashDlg : public CDialogEx
	,public cl_thread
{
// ����
public:
	Ccly_whashDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_CLY_WHASH_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

protected:
	
	CString m_strExt;
	CString m_strDir;
	CButton m_btnStart;
	CListCtrl m_wndList;
	CListCtrl m_wndFini;
	CButton m_btnAdd;
	cl_MessageQueue m_queue;
public:
	void job();
	virtual int work(int e);
	int hash(const char* path);
public:
	afx_msg void OnNMRClickListD(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedBtnAdd();
	afx_msg void OnBnClickedBtnStart();
	afx_msg LRESULT OnMsgHashFini(WPARAM p1,LPARAM p2);
	CStatic m_staHash;
	afx_msg void OnDestroy();
	virtual void OnOK();  
    //virtual void OnCancel();  
};
