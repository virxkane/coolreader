/** \file lvrefvec.h
	\brief vector of smart pointers

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.
	See LICENSE file for details.

	Refactoring by virxkane, 2018
*/

#ifndef __LVREFVEC_H_INCLUDED__
#define __LVREFVEC_H_INCLUDED__

#include "lvref.h"

template <typename T >
class LVRefVec
{
	LVRef<T> * _array;
	int _size;
	int _count;
public:
	/// default constructor
	LVRefVec() : _array(NULL), _size(0), _count(0) {}
	/// creates array of given size
	LVRefVec( int len, LVRef<T> value )
	{
		_size = _count = len;
		_array = new LVRef<T>[_size];
		for (int i=0; i<_count; i++)
			_array[i] = value;
	}
	LVRefVec( const LVRefVec & v )
	{
		_size = _count = v._count;
		if ( _size ) {
			_array = new LVRef<T>[_size];
			for (int i=0; i<_count; i++)
				_array[i] = v._array[i];
		} else {
			_array = NULL;
		}
	}
	LVRefVec & operator = ( const LVRefVec & v )
	{
		clear();
		_size = _count = v._count;
		if ( _size ) {
			_array = new LVRef<T>[_size];
			for (int i=0; i<_count; i++)
				_array[i] = v._array[i];
		} else {
			_array = NULL;
		}
		return *this;
	}
	/// retrieves item from specified position
	LVRef<T> operator [] ( int pos ) const { return _array[pos]; }
	/// retrieves item reference from specified position
	LVRef<T> & operator [] ( int pos ) { return _array[pos]; }
	/// ensures that size of vector is not less than specified value
	void reserve( int size )
	{
		if ( size > _size )
		{
			LVRef<T> * newarray = new LVRef<T>[ size ];
			for ( int i=0; i<_size; i++ )
				newarray[ i ] = _array[ i ];
			if ( _array )
				delete [] _array;
			_array = newarray;
			_size = size;
		}
	}
	/// sets item by index (extends vector if necessary)
	void set( int index, LVRef<T> item )
	{
		reserve( index );
		_array[index] = item;
	}
	/// returns size of buffer
	int size() { return _size; }
	/// returns number of items in vector
	int length() { return _count; }
	/// returns true if there are no items in vector
	bool empty() { return _count==0; }
	/// clears all items
	void clear()
	{
		if (_array)
		{
			delete [] _array;
			_array = NULL;
		}
		_size = 0;
		_count = 0;
	}
	/// copies range to beginning of array
	void trim( int pos, int count, int reserved )
	{
		if ( pos<0 || count<=0 || pos+count > _count )
			throw;
		int i;
		int new_sz = count;
		if (new_sz < reserved)
			new_sz = reserved;
		T* new_array = (T*)malloc( new_sz * sizeof( T ) );
		if (_array)
		{
			for ( i=0; i<count; i++ )
			{
				new_array[i] = _array[ pos + i ];
			}
			free( _array );
		}
		_array = new_array;
		_count = count;
		_size = new_sz;
	}
	/// removes several items from vector
	void erase( int pos, int count )
	{
		if ( pos<0 || count<=0 || pos+count > _count )
			throw;
		int i;
		for (i=pos+count; i<_count; i++)
		{
			_array[i-count] = _array[i];
		}
		_count -= count;
	}

	/// adds new item to end of vector
	void add( LVRef<T> item )
	{
		insert( -1, item );
	}

	void add( LVRefVec<T> & list )
	{
		for ( int i=0; i<list.length(); i++ )
			add( list[i] );
	}

	/// adds new item to end of vector
	void append( const LVRef<T> * items, int count )
	{
		reserve( _count + count );
		for (int i=0; i<count; i++)
			_array[ _count+i ] = items[i];
		_count += count;
	}

	LVRef<T> * addSpace( int count )
	{
		reserve( _count + count );
		LVRef<T> * ptr = _array + _count;
		_count += count;
		return ptr;
	}

	/// inserts new item to specified position
	void insert( int pos, LVRef<T> item )
	{
		if (pos<0 || pos>_count)
			pos = _count;
		if ( _count >= _size )
			reserve( _count * 3 / 2  + 8 );
		for (int i=_count; i>pos; --i)
			_array[i] = _array[i-1];
		_array[pos] = item;
		_count++;
	}
	/// returns array pointer
	LVRef<T> * ptr() { return _array; }
	/// destructor
	~LVRefVec() { clear(); }
};

#endif	// __LVREFVEC_H_INCLUDED__
