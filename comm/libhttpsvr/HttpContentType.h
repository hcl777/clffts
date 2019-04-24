#pragma once
#include "cl_basetypes.h"

class HttpContentType
{
public:
	HttpContentType(void);
	~HttpContentType(void);

	string get_ct_name(const string& key);
	string operator [](const string& key);

private:
	map<string,string> m_ct_map;
};

extern HttpContentType http_content_type;
