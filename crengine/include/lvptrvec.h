/** \file lvptrvec.h
	\brief pointer vector template

	Implements vector of pointers.

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.

	Refactoring by virxkane, 2018

*/

#ifndef __LVPTRVEC_H_INCLUDED__
#define __LVPTRVEC_H_INCLUDED__

#include <stdlib.h>
#include "lvmemman.h"

/** \brief template which implements vector of pointer

	Automatically deletes objects when vector items are destroyed.
*/
template < class T, bool ownItems = true >
class LVPtrVector
{
	T * * _list;
	int _size;
	int _count;
	LVPtrVector & operator = (LVPtrVector&) {
		// no assignment
		return *this;
	}
public:
	/// default constructor
	LVPtrVector() : _list(NULL), _size(0), _count(0) {}
	/// retrieves item from specified position
	T * operator [] ( int pos ) const {
		if (!_list || pos < 0 || pos >= _count)
			crFatalError(CR_FATAL_ERROR_INDEX_OUT_OF_BOUND, "index out of bounds");
		return _list[pos];
	}
	/// returns pointer array
	T ** get() { return _list; }
	/// retrieves item from specified position
	T * get( int pos ) const {
		if (!_list || pos < 0 || pos >= _count)
			crFatalError(CR_FATAL_ERROR_INDEX_OUT_OF_BOUND, "index out of bounds");
		return _list[pos];
	}
	/// retrieves item reference from specified position
	T * & operator [] ( int pos ) {
		if (!_list || pos < 0 || pos >= _count)
			crFatalError(CR_FATAL_ERROR_INDEX_OUT_OF_BOUND, "index out of bounds");
		return _list[pos];
	}
	/// ensures that size of vector is not less than specified value
	bool reserve( int size )
	{
		bool res = true;
		if ( size > _size )
		{
			void* tmp = realloc( _list, size * sizeof( T* ));
			if (tmp) {
				_list = (T**)tmp;
				for (int i=_size; i<size; i++)
					_list[i] = NULL;
				_size = size;
			}
			else {
				// TODO: crFatalError() or even thow exception?
				res = false;
			}
		}
		return res;
	}
	void sort(int (comparator)(const T ** item1, const T ** item2 ) ) {
		qsort(_list, _count, sizeof(T*), (int (*)(const void *, const void *))comparator);
	}
	/// sets item by index (extends vector if necessary)
	bool set( int index, T * item )
	{
		if (!reserve( index+1 ))
			return false;

		while (length()<index)
			add(NULL);
		if ( ownItems && _list[index] )
			delete _list[index];
		_list[index] = item;
		if (_count<=index)
			_count = index + 1;
		return true;
	}
	/// returns size of buffer
	int size() const { return _size; }
	/// returns number of items in vector
	int length() const { return _count; }
	/// returns true if there are no items in vector
	bool empty() const { return _count==0; }
	/// clears all items
	void clear()
	{
		if (_list)
		{
			int cnt = _count;
			_count = 0;
			if ( ownItems ) {
				for (int i=cnt - 1; i>=0; --i)
					if (_list[i])
						delete _list[i];
			}
			free( _list );
			_list = NULL;
		}
		_size = 0;
		_count = 0;
	}
	/// removes several items from vector
	void erase( int pos, int count )
	{
		if ( count<=0 )
			return;
		if (!_list || pos<0 || pos+count > _count)
			crFatalError();
		int i;
		for (i=0; i<count; i++)
		{
			if (_list[pos+i])
			{
				if ( ownItems )
					delete _list[pos+i];
				_list[pos+i] = NULL;
			}
		}
		for (i=pos+count; i<_count; i++)
		{
			_list[i-count] = _list[i];
			_list[i] = NULL;
		}
		_count -= count;
	}
	/// removes item from vector by index
	T * remove( int pos )
	{
		if (!_list || pos < 0 || pos > _count)
			crFatalError();
		int i;
		T * item = _list[pos];
		for ( i=pos; i<_count-1; i++ )
		{
			_list[i] = _list[i+1];
			//_list[i+1] = NULL;
		}
		_count--;
		return item;
	}
	/// returns vector index of specified pointer, -1 if not found
	int indexOf( T * p )
	{
		for ( int i=0; i<_count; i++ ) {
			if ( _list[i] == p )
				return i;
		}
		return -1;
	}
	T * last()
	{
		if ( _count<=0 )
			return NULL;
		return _list[_count-1];
	}
	T * first()
	{
		if ( _count<=0 )
			return NULL;
		return _list[0];
	}
	/// removes item from vector by index
	T * remove( T * p )
	{
		int i;
		int pos = indexOf( p );
		if ( pos<0 )
			return NULL;
		T * item = _list[pos];
		for ( i=pos; i<_count-1; i++ )
		{
			_list[i] = _list[i+1];
		}
		_count--;
		return item;
	}
	/// adds new item to end of vector
	bool add( T * item ) { return insert( -1, item ); }
	/// inserts new item to specified position
	bool insert( int pos, T * item )
	{
		bool res = true;
		if (pos<0 || pos>_count)
			pos = _count;
		if ( _count >= _size )
			res = reserve( _count * 3 / 2  + 8 );
		if (res) {
			for (int i=_count; i>pos; --i)
				_list[i] = _list[i-1];
			_list[pos] = item;
			_count++;
		}
		return res;
	}
	/// move item to specified position, other items will be shifted
	void move( int indexTo, int indexFrom )
	{
		if ( indexTo==indexFrom )
			return;
		T * p = _list[indexFrom];
		if ( indexTo<indexFrom ) {
			for ( int i=indexFrom; i>indexTo; i--)
				_list[i] = _list[i-1];
		} else {
			for ( int i=indexFrom; i<indexTo; i++)
				_list[i] = _list[i+1];
		}
		_list[ indexTo ] = p;
	}
	/// copy constructor
	LVPtrVector( const LVPtrVector & v )
		: _list(NULL), _size(0), _count(0)
	{
		if ( v._count>0 ) {
			if (reserve( v._count )) {
				for ( int i=0; i<v._count; i++ )
					add( new T(*v[i]) );
			}
			else
				crFatalError(-2, "Failed to reserve memory");
		}
	}
	/// stack-like interface: pop top item from stack
	T * pop()
	{
		if ( empty() )
			return NULL;
		return remove( length() - 1 );
	}
	/// stack-like interface: pop top item from stack
	T * popHead()
	{
		if ( empty() )
			return NULL;
		return remove( 0 );
	}
	/// stack-like interface: push item to stack
	void push( T * item )
	{
		add( item );
	}
	/// stack-like interface: push item to stack
	bool pushHead( T * item )
	{
		return insert( 0, item );
	}
	/// stack-like interface: get top item w/o removing from stack
	T * peek()
	{
		if ( empty() )
			return NULL;
		return get( length() - 1 );
	}
	/// stack-like interface: get top item w/o removing from stack
	T * peekHead()
	{
		if ( empty() )
			return NULL;
		return get( 0 );
	}
	/// destructor
	~LVPtrVector() { clear(); }
};

#endif	// __LVPTRVEC_H_INCLUDED__
