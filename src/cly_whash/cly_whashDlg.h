
// cly_whashDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "cl_thread.h"
#include "cl_MessageQueue.h"

// Ccly_whashDlg 对话框
class Ccly_whashDlg : public CDialogEx
	,public cl_thread
{
// 构造
public:
	Ccly_whashDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_CLY_WHASH_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
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
