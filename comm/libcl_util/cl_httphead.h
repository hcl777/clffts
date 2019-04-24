#pragma once
#include "cl_basetypes.h"

class cl_httphead
{
public:
	cl_httphead(void);
	~cl_httphead(void);

public:
	static int get_code(const string& header);
	static int get_field(const string& header,const string& session, string& text);
};


