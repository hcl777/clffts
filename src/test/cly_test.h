#pragma once

#include "cl_thread.h"


class cly_testLock : public cl_thread
{
public:
	cly_testLock();
	virtual ~cly_testLock();

	void init();

	virtual int work(int e);

public:
	
};

