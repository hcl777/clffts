
// cly_whash.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// Ccly_whashApp:
// �йش����ʵ�֣������ cly_whash.cpp
//

class Ccly_whashApp : public CWinApp
{
public:
	Ccly_whashApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern Ccly_whashApp theApp;