#ifndef _CMISINGLETON_H_
#define _CMISINGLETON_H_

#include<pthread.h>

class CResGuard {

public:
	CResGuard()  { pthread_mutex_init(&lock, NULL);}
	~CResGuard() { pthread_mutex_destroy(&lock); }

	void Guard()   { pthread_mutex_lock(&lock); }
	void Unguard() { pthread_mutex_unlock(&lock); }

private:
	pthread_mutex_t  lock;
};

/////////////////////////////////////////////////
template <class T>
class CmiSingleton
{
public:
	static inline T& instance();

protected:
	CmiSingleton(void){}
	~CmiSingleton(void){}

	CmiSingleton(const CmiSingleton&){}
	CmiSingleton & operator= (const CmiSingleton &){}


	static T* _instance;
	static CResGuard _rs;

};

#define DECLARE_SINGLETON_CLASS(type) \
friend class CmiSingleton<type>;

template <class T>
T* CmiSingleton<T>::_instance;

template <class T>
CResGuard CmiSingleton<T>::_rs;


template <class T>
inline T& CmiSingleton<T>::instance()
{

	if (0 == _instance)
	{
		_rs.Guard();
		if (0 == _instance)
		{
			_instance = new T;
		}
		_rs.Unguard();

	}

	return *_instance;

}
#endif

/*

template <typename T>
class singleton
{
private:
	struct object_creator
	{
		object_creator() { singleton<T>::instance(); }
		inline void do_nothing() const { }
	};

	static object_creator create_object;
	
	singleton(const singleton&);
    singleton& operator =(const singleton&);

protected:
	singleton(){}
	~singleton(){}
	
public:
	typedef T object_type;

	static object_type & instance()
	{
		static object_type obj;
		create_object.do_nothing();
		return obj;
	}
};

template <typename T> 
typename singleton<T>::object_creator 
singleton<T>::create_object; 

*/