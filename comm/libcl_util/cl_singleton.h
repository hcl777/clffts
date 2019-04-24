#pragma once

template<class TYPE>
class cl_singleton
{
public:
	cl_singleton(void){}
	virtual ~cl_singleton(void){}
private:
	static TYPE* _instance;

public:
	static TYPE* instance()
	{
		if(0==_instance)
		{
			_instance = new TYPE();
		}
		return _instance;
	}
	static void instance(TYPE *ins)
	{
		destroy();
		_instance = ins;
	}
	static void destroy()
	{
		if(_instance)
		{
			delete _instance;
			_instance = 0;
		}
	}

private:
	cl_singleton(const cl_singleton&);
	cl_singleton& operator=(const cl_singleton&);

};
template<class TYPE> TYPE* cl_singleton<TYPE>::_instance = 0;


