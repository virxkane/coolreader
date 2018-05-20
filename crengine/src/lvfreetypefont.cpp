/** \file lvfreetypefont.cpp
    \brief FreeType font implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "../include/lvfreetypefont.h"
#include "../include/lvfontman.h"
#include "../include/lvdrawbuf.h"
#include "../include/lvfnt.h"
#include "../include/lvtextfm.h"
#include "../include/crlog.h"

#if USE_FREETYPE==1

inline int myabs(int n) { return n < 0 ? -n : n; }

static lChar16 getReplacementChar( lUInt16 code ) {
    switch (code) {
    case UNICODE_SOFT_HYPHEN_CODE:
        return '-';
    case 0x0401: // CYRILLIC CAPITAL LETTER IO
        return 0x0415; //CYRILLIC CAPITAL LETTER IE
    case 0x0451: // CYRILLIC SMALL LETTER IO
        return 0x0435; // CYRILLIC SMALL LETTER IE
    case UNICODE_NO_BREAK_SPACE:
        return ' ';
    case 0x2010:
    case 0x2011:
    case 0x2012:
    case 0x2013:
    case 0x2014:
    case 0x2015:
        return '-';
    case 0x2018:
    case 0x2019:
    case 0x201a:
    case 0x201b:
        return '\'';
    case 0x201c:
    case 0x201d:
    case 0x201e:
    case 0x201f:
    case 0x00ab:
    case 0x00bb:
        return '\"';
    case 0x2039:
        return '<';
    case 0x203A:
        return '>';
    case 0x2044:
        return '/';
    case 0x2022: // css_lst_disc:
        return '*';
    case 0x26AA: // css_lst_disc:
    case 0x25E6: // css_lst_disc:
    case 0x25CF: // css_lst_disc:
        return 'o';
    case 0x25CB: // css_lst_circle:
        return '*';
    case 0x25A0: // css_lst_square:
        return '-';
    }
    return 0;
}

lString8 LVFreeTypeFace::familyName( FT_Face face )
{
    lString8 faceName( face->family_name );
    if ( faceName == "Arial" && face->style_name && !strcmp(face->style_name, "Narrow") )
        faceName << " " << face->style_name;
    else if ( /*faceName == "Arial" &&*/ face->style_name && strstr(face->style_name, "Condensed") )
        faceName << " " << "Condensed";
    return faceName;
}

static lUInt16 char_flags[] = {
    0, 0, 0, 0, 0, 0, 0, 0, // 0    00
    0, 0, LCHAR_IS_SPACE | LCHAR_IS_EOL | LCHAR_ALLOW_WRAP_AFTER, 0, 0, LCHAR_IS_SPACE | LCHAR_IS_EOL | LCHAR_ALLOW_WRAP_AFTER, 0, 0, // 8    08
    0, 0, 0, 0, 0, 0, 0, 0, // 16   10
    0, 0, 0, 0, 0, 0, 0, 0, // 24   18
    LCHAR_IS_SPACE | LCHAR_ALLOW_WRAP_AFTER, 0, 0, 0, 0, 0, 0, 0, // 32   20
    0, 0, 0, 0, 0, LCHAR_DEPRECATED_WRAP_AFTER, 0, 0, // 40   28
    0, 0, 0, 0, 0, 0, 0, 0, // 48   30
};

#define GET_CHAR_FLAGS(ch) \
     (ch<48?char_flags[ch]: \
        (ch==UNICODE_SOFT_HYPHEN_CODE?LCHAR_ALLOW_WRAP_AFTER: \
        (ch==UNICODE_NO_BREAK_SPACE?LCHAR_DEPRECATED_WRAP_AFTER|LCHAR_IS_SPACE: \
        (ch==UNICODE_HYPHEN?LCHAR_DEPRECATED_WRAP_AFTER:0))))



LVFont *LVFreeTypeFace::getFallbackFont() {
	if ( _fallbackFontIsSet )
		return _fallbackFont.get();
	LVFontManager* fontMan = LVFontManager::getInstance();
	if ( fontMan->GetFallbackFontFace()!=_faceName ) // to avoid circular link, disable fallback for fallback font
		_fallbackFont = fontMan->GetFallbackFont(_size);
	_fallbackFontIsSet = true;
	return _fallbackFont.get();
}

