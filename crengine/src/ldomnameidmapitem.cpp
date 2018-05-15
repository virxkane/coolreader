/*******************************************************

   CoolReader Engine DOM Tree 

   ldomnameidmapitem.cpp:  Name to Id map

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/ldomnameidmapitem.h"
#include "../include/dtddef.h"
#include "../include/lvtinydom.h"

#include <string.h>

LDOMNameIdMapItem::LDOMNameIdMapItem(lUInt16 _id, const lString16 & _value, const css_elem_def_props_t * _data)
    : id(_id), value(_value)
{
	if ( _data ) {
        data = new css_elem_def_props_t();
		memcpy(data, _data, sizeof(struct css_elem_def_props_t));
	} else
		data = NULL;
}

LDOMNameIdMapItem::LDOMNameIdMapItem(LDOMNameIdMapItem & item)
    : id(item.id), value(item.value)
{
	if ( item.data ) {
		data = new css_elem_def_props_t();
		memcpy(data, item.data, sizeof(struct css_elem_def_props_t));
	} else {
		data = NULL;
	}
}


static const char id_map_item_magic[] = "IDMI";

/// serialize to byte array
void LDOMNameIdMapItem::serialize( SerialBuf & buf )
{
    if ( buf.error() )
        return;
	buf.putMagic( id_map_item_magic );
	buf << id;
	buf << value;
	if ( data ) {
		buf << (lUInt8)1;
		buf << (lUInt8)data->display;
		buf << (lUInt8)data->white_space;
		buf << data->allow_text;
		buf << data->is_object;
	} else {
		buf << (lUInt8)0;
	}
}

/// deserialize from byte array
LDOMNameIdMapItem * LDOMNameIdMapItem::deserialize( SerialBuf & buf )
{
    if ( buf.error() )
        return NULL;
	if ( !buf.checkMagic( id_map_item_magic ) )
        return NULL;
	lUInt16 id;
	lString16 value;
	lUInt8 flgData;
    buf >> id >> value >> flgData;
    if ( id>=MAX_TYPE_ID )
        return NULL;
    if ( flgData ) {
        css_elem_def_props_t props;
        lUInt8 display;
        lUInt8 white_space;
        buf >> display >> white_space >> props.allow_text >> props.is_object;
        if ( display > css_d_none || white_space > css_ws_nowrap )
            return NULL;
        props.display = (css_display_t)display;
        props.white_space = (css_white_space_t)white_space;
    	return new LDOMNameIdMapItem(id, value, &props);
    }
   	return new LDOMNameIdMapItem(id, value, NULL);
}

LDOMNameIdMapItem::~LDOMNameIdMapItem()
{
	if ( data )
		delete data;
}
