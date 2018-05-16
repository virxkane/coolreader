/** \file lvgraydrawbuf.h
	\brief Gray bitmap buffer

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.

*/

#ifndef __LVGRAYDRAWBUF_H_INCLUDED__
#define __LVGRAYDRAWBUF_H_INCLUDED__

#include "lvbasedrawbuf.h"

/**
 * 2-bit gray bitmap buffer, partial support for 1-bit buffer
 * Supported pixel formats for LVGrayDrawBuf :
 *    1 bpp, 8 pixels per byte packed
 *    2 bpp, 4 pixels per byte packed
 *    3 bpp, 1 pixel per byte, higher 3 bits are significant
 *    4 bpp, 1 pixel per byte, higher 4 bits are significant
 *    8 bpp, 1 pixel per byte, all 8 bits are significant
 *
 */
class LVGrayDrawBuf : public LVBaseDrawBuf
{
private:
	int _bpp;
	bool _ownData;
public:
	/// rotates buffer contents by specified angle
	virtual void Rotate( cr_rotate_angle_t angle );
	/// returns white pixel value
	virtual lUInt32 GetWhiteColor();
	/// returns black pixel value
	virtual lUInt32 GetBlackColor();
	/// draws buffer content to another buffer doing color conversion if necessary
	virtual void DrawTo( LVDrawBuf * buf, int x, int y, int options, lUInt32 * palette );
	/// draws rescaled buffer content to another buffer doing color conversion if necessary
	virtual void DrawRescaled(LVDrawBuf * src, int x, int y, int dx, int dy, int options);
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
	/// draws buffer content to another buffer doing color conversion if necessary
	virtual void DrawTo( HDC dc, int x, int y, int options, lUInt32 * palette );
#endif
	/// invert image
	virtual void  Invert();
	/// get buffer bits per pixel
	virtual int  GetBitsPerPixel();
	/// returns scanline pointer
	virtual lUInt8 * GetScanLine( int y );
	/// fills buffer with specified color
	virtual void Clear( lUInt32 color );
	/// get pixel value
	virtual lUInt32 GetPixel( int x, int y );
	/// fills rectangle with specified color
	virtual void FillRect( int x0, int y0, int x1, int y1, lUInt32 color );
	/// inverts image in specified rectangle
	virtual void InvertRect( int x0, int y0, int x1, int y1 );
	/// fills rectangle with pattern
	virtual void FillRectPattern( int x0, int y0, int x1, int y1, lUInt32 color0, lUInt32 color1, lUInt8 * pattern );
	/// sets new size
	virtual void Resize( int dx, int dy );
	/// draws image
	virtual void Draw( LVImageSourceRef img, int x, int y, int width, int height, bool dither );
	/// draws bitmap (1 byte per pixel) using specified palette
	virtual void Draw( int x, int y, const lUInt8 * bitmap, int width, int height, lUInt32 * palette );
	/// constructor
	LVGrayDrawBuf(int dx, int dy, int bpp=2, void * auxdata = NULL );
	/// destructor
	virtual ~LVGrayDrawBuf();
	/// convert to 1-bit bitmap
	void ConvertToBitmap(bool flgDither);
};

#endif	// __LVGRAYDRAWBUF_H_INCLUDED__