LVFreeTypeFace::LVFreeTypeFace(LVMutex &mutex, FT_Library library, LVFontGlobalGlyphCache *globalCache)
	: _mutex(mutex), _fontFamily(css_ff_sans_serif), _library(library), _face(NULL), _size(0), _hyphen_width(0), _baseline(0)
	, _weight(400), _italic(0)
	, _glyph_cache(globalCache), _drawMonochrome(false), _allowKerning(false), _hintingMode(HINTING_MODE_AUTOHINT), _fallbackFontIsSet(false)
{
	_matrix.xx = 0x10000;
	_matrix.yy = 0x10000;
	_matrix.xy = 0;
	_matrix.yx = 0;
	_hintingMode = LVFontManager::getInstance()->GetHintingMode();
}

LVFreeTypeFace::~LVFreeTypeFace()
{
	Clear();
}

int LVFreeTypeFace::getHyphenWidth()
{
	FONT_GUARD
			if ( !_hyphen_width ) {
		_hyphen_width = getCharWidth( UNICODE_SOFT_HYPHEN_CODE );
	}
	return _hyphen_width;
}

void LVFreeTypeFace::setHintingMode(hinting_mode_t mode) {
	if (_hintingMode == mode)
		return;
	_hintingMode = mode;
	_glyph_cache.clear();
	_wcache.clear();
}

void LVFreeTypeFace::setBitmapMode(bool drawBitmap)
{
	if ( _drawMonochrome == drawBitmap )
		return;
	_drawMonochrome = drawBitmap;
	_glyph_cache.clear();
	_wcache.clear();
}

bool LVFreeTypeFace::loadFromBuffer(LVByteArrayRef buf, int index, int size, css_font_family_t fontFamily, bool monochrome, bool italicize)
{
	FONT_GUARD
			_hintingMode = LVFontManager::getInstance()->GetHintingMode();
	_drawMonochrome = monochrome;
	_fontFamily = fontFamily;
	int error = FT_New_Memory_Face( _library, buf->get(), buf->length(), index, &_face ); /* create face object */
	if (error)
		return false;
	if ( _fileName.endsWith(".pfb") || _fileName.endsWith(".pfa") ) {
		lString8 kernFile = _fileName.substr(0, _fileName.length()-4);
		if ( LVFileExists(Utf8ToUnicode(kernFile) + ".afm" ) ) {
			kernFile += ".afm";
		} else if ( LVFileExists(Utf8ToUnicode(kernFile) + ".pfm" ) ) {
			kernFile += ".pfm";
		} else {
			kernFile.clear();
		}
		if ( !kernFile.empty() )
			error = FT_Attach_File( _face, kernFile.c_str() );
	}
	//FT_Face_SetUnpatentedHinting( _face, 1 );
	_slot = _face->glyph;
	_faceName = LVFreeTypeFace::familyName(_face);
	CRLog::debug("Loaded font %s [%d]: faceName=%s, ", _fileName.c_str(), index, _faceName.c_str() );
	//if ( !FT_IS_SCALABLE( _face ) ) {
	//    Clear();
	//    return false;
	// }
	error = FT_Set_Pixel_Sizes(
				_face,    /* handle to face object */
				0,        /* pixel_width           */
				size );  /* pixel_height          */
	if (error) {
		Clear();
		return false;
	}
#if 0
	int nheight = _face->size->metrics.height;
	int targetheight = size << 6;
	error = FT_Set_Pixel_Sizes(
				_face,    /* handle to face object */
				0,        /* pixel_width           */
				(size * targetheight + nheight/2)/ nheight );  /* pixel_height          */
#endif
	_height = _face->size->metrics.height >> 6;
	_size = size; //(_face->size->metrics.height >> 6);
	_baseline = _height + (_face->size->metrics.descender >> 6);
	_weight = _face->style_flags & FT_STYLE_FLAG_BOLD ? 700 : 400;
	_italic = _face->style_flags & FT_STYLE_FLAG_ITALIC ? 1 : 0;
	
	if ( !error && italicize && !_italic ) {
		_matrix.xy = 0x10000*3/10;
		FT_Set_Transform(_face, &_matrix, NULL);
		_italic = true;
	}
	
	if ( error ) {
		// error
		return false;
	}
	return true;
}

