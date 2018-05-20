/** \file lvfontman.h
	\brief font manager interface

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006

	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.

*/

#ifndef __LV_FONT_MAN_H_INCLUDED__
#define __LV_FONT_MAN_H_INCLUDED__

#include "crsetup.h"
#include "lvstring.h"
#include "lvstream.h"
#include "lvstring16collection.h"
#include "lvfont.h"


enum font_antialiasing_t
{
	font_aa_none,
	font_aa_big,
	font_aa_all
};

/// font manager interface class
class LVFontManager
{
private:
	static LVFontManager* _instance;
	static double _gammaLevel;
	static int _gammaIndex;
protected:
	font_antialiasing_t _antialiasMode;
	bool _allowKerning;
	hinting_mode_t _hintingMode;
public:
	/// garbage collector frees unused fonts
	virtual void gc() = 0;
	/// returns most similar font
	virtual LVFontRef GetFont(int size, int weight, bool italic, css_font_family_t family, lString8 typeface, int documentId = -1) = 0;
	/// set fallback font face (returns true if specified font is found)
	virtual bool SetFallbackFontFace( lString8 face ) { CR_UNUSED(face); return false; }
	/// get fallback font face (returns empty string if no fallback font is set)
	virtual lString8 GetFallbackFontFace() { return lString8::empty_str; }
	/// returns fallback font for specified size
	virtual LVFontRef GetFallbackFont(int /*size*/) { return LVFontRef(); }
	/// registers font by name
	virtual bool RegisterFont( lString8 name ) = 0;
	/// registers font by name and face
	virtual bool RegisterExternalFont(lString16 /*name*/, lString8 /*face*/, bool /*bold*/, bool /*italic*/) { return false; }
	/// registers document font
	virtual bool RegisterDocumentFont(int /*documentId*/, LVContainerRef /*container*/, lString16 /*name*/, lString8 /*face*/, bool /*bold*/, bool /*italic*/) { return false; }
	/// unregisters all document fonts
	virtual void UnregisterDocumentFonts(int /*documentId*/) { }
	/// initializes font manager
	virtual bool Init( lString8 path ) = 0;
	/// get count of registered fonts
	virtual int GetFontCount() const = 0;
	/// get hash of installed fonts and fallback font
	virtual lUInt32 GetFontListHash(int /*documentId*/) { return 0; }
	/// clear glyph cache
	virtual void clearGlyphCache() { }

	/// get antialiasing mode
	virtual font_antialiasing_t GetAntialiasMode() { return _antialiasMode; }
	/// set antialiasing mode
	virtual void SetAntialiasMode( font_antialiasing_t mode ) { _antialiasMode = mode; gc(); clearGlyphCache(); }

	/// get kerning mode: true==ON, false=OFF
	virtual bool getKerning() { return _allowKerning; }
	/// get kerning mode: true==ON, false=OFF
	virtual void setKerning( bool kerningEnabled ) { _allowKerning = kerningEnabled; gc(); clearGlyphCache(); }

	/// constructor
	LVFontManager() : _antialiasMode(font_aa_all), _allowKerning(false), _hintingMode(HINTING_MODE_AUTOHINT) { }
	/// destructor
	virtual ~LVFontManager() { }
	/// returns available typefaces
	virtual void getFaceList( lString16Collection & ) { }

	/// returns first found face from passed list, or return face for font found by family only
	virtual lString8 findFontFace(lString8 commaSeparatedFaceList, css_font_family_t fallbackByFamily);

	/// fills array with list of available gamma levels
	virtual void GetGammaLevels(LVArray<double>& dst);
	/// returns current gamma level index
	virtual int  GetGammaIndex();
	/// sets current gamma level index
	virtual void SetGammaIndex( int gammaIndex );
	/// returns current gamma level
	virtual double GetGamma();
	/// sets current gamma level
	virtual void SetGamma( double gamma );

	/// sets current hinting mode
	virtual void SetHintingMode(hinting_mode_t /*mode*/) { }
	/// returns current hinting mode
	virtual hinting_mode_t  GetHintingMode() { return HINTING_MODE_AUTOHINT; }
	virtual bool setalias(lString8 alias,lString8 facename,int id,bool italic,bool bold){
		CR_UNUSED5(alias, facename, id, italic, bold);
		return false;
	}

	static LVFontManager* getInstance();
	static bool InitInstance( lString8 path );
	static bool FreeInstance();
};

#if USE_FREETYPE==1
//#define LVFONT_TRANSFORM_EMBOLDEN 1
/// create transform for font
//LVFontRef LVCreateFontTransform( LVFontRef baseFont, int transformFlags );
#endif

#if USE_BITMAP_FONTS==1
LVFontRef LoadFontFromFile( const char * fname );
#endif

#endif // __LV_FONT_MAN_H_INCLUDED__
