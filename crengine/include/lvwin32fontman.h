/** \file lvwin32fontman.h
	\brief Windows fonts manager interface

	Note: unused even in Windows builds (see crsetup.h, CMakeLists.txt)

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006

	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#ifndef __LV_WIN32FONTMAN_H_INCLUDED__
#define __LV_WIN32FONTMAN_H_INCLUDED__

#include "lvfontman.h"
#include "lvfontcache.h"
#include "crsetup.h"		// USE_WIN32_FONTS

#if !defined(__SYMBIAN32__) && defined(_WIN32) && USE_WIN32_FONTS==1

extern "C" {
#include <windows.h>
}

class LVWin32FontManager : public LVFontManager
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
	virtual ~LVWin32FontManager()
	{
		//if (_log)
		//    fclose(_log);
	}
	LVWin32FontManager()
	{
		//_log = fopen( "fonts.log", "wt" );
	}
	virtual void gc() // garbage collector
	{
		_cache.gc();
	}
	virtual LVFontRef GetFont(int size, int weight, bool bitalic, css_font_family_t family, lString8 typeface, int documentId = -1 );

	virtual bool RegisterFont( const LOGFONTA * lf );
	virtual bool RegisterFont( lString8 name )
	{
		return false;
	}
	virtual bool Init( lString8 path );

	virtual void getFaceList( lString16Collection & list )
	{
		_cache.getFaceList(list);
	}
};

#endif	// !defined(__SYMBIAN32__) && defined(_WIN32) && USE_WIN32_FONTS==1

#endif	// __LV_WIN32FONTMAN_H_INCLUDED__