bool LVFreeTypeFace::loadFromFile(const char *fname, int index, int size, css_font_family_t fontFamily, bool monochrome, bool italicize)
{
	FONT_GUARD
			_hintingMode = LVFontManager::getInstance()->GetHintingMode();
	_drawMonochrome = monochrome;
	_fontFamily = fontFamily;
	if ( fname )
		_fileName = fname;
	if ( _fileName.empty() )
		return false;
	int error = FT_New_Face( _library, _fileName.c_str(), index, &_face ); /* create face object */
	if (error)
		return false;
	if ( _fileName.endsWith(".pfb") || _fileName.endsWith(".pfa") ) {
		lString8 kernFile = _fileName.substr(0, _fileName.length()-4);
		if ( LVFileExists(Utf8ToUnicode(kernFile) + ".afm") ) {
			kernFile += ".afm";
		} else if ( LVFileExists(Utf8ToUnicode(kernFile) + ".pfm" ) ) {
			kernFile += ".pfm";
		} else {
			kernFile.clear();
		}
		if ( !kernFile.empty() )
			error = FT_Attach_File( _face, kernFile.c_str() );
	}
	//FT_Face_SetUnpatentedHinting( _face, 1 );
	_slot = _face->glyph;
	_faceName = LVFreeTypeFace::familyName(_face);
	CRLog::debug("Loaded font %s [%d]: faceName=%s, ", _fileName.c_str(), index, _faceName.c_str() );
	//if ( !FT_IS_SCALABLE( _face ) ) {
	//    Clear();
	//    return false;
	// }
	error = FT_Set_Pixel_Sizes(
				_face,    /* handle to face object */
				0,        /* pixel_width           */
				size );  /* pixel_height          */
	if (error) {
		Clear();
		return false;
	}
#if 0
	int nheight = _face->size->metrics.height;
	int targetheight = size << 6;
	error = FT_Set_Pixel_Sizes(
				_face,    /* handle to face object */
				0,        /* pixel_width           */
				(size * targetheight + nheight/2)/ nheight );  /* pixel_height          */
#endif
	_height = _face->size->metrics.height >> 6;
	_size = size; //(_face->size->metrics.height >> 6);
	_baseline = _height + (_face->size->metrics.descender >> 6);
	_weight = _face->style_flags & FT_STYLE_FLAG_BOLD ? 700 : 400;
	_italic = _face->style_flags & FT_STYLE_FLAG_ITALIC ? 1 : 0;
	
	if ( !error && italicize && !_italic ) {
		_matrix.xy = 0x10000*3/10;
		FT_Set_Transform(_face, &_matrix, NULL);
		_italic = true;
	}
	
	if ( error ) {
		// error
		return false;
	}
	return true;
}

FT_UInt LVFreeTypeFace::getCharIndex(lChar16 code, lChar16 def_char) {
	if ( code=='\t' )
		code = ' ';
	FT_UInt ch_glyph_index = FT_Get_Char_Index( _face, code );
	if ( ch_glyph_index==0 ) {
		lUInt16 replacement = getReplacementChar( code );
		if ( replacement )
			ch_glyph_index = FT_Get_Char_Index( _face, replacement );
		if ( ch_glyph_index==0 && def_char )
			ch_glyph_index = FT_Get_Char_Index( _face, def_char );
	}
	return ch_glyph_index;
}

bool LVFreeTypeFace::getGlyphInfo(lUInt16 code, LVFont::glyph_info_t *glyph, lChar16 def_char)
{
	//FONT_GUARD
	int glyph_index = getCharIndex( code, 0 );
	if ( glyph_index==0 ) {
		LVFont * fallback = getFallbackFont();
		if ( !fallback ) {
			// No fallback
			glyph_index = getCharIndex( code, def_char );
			if ( glyph_index==0 )
				return false;
		} else {
			// Fallback
			return fallback->getGlyphInfo(code, glyph, def_char);
		}
	}
	int flags = FT_LOAD_DEFAULT;
	flags |= (!_drawMonochrome ? FT_LOAD_TARGET_NORMAL : FT_LOAD_TARGET_MONO);
	if (_hintingMode == HINTING_MODE_AUTOHINT)
		flags |= FT_LOAD_FORCE_AUTOHINT;
	else if (_hintingMode == HINTING_MODE_DISABLED)
		flags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
	updateTransform();
	int error = FT_Load_Glyph(
					_face,          /* handle to face object */
					glyph_index,   /* glyph index           */
					flags );  /* load flags, see below */
	if ( error )
		return false;
	glyph->blackBoxX = (lUInt8)(_slot->metrics.width >> 6);
	glyph->blackBoxY = (lUInt8)(_slot->metrics.height >> 6);
	glyph->originX =   (lInt8)(_slot->metrics.horiBearingX >> 6);
	glyph->originY =   (lInt8)(_slot->metrics.horiBearingY >> 6);
	glyph->width =     (lUInt8)(myabs(_slot->metrics.horiAdvance) >> 6);
	return true;
}

