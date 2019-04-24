#include "stdafx.h"
#include "cl_winutil.h"
#ifdef _WIN32
#include <ShlObj.h>

cl_winutil::cl_winutil(void)
{
}


cl_winutil::~cl_winutil(void)
{
}

CString cl_winutil::get_browse_folder()
{
	// �����ļ���
	TCHAR           szFolderPath[MAX_PATH] = {0};  
    CString         strFolderPath = TEXT("");  
          
    BROWSEINFO      sInfo;  
    ::ZeroMemory(&sInfo, sizeof(BROWSEINFO));  
    sInfo.pidlRoot   = 0;  
    sInfo.lpszTitle   = _T("��ѡ��һ���ļ��У�");  
    sInfo.ulFlags   = BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;  
    sInfo.lpfn     = NULL;  
  
    // ��ʾ�ļ���ѡ��Ի���  
    LPITEMIDLIST lpidlBrowse = ::SHBrowseForFolder(&sInfo);   
    if (lpidlBrowse != NULL)  
    {  
        // ȡ���ļ�����  
        if (::SHGetPathFromIDList(lpidlBrowse,szFolderPath))    
        {  
            strFolderPath = szFolderPath;  
        }  
    }  
    if(lpidlBrowse != NULL)  
    {  
        ::CoTaskMemFree(lpidlBrowse);  
    }  
  
    return strFolderPath;  
  
}

#endif

