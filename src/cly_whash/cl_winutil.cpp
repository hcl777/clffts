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
	// 弹出文件夹
	TCHAR           szFolderPath[MAX_PATH] = {0};  
    CString         strFolderPath = TEXT("");  
          
    BROWSEINFO      sInfo;  
    ::ZeroMemory(&sInfo, sizeof(BROWSEINFO));  
    sInfo.pidlRoot   = 0;  
    sInfo.lpszTitle   = _T("请选择一个文件夹：");  
    sInfo.ulFlags   = BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;  
    sInfo.lpfn     = NULL;  
  
    // 显示文件夹选择对话框  
    LPITEMIDLIST lpidlBrowse = ::SHBrowseForFolder(&sInfo);   
    if (lpidlBrowse != NULL)  
    {  
        // 取得文件夹名  
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

