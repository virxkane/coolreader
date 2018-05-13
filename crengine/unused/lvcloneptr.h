#ifndef LVCLONEPTR_H
#define LVCLONEPTR_H

// This class is unused, saved for history.

#include <stddef.h>

/// unique pointer
template <class T >
class LVClonePtr {
	T * p;
public:
	LVClonePtr()
		: p(NULL)
{
	}
	explicit LVClonePtr( T* ptr )
		: p(ptr ? (T*)ptr->clone() : NULL)
	{
	}
	LVClonePtr( const LVClonePtr & v ) { p = v.p ? (T*)v.p->clone() : NULL; }
	LVClonePtr & operator = (const LVClonePtr & v) {
		clear();
		p = v.p ? (T*)v.p->clone() : NULL;
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
	~LVClonePtr()
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
	inline T & operator [] (int index) { return p[index]; }
	inline T * get() const { return p; }
	inline T & operator * ()
	{
		return *p;
	}
	inline const T & operator * () const
	{
		return *p;
	}
	inline LVClonePtr & operator = ( T* ptr )
	{
		if ( p==ptr )
			return *this;
		if ( p )
			delete p;
		p = ptr ? (T*)ptr->clone() : NULL;
		return *this;
	}
};

#endif // LVCLONEPTR_H
