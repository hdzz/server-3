#ifndef _CMIMEMORY_H_
#define _CMIMEMORY_H_

#include<stdlib.h>
#include"utils.h"

class CmiMemory
{
public:

	CmiMemory(){}
	virtual ~CmiMemory(){}

	void* operator new(size_t size)
	{
		return malloc(size);
	}

	void operator delete(void *p)
	{
		free(p);
	}


	void* operator new[](size_t size)
	{
		return malloc(size);
	}


	void operator delete[](void *p)
	{
		free(p);
	}
};


class CmiMemoryRef
{
public:

	CmiMemoryRef()
		:ref(1)
	{
	}
	virtual ~CmiMemoryRef(){}

	void addRef(){ref++;}
	int32_t getRef(){ return ref; }

	void release()
	{
		ref--;
		if (ref == 0){
			delete this;
		}
	}

	void* operator new(size_t size)
	{
		return malloc(size);
	}


	void operator delete(void *p)
	{
		if(	--((CmiMemoryRef*)p)->ref <= 0)
			free(p);
	}

	void* operator new[](size_t size){return 0;}
	void operator delete[](void *p){}

private:
	int32_t ref;
};

#endif