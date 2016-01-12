#ifndef  _CMIREF_H_
#define  _CMIREF_H_


class CmiRef
{
public:

	void retain();
	void release();
	unsigned int getReferenceCount() const;

protected:
	CmiRef();

public:
	virtual ~CmiRef();

protected:
	unsigned int _referenceCount;
};

#endif
