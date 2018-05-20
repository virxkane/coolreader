/** \file lvfontglyphcache.cpp
	\brief font's glyph cache

	CoolReader Engine


	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#include "../include/lvfontglyphcache.h"
#include "../include/lvfontman.h"
#include "../include/crlocks.h"
#include "../include/gammatbl.h"

#if (USE_FREETYPE==1)

inline int myabs(int n) { return n < 0 ? -n : n; }

LVFontGlyphCacheItem* LVFontGlyphCacheItem::newItem( LVFontLocalGlyphCache * local_cache, lChar16 ch, int w, int h )
{
	LVFontGlyphCacheItem * item = (LVFontGlyphCacheItem *)malloc( sizeof(LVFontGlyphCacheItem)
		+ (w*h - 1)*sizeof(lUInt8) );
	item->ch = ch;
	item->bmp_width = (lUInt8)w;
	item->bmp_height = (lUInt8)h;
	item->origin_x =   0;
	item->origin_y =   0;
	item->advance =    0;
	item->prev_global = NULL;
	item->next_global = NULL;
	item->prev_local = NULL;
	item->next_local = NULL;
	item->local_cache = local_cache;
	return item;
}

LVFontGlyphCacheItem* LVFontGlyphCacheItem::newItem( LVFontLocalGlyphCache * local_cache, lChar16 ch, FT_GlyphSlot slot ) // , bool drawMonochrome
{
	FONT_LOCAL_GLYPH_CACHE_GUARD
	FT_Bitmap*  bitmap = &slot->bitmap;
	lUInt8 w = (lUInt8)(bitmap->width);
	lUInt8 h = (lUInt8)(bitmap->rows);
	LVFontGlyphCacheItem * item = LVFontGlyphCacheItem::newItem(local_cache, ch, w, h );
	if ( bitmap->pixel_mode==FT_PIXEL_MODE_MONO ) { //drawMonochrome
		lUInt8 mask = 0x80;
		const lUInt8 * ptr = (const lUInt8 *)bitmap->buffer;
		lUInt8 * dst = item->bmp;
		//int rowsize = ((w + 15) / 16) * 2;
		for ( int y=0; y<h; y++ ) {
			const lUInt8 * row = ptr;
			mask = 0x80;
			for ( int x=0; x<w; x++ ) {
				*dst++ = (*row & mask) ? 0xFF : 00;
				mask >>= 1;
				if ( !mask && x!=w-1) {
					mask = 0x80;
					row++;
				}
			}
			ptr += bitmap->pitch;//rowsize;
		}
	} else {
#if 0
		if ( bitmap->pixel_mode==FT_PIXEL_MODE_MONO ) {
			memset( item->bmp, 0, w*h );
			lUInt8 * srcrow = bitmap->buffer;
			lUInt8 * dstrow = item->bmp;
			for ( int y=0; y<h; y++ ) {
				lUInt8 * src = srcrow;
				for ( int x=0; x<w; x++ ) {
					dstrow[x] =  ( (*src)&(0x80>>(x&7)) ) ? 255 : 0;
					if ((x&7)==7)
						src++;
				}
				srcrow += bitmap->pitch;
				dstrow += w;
			}
		} else {
#endif
			memcpy( item->bmp, bitmap->buffer, w*h );
			// correct gamma
			if ( LVFontManager::getInstance()->GetGammaIndex()!=GAMMA_LEVELS/2 )
				cr_correct_gamma_buf(item->bmp, w*h, LVFontManager::getInstance()->GetGammaIndex());
//            }
	}
	item->origin_x =   (lInt8)slot->bitmap_left;
	item->origin_y =   (lInt8)slot->bitmap_top;
	item->advance =    (lUInt8)(myabs(slot->metrics.horiAdvance) >> 6);
	return item;
}

void LVFontGlyphCacheItem::freeItem( LVFontGlyphCacheItem * item )
{
	free( item );
}

void LVFontLocalGlyphCache::clear()
{
	FONT_LOCAL_GLYPH_CACHE_GUARD
	while ( head ) {
		LVFontGlyphCacheItem * ptr = head;
		remove( ptr );
		global_cache->remove( ptr );
		LVFontGlyphCacheItem::freeItem( ptr );
	}
}

LVFontGlyphCacheItem * LVFontLocalGlyphCache::get( lUInt16 ch )
{
	FONT_LOCAL_GLYPH_CACHE_GUARD
	LVFontGlyphCacheItem * ptr = head;
	for ( ; ptr; ptr = ptr->next_local ) {
		if ( ptr->ch == ch ) {
			global_cache->refresh( ptr );
			return ptr;
		}
	}
	return NULL;
}

void LVFontLocalGlyphCache::put( LVFontGlyphCacheItem * item )
{
	FONT_LOCAL_GLYPH_CACHE_GUARD
	global_cache->put( item );
	item->next_local = head;
	if ( head )
		head->prev_local = item;
	if ( !tail )
		tail = item;
	head = item;
}

/// remove from list, but don't delete
void LVFontLocalGlyphCache::remove( LVFontGlyphCacheItem * item )
{
	FONT_LOCAL_GLYPH_CACHE_GUARD
	if ( item==head )
		head = item->next_local;
	if ( item==tail )
		tail = item->prev_local;
	if ( !head || !tail )
		return;
	if ( item->prev_local )
		item->prev_local->next_local = item->next_local;
	if ( item->next_local )
		item->next_local->prev_local = item->prev_local;
	item->next_local = NULL;
	item->prev_local = NULL;
}

void LVFontGlobalGlyphCache::refresh( LVFontGlyphCacheItem * item )
{
	FONT_GLYPH_CACHE_GUARD
	if ( tail!=item ) {
		//move to head
		removeNoLock( item );
		putNoLock( item );
	}
}

void LVFontGlobalGlyphCache::put( LVFontGlyphCacheItem * item )
{
	FONT_GLYPH_CACHE_GUARD
	putNoLock(item);
}

void LVFontGlobalGlyphCache::putNoLock( LVFontGlyphCacheItem * item )
{
	int sz = item->getSize();
	// remove extra items from tail
	while ( sz + size > max_size ) {
		LVFontGlyphCacheItem * removed_item = tail;
		if ( !removed_item )
			break;
		removeNoLock( removed_item );
		removed_item->local_cache->remove( removed_item );
		LVFontGlyphCacheItem::freeItem( removed_item );
	}
	// add new item to head
	item->next_global = head;
	if ( head )
		head->prev_global = item;
	head = item;
	if ( !tail )
		tail = item;
	size += sz;
}

void LVFontGlobalGlyphCache::remove( LVFontGlyphCacheItem * item )
{
	FONT_GLYPH_CACHE_GUARD
	removeNoLock(item);
}

void LVFontGlobalGlyphCache::removeNoLock( LVFontGlyphCacheItem * item )
{
	if ( item==head )
		head = item->next_global;
	if ( item==tail )
		tail = item->prev_global;
	if ( !head || !tail )
		return;
	if ( item->prev_global )
		item->prev_global->next_global = item->next_global;
	if ( item->next_global )
		item->next_global->prev_global = item->prev_global;
	item->next_global = NULL;
	item->prev_global = NULL;
	size -= item->getSize();
}

void LVFontGlobalGlyphCache::clear()
{
	FONT_GLYPH_CACHE_GUARD
	while ( head ) {
		LVFontGlyphCacheItem * ptr = head;
		remove( ptr );
		ptr->local_cache->remove( ptr );
		LVFontGlyphCacheItem::freeItem( ptr );
	}
}


// class LVFontGlyphWidthCache
lUInt8 LVFontGlyphWidthCache::get(lChar16 ch)
{
	FONT_GLYPH_CACHE_GUARD
			int inx = (ch>>9) & 0x7f;
	lUInt8 * ptr = ptrs[inx];
	if ( !ptr )
		return 0xFF;
	return ptr[ch & 0x1FF ];
}

void LVFontGlyphWidthCache::put(lChar16 ch, lUInt8 w)
{
	FONT_GLYPH_CACHE_GUARD
			int inx = (ch>>9) & 0x7f;
	lUInt8 * ptr = ptrs[inx];
	if ( !ptr ) {
		ptr = new lUInt8[512];
		ptrs[inx] = ptr;
		memset( ptr, 0xFF, sizeof(lUInt8) * 512 );
	}
	ptr[ ch & 0x1FF ] = w;
}

void LVFontGlyphWidthCache::clear()
{
	FONT_GLYPH_CACHE_GUARD
			for ( int i=0; i<128; i++ ) {
		if ( ptrs[i] )
			delete [] ptrs[i];
		ptrs[i] = NULL;
	}
}

#endif	// (USE_FREETYPE==1)
