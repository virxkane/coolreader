/** \file ldomnameidmap.h
	\brief Name <-> Id map

   CoolReader Engine DOM Tree

   Implements mapping between Name and Id

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LDOMNAME_ID_MAP_H__INCLUDED__
#define __LDOMNAME_ID_MAP_H__INCLUDED__

#include "lvstring.h"
#include "ldomnameidmapitem.h"
#include <stdio.h>

// from dtddef.h
struct css_elem_def_props_t;


class LDOMNameIdMap
{
private:
	LDOMNameIdMapItem * * m_by_id;
	LDOMNameIdMapItem * * m_by_name;
	lUInt16 m_count; // non-empty count
	lUInt16 m_size;  // max number of ids
	bool    m_sorted;
	bool    m_changed;

	void    Sort();
public:
	/// Main constructor
	LDOMNameIdMap( lUInt16 maxId );
	/// Copy constructor
	LDOMNameIdMap( LDOMNameIdMap & map );
	~LDOMNameIdMap();

	/// serialize to byte array (pointer will be incremented by number of bytes written)
	void serialize( SerialBuf & buf );
	/// deserialize from byte array (pointer will be incremented by number of bytes read)
	bool deserialize( SerialBuf & buf );

	void Clear();

	void AddItem( lUInt16 id, const lString16 & value, const css_elem_def_props_t * data );

	bool AddItem( LDOMNameIdMapItem *item );

	const LDOMNameIdMapItem * findItem( lUInt16 id ) const
	{
		return m_by_id[id];
	}

	const LDOMNameIdMapItem * findItem( const lChar16 * name );
	const LDOMNameIdMapItem * findItem( const lChar8 * name );
	const LDOMNameIdMapItem * findItem( const lString16 & name ) { return findItem(name.c_str()); }

	inline lUInt16 idByName( const lChar16 * name )
	{
		const LDOMNameIdMapItem * item = findItem(name);
		return item?item->id:0;
	}

	inline lUInt16 idByName( const lChar8 * name )
	{
		const LDOMNameIdMapItem * item = findItem(name);
		return item?item->id:0;
	}

	inline const lString16 & nameById( lUInt16 id )
	{
		if (id>=m_size)
			return lString16::empty_str;
		const LDOMNameIdMapItem * item = findItem(id);
		return item?item->value:lString16::empty_str;
	}

	inline const css_elem_def_props_t * dataById( lUInt16 id )
	{
		if (id>=m_size)
			return NULL;
		const LDOMNameIdMapItem * item = findItem(id);
		return item ? item->getData() : NULL;
	}

	// debug dump of all unknown entities
	void dumpUnknownItems( FILE * f, int start_id );
};

#endif	// __LDOMNAME_ID_MAP_H__INCLUDED__