lUInt16 LVFreeTypeFace::measureText(const lChar16 *text, int len, lUInt16 *widths, lUInt8 *flags, int max_width, lChar16 def_char, int letter_spacing, bool allow_hyphenation)
{
	FONT_GUARD
			if ( len <= 0 || _face==NULL )
			return 0;
	int error;
	
#if (ALLOW_KERNING==1)
	int use_kerning = _allowKerning && FT_HAS_KERNING( _face );
#endif
	if ( letter_spacing<0 || letter_spacing>50 )
		letter_spacing = 0;
	
	//int i;
	
	FT_UInt previous = 0;
	lUInt16 prev_width = 0;
	int nchars = 0;
	int lastFitChar = 0;
	updateTransform();
	// measure character widths
	for ( nchars=0; nchars<len; nchars++) {
		lChar16 ch = text[nchars];
		bool isHyphen = (ch==UNICODE_SOFT_HYPHEN_CODE);
		FT_UInt ch_glyph_index = (FT_UInt)-1;
		int kerning = 0;
#if (ALLOW_KERNING==1)
		if ( use_kerning && previous>0  ) {
			if ( ch_glyph_index==(FT_UInt)-1 )
				ch_glyph_index = getCharIndex( ch, def_char );
			if ( ch_glyph_index != 0 ) {
				FT_Vector delta;
				error = FT_Get_Kerning( _face,          /* handle to face object */
										previous,          /* left glyph index      */
										ch_glyph_index,         /* right glyph index     */
										FT_KERNING_DEFAULT,  /* kerning mode          */
										&delta );    /* target vector         */
				if ( !error )
					kerning = delta.x;
			}
		}
#endif
		
		flags[nchars] = GET_CHAR_FLAGS(ch); //calcCharFlags( ch );
		
		/* load glyph image into the slot (erase previous one) */
		int w = _wcache.get(ch);
		if ( w==0xFF ) {
			glyph_info_t glyph;
			if ( getGlyphInfo( ch, &glyph, def_char ) ) {
				w = glyph.width;
				_wcache.put(ch, w);
			} else {
				widths[nchars] = prev_width;
				continue;  /* ignore errors */
			}
			if ( ch_glyph_index==(FT_UInt)-1 )
				ch_glyph_index = getCharIndex( ch, 0 );
			//                error = FT_Load_Glyph( _face,          /* handle to face object */
			//                        ch_glyph_index,                /* glyph index           */
			//                        FT_LOAD_DEFAULT );             /* load flags, see below */
			//                if ( error ) {
			//                    widths[nchars] = prev_width;
			//                    continue;  /* ignore errors */
			//                }
		}
		widths[nchars] = prev_width + w + (kerning >> 6) + letter_spacing;
		previous = ch_glyph_index;
		if ( !isHyphen ) // avoid soft hyphens inside text string
			prev_width = widths[nchars];
		if ( prev_width > max_width ) {
			if ( lastFitChar < nchars + 7)
				break;
		} else {
			lastFitChar = nchars + 1;
		}
	}
	
	// fill props for rest of chars
	for ( int ii=nchars; ii<len; ii++ ) {
		flags[nchars] = GET_CHAR_FLAGS( text[ii] );
	}
	
	//maxFit = nchars;
	
	
	// find last word
	if ( allow_hyphenation ) {
		if ( !_hyphen_width )
			_hyphen_width = getCharWidth( UNICODE_SOFT_HYPHEN_CODE );
		if ( lastFitChar > 3 ) {
			int hwStart, hwEnd;
			lStr_findWordBounds( text, len, lastFitChar-1, hwStart, hwEnd );
			if ( hwStart < lastFitChar-1 && hwEnd > hwStart+3 ) {
				//int maxw = max_width - (hwStart>0 ? widths[hwStart-1] : 0);
				HyphMan::hyphenate(text+hwStart, hwEnd-hwStart, widths+hwStart, flags+hwStart, _hyphen_width, max_width);
			}
		}
	}
	return lastFitChar; //nchars;
}

