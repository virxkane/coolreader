/** \file lvfontglyphcache.h
	\brief font's glyph cache

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006

	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#ifndef __LV_FONTSGLYPH_CACHE_H_INCLUDED__
#define __LV_FONTSGLYPH_CACHE_H_INCLUDED__

#include "crsetup.h"
#include "lvtypes.h"

#if USE_FREETYPE==1
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

struct LVFontGlyphCacheItem;

class LVFontGlobalGlyphCache
{
private:
	LVFontGlyphCacheItem * head;
	LVFontGlyphCacheItem * tail;
	int size;
	int max_size;
	void removeNoLock( LVFontGlyphCacheItem * item );
	void putNoLock( LVFontGlyphCacheItem * item );
public:
	LVFontGlobalGlyphCache( int maxSize )
		: head(NULL), tail(NULL), size(0), max_size(maxSize )
	{
	}
	~LVFontGlobalGlyphCache()
	{
		clear();
	}
	void put( LVFontGlyphCacheItem * item );
	void remove( LVFontGlyphCacheItem * item );
	void refresh( LVFontGlyphCacheItem * item );
	void clear();
};

class LVFontLocalGlyphCache
{
private:
	LVFontGlyphCacheItem * head;
	LVFontGlyphCacheItem * tail;
	LVFontGlobalGlyphCache * global_cache;
	int size;
public:
	LVFontLocalGlyphCache( LVFontGlobalGlyphCache * globalCache )
		: head(NULL), tail(NULL), global_cache( globalCache )
	{ }
	~LVFontLocalGlyphCache()
	{
		clear();
	}
	void clear();
	LVFontGlyphCacheItem * get( lUInt16 ch );
	void put( LVFontGlyphCacheItem * item );
	void remove( LVFontGlyphCacheItem * item );
};

struct LVFontGlyphCacheItem
{
	LVFontGlyphCacheItem * prev_global;
	LVFontGlyphCacheItem * next_global;
	LVFontGlyphCacheItem * prev_local;
	LVFontGlyphCacheItem * next_local;
	LVFontLocalGlyphCache * local_cache;
	lChar16 ch;
	lUInt8 bmp_width;
	lUInt8 bmp_height;
	lInt8  origin_x;
	lInt8  origin_y;
	lUInt8 advance;
	lUInt8 bmp[1];
	//=======================================================================
	int getSize() const
	{
		return sizeof(LVFontGlyphCacheItem)
			+ (bmp_width * bmp_height - 1) * sizeof(lUInt8);
	}
	static LVFontGlyphCacheItem * newItem( LVFontLocalGlyphCache * local_cache, lChar16 ch, int w, int h );
#if USE_FREETYPE==1
	static LVFontGlyphCacheItem * newItem( LVFontLocalGlyphCache * local_cache, lChar16 ch, FT_GlyphSlot slot );
#endif
	static void freeItem( LVFontGlyphCacheItem * item );
};

#if USE_FREETYPE==1
class LVFontGlyphWidthCache
{
private:
	lUInt8 * ptrs[128];
public:
	lUInt8 get( lChar16 ch );
	void put( lChar16 ch, lUInt8 w );
	void clear();
	LVFontGlyphWidthCache()
	{
		memset( ptrs, 0, 128*sizeof(lUInt8*) );
	}
	~LVFontGlyphWidthCache()
	{
		clear();
	}
};
#endif	// USE_FREETYPE==1

#endif	// __LV_FONTSGLYPH_CACHE_H_INCLUDED__
