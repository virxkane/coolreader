/*******************************************************

   CoolReader Engine DOM Tree 

   LDOMNodeIdMap.cpp:  Name to Id map

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/ldomnameidmap.h"

#include <string.h>

static const char id_map_magic[] = "IMAP";

/// serialize to byte array (pointer will be incremented by number of bytes written)
void LDOMNameIdMap::serialize( SerialBuf & buf )
{
    if ( buf.error() )
        return;
    if (!m_sorted)
        Sort();
    int start = buf.pos();
	buf.putMagic( id_map_magic );
    buf << m_count;
    for ( int i=0; i<m_size; i++ ) {
        if ( m_by_id[i] )
            m_by_id[i]->serialize( buf );
    }
    buf.putCRC( buf.pos() - start );
    m_changed = false;
}

/// deserialize from byte array (pointer will be incremented by number of bytes read)
bool LDOMNameIdMap::deserialize( SerialBuf & buf )
{
    if ( buf.error() )
        return false;
    int start = buf.pos();
    if ( !buf.checkMagic( id_map_magic ) ) {
        buf.seterror();
        return false;
    }
    Clear();
    lUInt16 count;
    buf >> count;
    if ( count>m_size ) {
        buf.seterror();
        return false;
    }
    for ( int i=0; i<count; i++ ) {
        LDOMNameIdMapItem * item = LDOMNameIdMapItem::deserialize(buf);
        if ( !item || (item->id<m_size && m_by_id[item->id]!=NULL ) ) { // invalid entry
            if ( item )
                delete item;
            buf.seterror();
            return false;
        }
        if (!AddItem( item ))
            delete item;
    }
    m_sorted = false;
    buf.checkCRC( buf.pos() - start );
    m_changed = false;
    if (!m_sorted)
        Sort();
    return !buf.error();
}


LDOMNameIdMap::LDOMNameIdMap(lUInt16 maxId)
{
    m_size = maxId+1;
    m_count = 0;
    m_by_id   = new LDOMNameIdMapItem * [m_size];
    memset( m_by_id, 0, sizeof(LDOMNameIdMapItem *)*m_size );  
    m_by_name = new LDOMNameIdMapItem * [m_size];
    memset( m_by_name, 0, sizeof(LDOMNameIdMapItem *)*m_size );  
    m_sorted = true;
    m_changed = false;
}

/// Copy constructor
LDOMNameIdMap::LDOMNameIdMap( LDOMNameIdMap & map )
{
    m_changed = false;
    m_size = map.m_size;
    m_count = map.m_count;
    m_by_id   = new LDOMNameIdMapItem * [m_size];
    int i;
    for ( i=0; i<m_size; i++ ) {
        if ( map.m_by_id[i] )
            m_by_id[i] = new LDOMNameIdMapItem( *map.m_by_id[i] );
        else
            m_by_id[i] = NULL;
    }
    m_by_name = new LDOMNameIdMapItem * [m_size];
    for ( i=0; i<m_size; i++ ) {
        if ( map.m_by_name[i] )
            m_by_name[i] = new LDOMNameIdMapItem( *map.m_by_name[i] );
        else
            m_by_name[i] = NULL;
    }
    m_sorted = map.m_sorted;
}

LDOMNameIdMap::~LDOMNameIdMap()
{
    Clear();
    delete[] m_by_name;
    delete[] m_by_id;
}

static int compare_items( const void * item1, const void * item2 )
{
    return (*((LDOMNameIdMapItem **)item1))->value.compare( (*((LDOMNameIdMapItem **)item2))->value );
}

void LDOMNameIdMap::Sort()
{
    if (m_count>1)
        qsort( m_by_name, m_count, sizeof(LDOMNameIdMapItem*), compare_items );
    m_sorted = true;
}

const LDOMNameIdMapItem * LDOMNameIdMap::findItem( const lChar16 * name )
{
    if (m_count==0 || !name || !*name)
        return NULL;
    if (!m_sorted)
        Sort();
    lUInt16 a, b, c;
    int r;
    a = 0;
    b = m_count;
    for (;;) {
        c = (a + b)>>1;
        r = lStr_cmp( name, m_by_name[c]->value.c_str() );
        if (r == 0)
            return m_by_name[c]; // found
        if (b==a+1)
            return NULL; // not found
        if (r>0) {
            a = c;
        } else {
            b = c;
        }
    }
}

const LDOMNameIdMapItem * LDOMNameIdMap::findItem( const lChar8 * name )
{
    if (m_count==0 || !name || !*name)
        return NULL;
    if (!m_sorted)
        Sort();
    lUInt16 a, b, c;
    int r;
    a = 0;
    b = m_count;
    for (;;) {
        c = (a + b)>>1;
        r = lStr_cmp( name, m_by_name[c]->value.c_str() );
        if (r == 0)
            return m_by_name[c]; // found
        if (b==a+1)
            return NULL; // not found
        if (r>0) {
            a = c;
        } else {
            b = c;
        }
    }
}

bool LDOMNameIdMap::AddItem( LDOMNameIdMapItem *item )
{
    if ( item==NULL )
        return false;
    if ( item->id==0 ) {
        return false;
    }
    if (item->id>=m_size)
    {
        // reallocate storage
        lUInt16 newsize = item->id+16;
        // We alloc memory using operator `new' (array of pointers),
        // then using realloc() to increase memory allocation is not good idea.
        lUInt16 i;
        LDOMNameIdMapItem ** new_by_id = new LDOMNameIdMapItem * [newsize];
        LDOMNameIdMapItem ** new_by_name = new LDOMNameIdMapItem * [newsize];
        for (i = 0; i < m_size; i++) {
            new_by_id[i] = m_by_id[i];
            new_by_name[i] = m_by_name[i];
        }
        delete [] m_by_id;
        delete [] m_by_name;
        m_by_id = new_by_id;
        m_by_name = new_by_name;
        for (i = m_size; i<newsize; i++)
        {
            m_by_id[i] = NULL;
            m_by_name[i] = NULL;
        }
        m_size = newsize;
    }
    if (m_by_id[item->id] != NULL)
    {
        return false; // already exists
    }
    m_by_id[item->id] = item;
    m_by_name[m_count++] = item;
    m_sorted = false;
    if (!m_changed) {
        m_changed = true;
        //CRLog::trace("new ID for %s is %d", LCSTR(item->value), item->id);
    }
    return true;
}

void LDOMNameIdMap::AddItem( lUInt16 id, const lString16 & value, const css_elem_def_props_t * data )
{
    if (id==0)
        return;
    LDOMNameIdMapItem * item = new LDOMNameIdMapItem( id, value, data );
    if (!AddItem( item ))
        delete item;
}


void LDOMNameIdMap::Clear()
{
    for (lUInt16 i = 0; i<m_count; i++)
    {
        if (m_by_name[i])
            delete m_by_name[i];
    }
    memset( m_by_id, 0, sizeof(LDOMNameIdMapItem *)*m_size);
    m_count = 0;
}

void LDOMNameIdMap::dumpUnknownItems( FILE * f, int start_id )
{
    for (int i=start_id; i<m_size; i++)
    {
        if (m_by_id[i] != NULL)
        {
            lString8 s8( m_by_id[i]->value.c_str() );
            fprintf( f, "%d %s\n", m_by_id[i]->id, s8.c_str() );
        }
    }
}
