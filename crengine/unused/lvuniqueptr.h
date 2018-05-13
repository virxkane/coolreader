#ifndef LVUNIQUEPTR_H
#define LVUNIQUEPTR_H

// This class is unused, saved for history.

#include <stddef.h>

/// unique pointer
template <class T >
class LVUniquePtr {
	T * p;
public:
	LVUniquePtr()
		: p(NULL)
{
	}
	explicit LVUniquePtr( T* ptr )
		: p(ptr)
	{
	}
	LVUniquePtr( const LVUniquePtr & v ) { p = v.p; v.p = NULL; }
	LVUniquePtr & operator = (const LVUniquePtr & v) {
		clear();
		p = v.p; v.p = NULL;
		return *this;
	} // no copy
	bool isNull() const {
		return p == NULL;
	}
	bool operator !() const { return p == NULL; }
	inline void clear()
	{
		if (p)
			delete p;
		p = NULL;
	}
	~LVUniquePtr()
	{
		clear();
	}
	inline T * operator -> ()
	{
		return p;
	}
	inline const T * operator -> () const
	{
		return p;
	}
	inline T * get() const { return p; }
	inline T & operator * ()
	{
		return *p;
	}
	inline const T & operator * () const
	{
		return *p;
	}
	inline LVUniquePtr & operator = ( T* ptr )
	{
		if ( p==ptr )
			return *this;
		if ( p )
			delete p;
		p = ptr;
		return *this;
	}
};

#endif	// LVUNIQUEPTR_H
