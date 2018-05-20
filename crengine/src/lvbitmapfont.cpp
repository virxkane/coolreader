/** \file lvbitmapfont.cpp
	\brief bitmap font implementation

	CoolReader Engine


	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#include "../include/lvbitmapfont.h"


#if USE_BITMAP_FONTS==1

bool LBitmapFont::getGlyphInfo( lUInt16 code, LVFont::glyph_info_t * glyph, lChar16 def_char )
{
	const lvfont_glyph_t * ptr = lvfontGetGlyph( m_font, code );
	if (!ptr)
		return false;
	glyph->blackBoxX = ptr->blackBoxX;
	glyph->blackBoxY = ptr->blackBoxY;
	glyph->originX = ptr->originX;
	glyph->originY = ptr->originY;
	glyph->width = ptr->width;
	return true;
}

lUInt16 LBitmapFont::measureText(
					const lChar16 * text, int len,
					lUInt16 * widths,
					lUInt8 * flags,
					int max_width,
					lChar16 def_char,
					int letter_spacing,
					bool allow_hyphenation
				 )
{
	return lvfontMeasureText( m_font, text, len, widths, flags, max_width, def_char );
}

lUInt32 LBitmapFont::getTextWidth( const lChar16 * text, int len )
{
	//
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
					L' '  // def_char
				 );
	if ( res>0 && res<MAX_LINE_CHARS )
		return widths[res-1];
	return 0;
}

/// returns font baseline offset
int LBitmapFont::getBaseline()
{
	const lvfont_header_t * hdr = lvfontGetHeader( m_font );
	return hdr->fontBaseline;
}

/// returns font height
int LBitmapFont::getHeight() const
{
	const lvfont_header_t * hdr = lvfontGetHeader( m_font );
	return hdr->fontHeight;
}

int LBitmapFont::getWeight() const
{
	const lvfont_header_t * hdr = lvfontGetHeader(m_font);
	return hdr->flgBold;
}

int LBitmapFont::getItalic() const
{
	const lvfont_header_t * hdr = lvfontGetHeader(m_font);
	return hdr->flgItalic;
}



bool LBitmapFont::getGlyphImage(lUInt16 code, lUInt8 * buf, lChar16 def_char)
{
	const lvfont_glyph_t * ptr = lvfontGetGlyph( m_font, code );
	if (!ptr)
		return false;
	const hrle_decode_info_t * pDecodeTable = lvfontGetDecodeTable( m_font );
	int sz = ptr->blackBoxX*ptr->blackBoxY;
	if (sz)
		lvfontUnpackGlyph(ptr->glyph, pDecodeTable, buf, sz);
	return true;
}

int LBitmapFont::LoadFromFile( const char * fname )
{
	Clear();
	int res = (void*)lvfontOpen( fname, &m_font )!=NULL;
	if (!res)
		return 0;
	lvfont_header_t * hdr = (lvfont_header_t*) m_font;
	_typeface = lString8( hdr->fontName );
	_family = (css_font_family_t) hdr->fontFamily;
	return 1;
}

#endif	// USE_BITMAP_FONTS==1