lUInt32 LVFreeTypeFace::getTextWidth(const lChar16 *text, int len)
{
	static lUInt16 widths[MAX_LINE_CHARS+1];
	static lUInt8 flags[MAX_LINE_CHARS+1];
	if ( len>MAX_LINE_CHARS )
		len = MAX_LINE_CHARS;
	if ( len<=0 )
		return 0;
	lUInt16 res = measureText(
					  text, len,
					  widths,
					  flags,
					  2048, // max_width,
					  L' ',  // def_char
					  0
					  );
	if ( res>0 && res<MAX_LINE_CHARS )
		return widths[res-1];
	return 0;
}

void LVFreeTypeFace::updateTransform() {
//	static void * transformOwner = NULL;
//	if ( transformOwner!=this ) {
//		FT_Set_Transform(_face, &_matrix, NULL);
//		transformOwner = this;
//	}
}

LVFontGlyphCacheItem *LVFreeTypeFace::getGlyph(lUInt16 ch, lChar16 def_char) {
	//FONT_GUARD
	FT_UInt ch_glyph_index = getCharIndex( ch, 0 );
	if ( ch_glyph_index==0 ) {
		LVFont * fallback = getFallbackFont();
		if ( !fallback ) {
			// No fallback
			ch_glyph_index = getCharIndex( ch, def_char );
			if ( ch_glyph_index==0 )
				return NULL;
		} else {
			// Fallback
			return fallback->getGlyph(ch, def_char);
		}
	}
	LVFontGlyphCacheItem * item = _glyph_cache.get( ch );
	if ( !item ) {
		
		int rend_flags = FT_LOAD_RENDER | ( !_drawMonochrome ? FT_LOAD_TARGET_NORMAL : (FT_LOAD_TARGET_MONO) ); //|FT_LOAD_MONOCHROME|FT_LOAD_FORCE_AUTOHINT
		if (_hintingMode == HINTING_MODE_AUTOHINT)
			rend_flags |= FT_LOAD_FORCE_AUTOHINT;
		else if (_hintingMode == HINTING_MODE_DISABLED)
			rend_flags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
		/* load glyph image into the slot (erase previous one) */
		
		updateTransform();
		int error = FT_Load_Glyph( _face,          /* handle to face object */
								   ch_glyph_index,                /* glyph index           */
								   rend_flags );             /* load flags, see below */
		if ( error ) {
			return NULL;  /* ignore errors */
		}
		item = LVFontGlyphCacheItem::newItem( &_glyph_cache, ch, _slot ); //, _drawMonochrome
		_glyph_cache.put( item );
	}
	return item;
}

#if 0
bool LVFreeTypeFace::getGlyphImage(lUInt16 ch, lUInt8 *bmp, lChar16 def_char)
{
	LVFontGlyphCacheItem * item = getGlyph(ch);
	if ( item )
		memcpy( bmp, item->bmp, item->bmp_width * item->bmp_height );
	return item;
}
#endif

int LVFreeTypeFace::getCharWidth(lChar16 ch, lChar16 def_char)
{
	int w = _wcache.get(ch);
	if ( w==0xFF ) {
		glyph_info_t glyph;
		if ( getGlyphInfo( ch, &glyph, def_char ) ) {
			w = glyph.width;
		} else {
			w = 0;
		}
		_wcache.put(ch, w);
	}
	return w;
}

bool LVFreeTypeFace::kerningEnabled() {
#if (ALLOW_KERNING==1)
	return _allowKerning && FT_HAS_KERNING( _face );
#else
	return false;
#endif
}

