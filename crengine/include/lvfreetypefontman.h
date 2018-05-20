/** \file lvfreetypefontman.h
	\brief FreeTYpe font manager interface

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006

	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#ifndef __LV_FREETYPE_FONT_MAN_H_INCLUDED__
#define __LV_FREETYPE_FONT_MAN_H_INCLUDED__

#include "crsetup.h"

#if USE_FREETYPE==1

#include "lvfontman.h"
#include "lvfontcache.h"
#include "lvfontglyphcache.h"
#include "lvthread.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#if (DEBUG_FONT_MAN==1)
#include <stdio.h>
#endif

class LVFreeTypeFontManager : public LVFontManager
{
private:
	lString8    _path;
	lString8    _fallbackFontFace;
	LVFontCache _cache;
	FT_Library  _library;
	LVFontGlobalGlyphCache _globalCache;
	lString16 _requiredChars;
#if (DEBUG_FONT_MAN==1)
	FILE * _log;
#endif
	LVMutex   _lock;
public:

	/// get hash of installed fonts and fallback font
	virtual lUInt32 GetFontListHash(int documentId);
	/// set fallback font
	virtual bool SetFallbackFontFace( lString8 face );
	/// get fallback font face (returns empty string if no fallback font is set)
	virtual lString8 GetFallbackFontFace() { return _fallbackFontFace; }
	/// returns fallback font for specified size
	virtual LVFontRef GetFallbackFont(int size);
	bool isBitmapModeForSize( int size );
	/// set antialiasing mode
	virtual void SetAntialiasMode( font_antialiasing_t mode );
	/// sets current gamma level
	virtual void SetHintingMode(hinting_mode_t mode);
	/// sets current gamma level
	virtual hinting_mode_t  GetHintingMode() {
		return _hintingMode;
	}
	/// set antialiasing mode
	virtual void setKerning( bool kerning );
	/// clear glyph cache
	virtual void clearGlyphCache();
	virtual int GetFontCount() const
	{
		return _cache.length();
	}
	virtual ~LVFreeTypeFontManager();
	LVFreeTypeFontManager();
	virtual void gc();
	/// returns available typefaces
	virtual void getFaceList( lString16Collection & list );
	bool setalias(lString8 alias,lString8 facename,int id,bool italic, bool bold);
	virtual LVFontRef GetFont(int size, int weight, bool italic, css_font_family_t family, lString8 typeface, int documentId);
	//bool isMonoSpaced( FT_Face face );
	/// registers document font
	virtual bool RegisterDocumentFont(int documentId, LVContainerRef container, lString16 name, lString8 faceName, bool bold, bool italic);
	/// unregisters all document fonts
	virtual void UnregisterDocumentFonts(int documentId);
	virtual bool RegisterExternalFont( lString16 name, lString8 family_name, bool bold, bool italic);
	virtual bool RegisterFont( lString8 name );
	virtual bool Init( lString8 path );
protected:
	lString8 makeFontFileName( lString8 name );
	bool checkCharSet( FT_Face face );
	bool initSystemFonts();
};

#endif	// USE_FREETYPE==1

#endif	// __LV_FREETYPE_FONT_MAN_H_INCLUDED__
