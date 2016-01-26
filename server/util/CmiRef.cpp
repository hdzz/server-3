#include"CmiRef.h"

CmiRef::CmiRef()
: _referenceCount(1)
{
}

CmiRef::~CmiRef()
{
}

void CmiRef::retain()
{
	++_referenceCount;
}

void CmiRef::release()
{
	--_referenceCount;

	if (_referenceCount == 0)
	{
		delete this;
	}
}

unsigned int CmiRef::getReferenceCount() const
{
	return _referenceCount;
}