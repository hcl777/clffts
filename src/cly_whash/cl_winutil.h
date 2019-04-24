#pragma once

#ifdef _WIN32
#include "cl_util.h"
#include <afxstr.h>

class cl_winutil
{
private:
	cl_winutil(void);
	~cl_winutil(void);
public:
	static CString get_browse_folder();
};


#endif
