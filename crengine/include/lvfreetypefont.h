/** \file lvfreetypefont.h
	\brief FreeTYpe font interface

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006

	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#ifndef __LV_FREETYPE_FONT_H_INCLUDED__
#define __LV_FREETYPE_FONT_H_INCLUDED__

#include "crsetup.h"

#if USE_FREETYPE==1

#include "lvfont.h"
#include "lvthread.h"
#include "lvfontglyphcache.h"
#include "lvarray.h"

#include <ft2build.h>
#include FT_FREETYPE_H

class LVFreeTypeFace : public LVFont
{
	friend class LVFreeTypeFontManager;
protected:
	LVMutex &      _mutex;
	lString8      _fileName;
	lString8      _faceName;
	css_font_family_t _fontFamily;
	FT_Library    _library;
	FT_Face       _face;
	FT_GlyphSlot  _slot;
	FT_Matrix     _matrix;                 /* transformation matrix */
	int           _size; // caracter height in pixels
	int           _height; // full line height in pixels
	int           _hyphen_width;
	int           _baseline;
	int            _weight;
	int            _italic;
	LVFontGlyphWidthCache _wcache;
	LVFontLocalGlyphCache _glyph_cache;
	bool          _drawMonochrome;
	bool          _allowKerning;
	hinting_mode_t _hintingMode;
	bool          _fallbackFontIsSet;
	LVFontRef     _fallbackFont;
protected:
	static lString8 familyName( FT_Face face );
public:
	// fallback font support
	/// set fallback font for this font
	void setFallbackFont( LVFontRef font ) {
		_fallbackFont = font;
		_fallbackFontIsSet = !font.isNull();
	}
	/// get fallback font for this font
	LVFont * getFallbackFont();
	/// returns font weight
	virtual int getWeight() const { return _weight; }
	/// returns italic flag
	virtual int getItalic() const { return _italic; }
	/// sets face name
	virtual void setFaceName( lString8 face ) { _faceName = face; }
	LVMutex & getMutex() { return _mutex; }
	FT_Library getLibrary() { return _library; }
	LVFreeTypeFace( LVMutex &mutex, FT_Library  library, LVFontGlobalGlyphCache * globalCache );
	virtual ~LVFreeTypeFace();
	virtual int getHyphenWidth();
	/// get kerning mode: true==ON, false=OFF
	virtual bool getKerning() const { return _allowKerning; }
	/// get kerning mode: true==ON, false=OFF
	virtual void setKerning( bool kerningEnabled ) { _allowKerning = kerningEnabled; }
	/// sets current hinting mode
	virtual void setHintingMode(hinting_mode_t mode);
	/// returns current hinting mode
	virtual hinting_mode_t  getHintingMode() const { return _hintingMode; }
	/// get bitmap mode (true=bitmap, false=antialiased)
	virtual bool getBitmapMode() { return _drawMonochrome; }
	/// set bitmap mode (true=bitmap, false=antialiased)
	virtual void setBitmapMode( bool drawBitmap );
	bool loadFromBuffer(LVByteArrayRef buf, int index, int size, css_font_family_t fontFamily, bool monochrome, bool italicize );
	bool loadFromFile( const char * fname, int index, int size, css_font_family_t fontFamily, bool monochrome, bool italicize );
	FT_UInt getCharIndex( lChar16 code, lChar16 def_char );
	/** \brief get glyph info
		\param glyph is pointer to glyph_info_t struct to place retrieved info
		\return true if glyh was found
	*/
	virtual bool getGlyphInfo( lUInt16 code, glyph_info_t * glyph, lChar16 def_char=0 );
#if 0
	// USE GET_CHAR_FLAGS instead
	inline int calcCharFlags( lChar16 ch )
	{
		switch ( ch ) {
		case 0x0020:
			return LCHAR_IS_SPACE | LCHAR_ALLOW_WRAP_AFTER;
		case UNICODE_SOFT_HYPHEN_CODE:
			return LCHAR_ALLOW_WRAP_AFTER;
		case '-':
			return LCHAR_DEPRECATED_WRAP_AFTER;
		case '\r':
		case '\n':
			return LCHAR_IS_SPACE | LCHAR_IS_EOL | LCHAR_ALLOW_WRAP_AFTER;
		default:
			return 0;
		}
	}
#endif
	/** \brief measure text
		\param text is text string pointer
		\param len is number of characters to measure
		\return number of characters before max_width reached
	*/
	virtual lUInt16 measureText(
						const lChar16 * text, int len,
						lUInt16 * widths,
						lUInt8 * flags,
						int max_width,
						lChar16 def_char,
						int letter_spacing = 0,
						bool allow_hyphenation = true
					 );
	/** \brief measure text
		\param text is text string pointer
		\param len is number of characters to measure
		\return width of specified string
	*/
	virtual lUInt32 getTextWidth(const lChar16 * text, int len);
	void updateTransform();
	/** \brief get glyph item
		\param code is unicode character
		\return glyph pointer if glyph was found, NULL otherwise
	*/
	virtual LVFontGlyphCacheItem * getGlyph(lUInt16 ch, lChar16 def_char=0);
//    /** \brief get glyph image in 1 byte per pixel format
//        \param code is unicode character
//        \param buf is buffer [width*height] to place glyph data
//        \return true if glyph was found
//    */
	//virtual bool getGlyphImage(lUInt16 ch, lUInt8 * bmp, lChar16 def_char=0);
	/// returns font baseline offset
	virtual int getBaseline()
	{
		return _baseline;
	}
	/// returns font height
	virtual int getHeight() const
	{
		return _height;
	}
	/// returns font character size
	virtual int getSize() const
	{
		return _size;
	}
	/// returns char width
	virtual int getCharWidth( lChar16 ch, lChar16 def_char='?' );
	/// retrieves font handle
	virtual void * GetHandle()
	{
		return NULL;
	}
	/// returns font typeface name
	virtual lString8 getTypeFace() const
	{
		return _faceName;
	}
	/// returns font family id
	virtual css_font_family_t getFontFamily() const
	{
		return _fontFamily;
	}
	virtual bool kerningEnabled();
	virtual int getKerningOffset(lChar16 ch1, lChar16 ch2, lChar16 def_char);
	/// draws text string
	virtual void DrawTextString( LVDrawBuf * buf, int x, int y,
					   const lChar16 * text, int len,
					   lChar16 def_char, lUInt32 * palette, bool addHyphen, lUInt32 flags, int letter_spacing );
	/// returns true if font is empty
	virtual bool IsNull() const
	{
		return _face == NULL;
	}
	virtual bool operator ! () const
	{
		return _face == NULL;
	}
	virtual void Clear();
};

#endif	// USE_FREETYPE==1

#endif	// __LV_FREETYPE_FONT_H_INCLUDED__
