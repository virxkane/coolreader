/** \file lvbasedrawbuf.h
	\brief Base drawing buffer

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.

*/

#ifndef __LVBASEDRAWBUF_H_INCLUDED__
#define __LVBASEDRAWBUF_H_INCLUDED__

#include "lvdrawbuf.h"

/// LVDrawBufferBase
class LVBaseDrawBuf : public LVDrawBuf
{
protected:
	int _dx;
	int _dy;
	int _rowsize;
	lvRect _clip;
	unsigned char * _data;
	lUInt32 _backgroundColor;
	lUInt32 _textColor;
	bool _hidePartialGlyphs;
public:
	virtual void setHidePartialGlyphs( bool hide ) { _hidePartialGlyphs = hide; }
	/// returns current background color
	virtual lUInt32 GetBackgroundColor() { return _backgroundColor; }
	/// sets current background color
	virtual void SetBackgroundColor( lUInt32 cl ) { _backgroundColor=cl; }
	/// returns current text color
	virtual lUInt32 GetTextColor() { return _textColor; }
	/// sets current text color
	virtual void SetTextColor( lUInt32 cl ) { _textColor = cl; }
	/// gets clip rect
	virtual void GetClipRect( lvRect * clipRect ) { *clipRect = _clip; }
	/// sets clip rect
	virtual void SetClipRect( const lvRect * clipRect );
	/// get average pixel value for area (coordinates are fixed floating points *16)
	virtual lUInt32 GetAvgColor(lvRect & rc16);
	/// get linearly interpolated pixel value (coordinates are fixed floating points *16)
	virtual lUInt32 GetInterpolatedColor(int x16, int y16);
	/// get buffer width, pixels
	virtual int  GetWidth();
	/// get buffer height, pixels
	virtual int  GetHeight();
	/// get row size (bytes)
	virtual int  GetRowSize() { return _rowsize; }
	/// draws text string
	/*
	virtual void DrawTextString( int x, int y, LVFont * pfont,
					   const lChar16 * text, int len,
					   lChar16 def_char,
					   lUInt32 * palette, bool addHyphen=false );
	*/
	/// draws formatted text
	//virtual void DrawFormattedText( formatted_text_fragment_t * text, int x, int y );

	LVBaseDrawBuf() : _dx(0), _dy(0), _rowsize(0), _data(NULL), _hidePartialGlyphs(true) { }
	virtual ~LVBaseDrawBuf() { }
};

#endif	// __LVBASEDRAWBUF_H_INCLUDED__
