/** \file lvbitmapfontman.h
	\brief bitmap font manager interface

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006

	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#ifndef __LV_BITMAPFONT_MAN_H_INCLUDED__
#define __LV_BITMAPFONT_MAN_H_INCLUDED__

#include "crsetup.h"

#if USE_BITMAP_FONTS==1

#include "lvfontman.h"
#include "lvfontcache.h"

class LVBitmapFontManager : public LVFontManager
{
private:
	lString8    _path;
	LVFontCache _cache;
	//FILE * _log;
public:
	virtual int GetFontCount() const
	{
		return _cache.length();
	}
	virtual ~LVBitmapFontManager()
	{
		//if (_log)
		//    fclose(_log);
	}
	LVBitmapFontManager()
	{
		//_log = fopen( "fonts.log", "wt" );
	}
	virtual void gc() // garbage collector
	{
		_cache.gc();
	}
	lString8 makeFontFileName( lString8 name );
	virtual LVFontRef GetFont(int size, int weight, bool italic, css_font_family_t family, lString8 typeface, int documentId);
	virtual bool RegisterFont( lString8 name );
	virtual bool Init( lString8 path );
};

#endif	// USE_BITMAP_FONTS==1

#endif	// __LV_BITMAPFONT_MAN_H_INCLUDED__
