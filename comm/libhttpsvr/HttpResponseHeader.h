#pragma once

#include "cl_incnet.h"
#include "cl_basetypes.h"

class HttpResponseHeader
{
public:
	HttpResponseHeader(void);
	~HttpResponseHeader(void);

	HttpResponseHeader(const string& header):m_sHeader(header) {}

public:
//Methods
  string GetField(const string& name);
  static string GetField(const string& head,const string& name);

  void    AddStatusCode(int nStatusCode);
  void    AddString(const string& sAdd);
  void    AddContentLength(int nSize);
  void    AddContentLength(long long nSize);
  void    AddContentType(const string& sMediaType);
  void    AddDate(const SYSTEMTIME& st);
  void    AddLastModified(const SYSTEMTIME& st);
  void    AddWWWBasicAuthenticate(const string& sRealm);
  void    AddExpires(const SYSTEMTIME& st);
  void    AddLocation(const string& sLocation);
  void    AddServer(const string& sServer);
  void    AddMyAllowFields();
  string  DateToStr(const SYSTEMTIME& st);
  BOOL    Send(SOCKET socket,int timeoutms);

public:
 string m_sHeader;
};
