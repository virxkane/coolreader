/** \file compactarray.h

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#ifndef __LV_COMPACT_ARRAY_H_INCLUDED__
#define __LV_COMPACT_ARRAY_H_INCLUDED__

#include "lvmemman.h"
#include "lvarray.h"

template <typename T, int RESIZE_MULT, int RESIZE_ADD> class LVCompactArray
{
private:
	class Array {
	public:
		T * _list;
		int _size;
		int _length;
		Array()
		: _list(NULL), _size(0), _length(0)
		{
		}
		~Array()
		{
			clear();
		}
		void add( T item )
		{
			if ( _size<=_length ) {
				_size = _size*RESIZE_MULT + RESIZE_ADD;
				_list = cr_realloc( _list, _size );
			}
			_list[_length++] = item;
		}
		void add( T * items, int count )
		{
			if ( count<=0 )
				return;
			if ( _size<_length+count ) {
				_size = _length+count;
				_list = cr_realloc( _list, _size );
			}
			for ( int i=0; i<count; i++ )
				_list[_length+i] = items[i];
			_length += count;
		}
		void reserve( int count )
		{
			if ( count<=0 )
				return;
			if ( _size<_length+count ) {
				_size = _length+count;
				_list = cr_realloc( _list, _size );
			}
		}
		void clear()
		{
			if ( _list ) {
				free( _list );
				_list = NULL;
				_size = 0;
				_length = 0;
			}
		}
		int length() const
		{
			return _length;
		}
		T get( int index ) const
		{
			return _list[index];
		}
		const T & operator [] (int index) const
		{
			return _list[index];
		}
		T & operator [] (int index)
		{
			return _list[index];
		}
	};
	Array * _data;
public:
	LVCompactArray()
	: _data(NULL)
	{
	}
	~LVCompactArray()
	{
		if ( _data )
			delete _data;
	}
	void add( T item )
	{
		if ( !_data )
			_data = new Array();
		_data->add(item);
	}
	void add( T * items, int count )
	{
		if ( !_data )
			_data = new Array();
		_data->add(items, count);
	}
	void add( LVArray<T> & items )
	{
		if ( items.length()<=0 )
			return;
		if ( !_data )
			_data = new Array();
		_data->add( &(items[0]), items.length() );
	}
	void reserve( int count )
	{
		if ( count<=0 )
			return;
		if ( !_data )
			_data = new Array();
		_data->reserve( count );
	}
	void clear()
	{
		if ( _data ) {
			delete _data;
			_data = NULL;
		}
	}
	int length() const
	{
		return _data ? _data->length() : 0;
	}
	T get( int index ) const
	{
		if (!_data)
			crFatalError(CR_FATAL_ERROR_UNKNOWN, "LVCompactArray: null pointer dereference!");
		if (index < 0 || index >= _data->length())
			crFatalError(CR_FATAL_ERROR_INDEX_OUT_OF_BOUND, "LVCompactArray: index out of range!");
		return _data->get(index);
	}
	const T & operator [] (int index) const
	{
		if (!_data)
			crFatalError(CR_FATAL_ERROR_UNKNOWN, "LVCompactArray: null pointer dereference!");
		if (index < 0 || index >= _data->length())
			crFatalError(CR_FATAL_ERROR_INDEX_OUT_OF_BOUND, "LVCompactArray: index out of range!");
		return _data->operator [](index);
	}
	T & operator [] (int index)
	{
		if (!_data)
			crFatalError(CR_FATAL_ERROR_UNKNOWN, "LVCompactArray: null pointer dereference!");
		if (index < 0 || index >= _data->length())
			crFatalError(CR_FATAL_ERROR_INDEX_OUT_OF_BOUND, "LVCompactArray: index out of range!");
		return _data->operator [](index);
	}
	bool empty() { return !_data || _data->length()==0; }

};

#endif	// __LV_COMPACT_ARRAY_H_INCLUDED__
