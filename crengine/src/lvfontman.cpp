/** \file lvfontman.cpp
    \brief font manager implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "../include/lvfontman.h"
#include "../include/lvstyles.h"		// for splitPropertyValueList()
#include "../include/crlog.h"

#if USE_WIN32_FONTS==1
#include "../include/lvwin32fontman.h"
#endif
#if USE_FREETYPE==1
#include "../include/lvfreetypefontman.h"
#endif
#include "../include/lvbitmapfontman.h"

#define GAMMA_TABLES_IMPL
#include "../include/gammatbl.h"

#if COLOR_BACKBUFFER==0
//#define USE_BITMAP_FONT
#endif

//DEFINE_NULL_REF( LVFont )

LVFontManager* LVFontManager::_instance = NULL;

double LVFontManager::_gammaLevel = 1.0;
int LVFontManager::_gammaIndex = GAMMA_LEVELS/2;

/// returns first found face from passed list, or return face for font found by family only
lString8 LVFontManager::findFontFace(lString8 commaSeparatedFaceList, css_font_family_t fallbackByFamily) {
	// faces we want
	lString8Collection list;
	splitPropertyValueList(commaSeparatedFaceList.c_str(), list);
	// faces we have
	lString16Collection faces;
	getFaceList(faces);
	// find first matched
	for (int i = 0; i < list.length(); i++) {
		lString8 wantFace = list[i];
		for (int j = 0; j < faces.length(); j++) {
			lString16 haveFace = faces[j];
			if (wantFace == haveFace)
				return wantFace;
		}
	}
	// not matched - get by family name
    LVFontRef fnt = GetFont(10, 400, false, fallbackByFamily, lString8("Arial"));
    if (fnt.isNull())
    	return lString8::empty_str; // not found
    // get face from found font
    return fnt->getTypeFace();
}

/// fills array with list of available gamma levels
void LVFontManager::GetGammaLevels(LVArray<double>& dst) {
    dst.clear();
    for ( int i=0; i<GAMMA_LEVELS; i++ )
        dst.add(cr_gamma_levels[i]);
}

/// returns current gamma level index
int  LVFontManager::GetGammaIndex() {
    return _gammaIndex;
}

/// sets current gamma level index
void LVFontManager::SetGammaIndex(int gammaIndex ) {
    if ( gammaIndex<0 )
        gammaIndex = 0;
    if ( gammaIndex>=GAMMA_LEVELS )
        gammaIndex = GAMMA_LEVELS-1;
    if ( _gammaIndex!=gammaIndex ) {
        CRLog::trace("FontManager gamma index changed from %d to %d", _gammaIndex, gammaIndex);
        _gammaIndex = gammaIndex;
        _gammaLevel = cr_gamma_levels[gammaIndex];
        clearGlyphCache();
    }
}

/// returns current gamma level
double LVFontManager::GetGamma() {
    return _gammaLevel;
}

/// sets current gamma level
void LVFontManager::SetGamma( double gamma ) {
//    gammaLevel = cr_ft_gamma_levels[GAMMA_LEVELS/2];
//    gammaIndex = GAMMA_LEVELS/2;
    int oldGammaIndex = _gammaIndex;
    for ( int i=0; i<GAMMA_LEVELS; i++ ) {
        double diff1 = cr_gamma_levels[i] - gamma;
        if ( diff1<0 ) diff1 = -diff1;
        double diff2 = _gammaLevel - gamma;
        if ( diff2<0 ) diff2 = -diff2;
        if ( diff1 < diff2 ) {
            _gammaLevel = cr_gamma_levels[i];
            _gammaIndex = i;
        }
    }
    if ( _gammaIndex!=oldGammaIndex ) {
        CRLog::trace("FontManager gamma index changed from %d to %d", oldGammaIndex, _gammaIndex);
        clearGlyphCache();
	}
}

LVFontManager *LVFontManager::getInstance()
{
    if ( LVFontManager::_instance == NULL )
        crFatalError(-4, "LVFontManager::_instance is NULL");
    return LVFontManager::_instance;
}

bool LVFontManager::InitInstance(lString8 path)
{
    if ( LVFontManager::_instance != NULL ) {
    	return true;
        //delete LVFontManager::_instance;
    }
#if (USE_WIN32_FONTS==1)
    LVFontManager::_instance = new LVWin32FontManager;
#elif (USE_FREETYPE==1)
    LVFontManager::_instance = new LVFreeTypeFontManager;
#else
    LVFontManager::_instance = new LVBitmapFontManager;
#endif
	return LVFontManager::_instance->Init( path );
}

bool LVFontManager::FreeInstance()
{
    if ( LVFontManager::_instance )
    {
        delete LVFontManager::_instance;
        LVFontManager::_instance = NULL;
        return true;
    }
    return false;
}

#if USE_FREETYPE==1
/// create transform for font
//LVFontRef LVCreateFontTransform( LVFontRef baseFont, int transformFlags )
//{
//	if ( transformFlags & LVFONT_TRANSFORM_EMBOLDEN ) {
//		// BOLD transform
//		return LVFontRef( new LVFontBoldTransform( baseFont ) );
//	} else {
//		return baseFont; // no transform
//	}
//}
#endif

#if USE_BITMAP_FONTS==1
LVFontRef LoadFontFromFile( const char * fname )
{
	LVFontRef ref;
	LBitmapFont * font = new LBitmapFont;
	if (font->LoadFromFile( fname ) )
	{
		ref = font;
	}
	else
	{
		delete font;
	}
	return ref;
}
#endif