int LVFreeTypeFace::getKerningOffset(lChar16 ch1, lChar16 ch2, lChar16 def_char) {
#if (ALLOW_KERNING==1)
	FT_UInt ch_glyph_index1 = getCharIndex( ch1, def_char );
	FT_UInt ch_glyph_index2 = getCharIndex( ch2, def_char );
	if (ch_glyph_index1 > 0 && ch_glyph_index2 > 0) {
		FT_Vector delta;
		int error = FT_Get_Kerning(
						_face,          /* handle to face object */
						ch_glyph_index1,          /* left glyph index      */
						ch_glyph_index2,         /* right glyph index     */
						FT_KERNING_DEFAULT,  /* kerning mode          */
						&delta );    /* target vector         */
		if ( !error )
			return delta.x;
	}
#endif
	return 0;
}

void LVFreeTypeFace::DrawTextString(LVDrawBuf *buf, int x, int y, const lChar16 *text, int len, lChar16 def_char, lUInt32 *palette, bool addHyphen, lUInt32 flags, int letter_spacing)
{
	FONT_GUARD
			if ( len <= 0 || _face==NULL )
			return;
	if ( letter_spacing<0 || letter_spacing>50 )
		letter_spacing = 0;
	lvRect clip;
	buf->GetClipRect( &clip );
	updateTransform();
	if ( y + _height < clip.top || y >= clip.bottom )
		return;
	
	int error;
	
#if (ALLOW_KERNING==1)
	int use_kerning = _allowKerning && FT_HAS_KERNING( _face );
#endif
	int i;
	
	FT_UInt previous = 0;
	//lUInt16 prev_width = 0;
	lChar16 ch;
	// measure character widths
	bool isHyphen = false;
	int x0 = x;
	for ( i=0; i<=len; i++) {
		if ( i==len && (!addHyphen || isHyphen) )
			break;
		if ( i<len ) {
			ch = text[i];
			if ( ch=='\t' )
				ch = ' ';
			isHyphen = (ch==UNICODE_SOFT_HYPHEN_CODE) && (i<len-1);
		} else {
			ch = UNICODE_SOFT_HYPHEN_CODE;
			isHyphen = 0;
		}
		FT_UInt ch_glyph_index = getCharIndex( ch, def_char );
		int kerning = 0;
#if (ALLOW_KERNING==1)
		if ( use_kerning && previous>0 && ch_glyph_index>0 ) {
			FT_Vector delta;
			error = FT_Get_Kerning( _face,          /* handle to face object */
									previous,          /* left glyph index      */
									ch_glyph_index,         /* right glyph index     */
									FT_KERNING_DEFAULT,  /* kerning mode          */
									&delta );    /* target vector         */
			if ( !error )
				kerning = delta.x;
		}
#endif
		
		
		LVFontGlyphCacheItem * item = getGlyph(ch, def_char);
		_glyph_cache.get( ch );
		if ( !item )
			continue;
		if ( (item && !isHyphen) || i>=len-1 ) { // avoid soft hyphens inside text string
			int w = item->advance + (kerning >> 6);
			buf->Draw( x + (kerning>>6) + item->origin_x,
					   y + _baseline - item->origin_y,
					   item->bmp,
					   item->bmp_width,
					   item->bmp_height,
					   palette);
			
			x  += w + letter_spacing;
			previous = ch_glyph_index;
		}
	}
	if ( flags & LTEXT_TD_MASK ) {
		// text decoration: underline, etc.
		int h = _size > 30 ? 2 : 1;
		lUInt32 cl = buf->GetTextColor();
		if ( (flags & LTEXT_TD_UNDERLINE) || (flags & LTEXT_TD_BLINK) ) {
			int liney = y + _baseline + h;
			buf->FillRect( x0, liney, x, liney+h, cl );
		}
		if ( flags & LTEXT_TD_OVERLINE ) {
			int liney = y + h;
			buf->FillRect( x0, liney, x, liney+h, cl );
		}
		if ( flags & LTEXT_TD_LINE_THROUGH ) {
			//                int liney = y + _baseline - _size/4 - h/2;
			int liney = y + _baseline - _size*2/7;
			buf->FillRect( x0, liney, x, liney+h, cl );
		}
	}
}

void LVFreeTypeFace::Clear()
{
	LVLock lock(_mutex);
	if ( _face )
		FT_Done_Face( _face );
	_face = NULL;
}

#endif	// USE_FREETYPE==1
