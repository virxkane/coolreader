/** \file lvarray.h
	\brief value array template

	Implements array of values.

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.

	This class is unused, saved for history!
*/

#ifndef __LVARRAYQUEUE_H_INCLUDED__
#define __LVARRAYQUEUE_H_INCLUDED__

#include <stdlib.h>
#include <assert.h>
#include "lvarray.h"
#include "lvmemman.h"

template <typename T >
class LVArrayQueue
{
private:
	LVArray<T> m_buf;
	int inpos;
public:
	LVArrayQueue()
		: inpos(0)
	{
	}

	/// returns pointer to reserved space of specified size
	T * prepareWrite( int size )
	{
		if ( m_buf.length() + size > m_buf.size() )
		{
			if ( inpos > (m_buf.length() + size) / 2 )
			{
				// trim
				m_buf.erase(0, inpos);
				inpos = 0;
			}
		}
		return m_buf.addSpace( size );
	}

	/// writes data to end of queue
	void write( const T * data, int size )
	{
		T * buf = prepareWrite( size );
		for (int i=0; i<size; i++)
			buf[i] = data[i];
	}

	int length()
	{
		return m_buf.length() - inpos;
	}

	/// returns pointer to data to be read
	T * peek() { return m_buf.ptr() + inpos; }

	/// reads data from start of queue
	void read( T * data, int size )
	{
		if ( size > length() )
			size = length();
		for ( int i=0; i<size; i++ )
			data[i] = m_buf[inpos + i];
		inpos += size;
	}

	/// skips data from start of queue
	void skip( int size )
	{
		if ( size > length() )
			size = length();
		inpos += size;
	}
};

#endif	// __LVARRAYQUEUE_H_INCLUDED__
