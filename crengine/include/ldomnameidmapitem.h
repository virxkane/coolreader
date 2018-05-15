/** \file ldomnameidmapitem.h
	\brief Name <-> Id map

   CoolReader Engine DOM Tree

   Implements mapping between Name and Id

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LDOMNAME_ID_MAP_ITEM_H__INCLUDED__
#define __LDOMNAME_ID_MAP_ITEM_H__INCLUDED__

#include "lvstring.h"
#include "serialbuf.h"
#include <stdio.h>

// from dtddef.h
struct css_elem_def_props_t;

class LDOMNameIdMapItem
{
	/// custom data pointer
	css_elem_def_props_t * data;
public:
	/// id
	lUInt16    id;
	/// value
	lString16 value;
	/// constructor
	LDOMNameIdMapItem(lUInt16 _id, const lString16 & _value, const css_elem_def_props_t * _data);
	/// copy constructor
	LDOMNameIdMapItem(LDOMNameIdMapItem & item);
	/// destructor
	~LDOMNameIdMapItem();

	const css_elem_def_props_t * getData() const { return data; }

	/// serialize to byte array (pointer will be incremented by number of bytes written)
	void serialize( SerialBuf & buf );
	/// deserialize from byte array (pointer will be incremented by number of bytes read)
	static LDOMNameIdMapItem * deserialize( SerialBuf & buf );
};

#endif	// __LDOMNAME_ID_MAP_ITEM_H__INCLUDED__
