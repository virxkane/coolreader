/*******************************************************

   CoolReader Engine

   lvcolordrawbuf.cpp:  32-bit RGB color bitmap buffer class

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "../include/lvcolordrawbuf.h"
#include "../include/lvimagescaled_callback.h"
#include "../include/crlog.h"

#define GUARD_BYTE 0xa5
#define CHECK_GUARD_BYTE \
	{ \
		if (_bpp != 1 && _bpp != 2 && _bpp !=3 && _bpp != 4 && _bpp != 8 && _bpp != 16 && _bpp != 32) crFatalError(-5, "wrong bpp"); \
		if (_ownData && _data[_rowsize * _dy] != GUARD_BYTE) crFatalError(-5, "corrupted bitmap buffer"); \
	}

static void ApplyAlphaRGB565( lUInt16 &dst, lUInt16 src, lUInt32 alpha )
{
	if ( alpha==0 )
		dst = src;
	else if ( alpha<255 ) {
		lUInt32 opaque = 256 - alpha;
		lUInt32 r = (((dst & 0xF800) * alpha + (src & 0xF800) * opaque) >> 8) & 0xF800;
		lUInt32 g = (((dst & 0x07E0) * alpha + (src & 0x07E0) * opaque) >> 8) & 0x07E0;
		lUInt32 b = (((dst & 0x001F) * alpha + (src & 0x001F) * opaque) >> 8) & 0x001F;
		dst = (lUInt16)(r | g | b);
	}
}

//=======================================================
// 32-bit RGB buffer
//=======================================================

/// rotates buffer contents by specified angle
void LVColorDrawBuf::Rotate( cr_rotate_angle_t angle )
{
	if ( angle==CR_ROTATE_ANGLE_0 )
		return;
	if ( _bpp==16 ) {
		int sz = (_dx * _dy);
		if ( angle==CR_ROTATE_ANGLE_180 ) {
			lUInt16 * buf = (lUInt16 *) _data;
			for ( int i=sz/2-1; i>=0; i-- ) {
				lUInt16 tmp = buf[i];
				buf[i] = buf[sz-i-1];
				buf[sz-i-1] = tmp;
			}
			return;
		}
		int newrowsize = _dy * 2;
		sz = (_dx * newrowsize);
		lUInt16 * dst = (lUInt16*) malloc( sz );
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
		bool cw = angle!=CR_ROTATE_ANGLE_90;
#else
		bool cw = angle==CR_ROTATE_ANGLE_90;
#endif
		for ( int y=0; y<_dy; y++ ) {
			lUInt16 * src = (lUInt16*)_data + _dx*y;
			int nx, ny;
			if ( cw ) {
				nx = _dy - 1 - y;
			} else {
				nx = y;
			}
			for ( int x=0; x<_dx; x++ ) {
				if ( cw ) {
					ny = x;
				} else {
					ny = _dx - 1 - x;
				}
				dst[ _dy*ny + nx ] = src[ x ];
			}
		}
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
		memcpy( _data, dst, sz );
		free( dst );
#else
		free( _data );
		_data = (lUInt8*)dst;
#endif
		int tmp = _dx;
		_dx = _dy;
		_dy = tmp;
		_rowsize = newrowsize;
	} else {
		int sz = (_dx * _dy);
		if ( angle==CR_ROTATE_ANGLE_180 ) {
			lUInt32 * buf = (lUInt32 *) _data;
			for ( int i=sz/2-1; i>=0; i-- ) {
				lUInt32 tmp = buf[i];
				buf[i] = buf[sz-i-1];
				buf[sz-i-1] = tmp;
			}
			return;
		}
		int newrowsize = _dy * 4;
		sz = (_dx * newrowsize);
		lUInt32 * dst = (lUInt32*) malloc( sz );
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
		bool cw = angle!=CR_ROTATE_ANGLE_90;
#else
		bool cw = angle==CR_ROTATE_ANGLE_90;
#endif
		for ( int y=0; y<_dy; y++ ) {
			lUInt32 * src = (lUInt32*)_data + _dx*y;
			int nx, ny;
			if ( cw ) {
				nx = _dy - 1 - y;
			} else {
				nx = y;
			}
			for ( int x=0; x<_dx; x++ ) {
				if ( cw ) {
					ny = x;
				} else {
					ny = _dx - 1 - x;
				}
				dst[ _dy*ny + nx ] = src[ x ];
			}
		}
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
		memcpy( _data, dst, sz );
		free( dst );
#else
		free( _data );
		_data = (lUInt8*)dst;
#endif
		int tmp = _dx;
		_dx = _dy;
		_dy = tmp;
		_rowsize = newrowsize;
	}
}

/// invert image
void  LVColorDrawBuf::Invert()
{
}

/// get buffer bits per pixel
int  LVColorDrawBuf::GetBitsPerPixel()
{
	return _bpp;
}

void LVColorDrawBuf::Draw( LVImageSourceRef img, int x, int y, int width, int height, bool dither )
{
	//fprintf( stderr, "LVColorDrawBuf::Draw( img(%d, %d), %d, %d, %d, %d\n", img->GetWidth(), img->GetHeight(), x, y, width, height );
	LVImageScaledDrawCallback drawcb( this, img, x, y, width, height, dither );
	img->Decode( &drawcb );
}

/// fills buffer with specified color
void LVColorDrawBuf::Clear( lUInt32 color )
{
	if ( _bpp==16 ) {
		lUInt16 cl16 = rgb888to565(color);
		for (int y=0; y<_dy; y++)
		{
			lUInt16 * line = (lUInt16 *)GetScanLine(y);
			for (int x=0; x<_dx; x++)
			{
				line[x] = cl16;
			}
		}
	} else {
		for (int y=0; y<_dy; y++)
		{
			lUInt32 * line = (lUInt32 *)GetScanLine(y);
			for (int x=0; x<_dx; x++)
			{
				line[x] = color;
			}
		}
	}
}


/// get pixel value
lUInt32 LVColorDrawBuf::GetPixel( int x, int y )
{
	if (!_data || y<0 || x<0 || y>=_dy || x>=_dx)
		return 0;
	if ( _bpp==16 )
		return rgb565to888(((lUInt16*)GetScanLine(y))[x]);
	return ((lUInt32*)GetScanLine(y))[x];
}

/// fills rectangle with specified color
void LVColorDrawBuf::FillRect( int x0, int y0, int x1, int y1, lUInt32 color )
{
	if (x0<_clip.left)
		x0 = _clip.left;
	if (y0<_clip.top)
		y0 = _clip.top;
	if (x1>_clip.right)
		x1 = _clip.right;
	if (y1>_clip.bottom)
		y1 = _clip.bottom;
	if (x0>=x1 || y0>=y1)
		return;
	int alpha = (color >> 24) & 0xFF;
	if ( _bpp==16 ) {
		lUInt16 cl16 = rgb888to565(color);
		for (int y=y0; y<y1; y++)
		{
			lUInt16 * line = (lUInt16 *)GetScanLine(y);
			for (int x=x0; x<x1; x++)
			{
				if (alpha)
					ApplyAlphaRGB565(line[x], cl16, alpha);
				else
					line[x] = cl16;
			}
		}
	} else {
		for (int y=y0; y<y1; y++)
		{
			lUInt32 * line = (lUInt32 *)GetScanLine(y);
			for (int x=x0; x<x1; x++)
			{
				if (alpha)
					ApplyAlphaRGB(line[x], color, alpha);
				else
					line[x] = color;
			}
		}
	}
}

/// fills rectangle with specified color
void LVColorDrawBuf::FillRectPattern( int x0, int y0, int x1, int y1, lUInt32 color0, lUInt32 color1, lUInt8 * pattern )
{
	if (x0<_clip.left)
		x0 = _clip.left;
	if (y0<_clip.top)
		y0 = _clip.top;
	if (x1>_clip.right)
		x1 = _clip.right;
	if (y1>_clip.bottom)
		y1 = _clip.bottom;
	if (x0>=x1 || y0>=y1)
		return;
	if ( _bpp==16 ) {
		lUInt16 cl16_0 = rgb888to565(color0);
		lUInt16 cl16_1 = rgb888to565(color1);
		for (int y=y0; y<y1; y++)
		{
			lUInt8 patternMask = pattern[y & 3];
			lUInt16 * line = (lUInt16 *)GetScanLine(y);
			for (int x=x0; x<x1; x++)
			{
				lUInt8 patternBit = (patternMask << (x&7)) & 0x80;
				line[x] = patternBit ? cl16_1 : cl16_0;
			}
		}
	} else {
		for (int y=y0; y<y1; y++)
		{
			lUInt8 patternMask = pattern[y & 3];
			lUInt32 * line = (lUInt32 *)GetScanLine(y);
			for (int x=x0; x<x1; x++)
			{
				lUInt8 patternBit = (patternMask << (x&7)) & 0x80;
				line[x] = patternBit ? color1 : color0;
			}
		}
	}
}

/// sets new size
void LVColorDrawBuf::Resize( int dx, int dy )
{
	if ( dx==_dx && dy==_dy ) {
		//CRLog::trace("LVColorDrawBuf::Resize : no resize, not changed");
		return;
	}
	if ( !_ownData ) {
		//CRLog::trace("LVColorDrawBuf::Resize : no resize, own data");
		return;
	}
	//CRLog::trace("LVColorDrawBuf::Resize : resizing %d x %d to %d x %d", _dx, _dy, dx, dy);
	// delete old bitmap
	if ( _dx>0 && _dy>0 && _data )
	{
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
		if (_drawbmp)
			DeleteObject( _drawbmp );
		if (_drawdc)
			DeleteObject( _drawdc );
		_drawbmp = NULL;
		_drawdc = NULL;
#else
		free(_data);
#endif
		_data = NULL;
		_rowsize = 0;
		_dx = 0;
		_dy = 0;
	}

	if (dx>0 && dy>0)
	{
		// create new bitmap
		_dx = dx;
		_dy = dy;
		_rowsize = dx*(_bpp>>3);
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
		BITMAPINFO bmi;
		memset( &bmi, 0, sizeof(bmi) );
		bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
		bmi.bmiHeader.biWidth = _dx;
		bmi.bmiHeader.biHeight = _dy;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = 0;
		bmi.bmiHeader.biXPelsPerMeter = 1024;
		bmi.bmiHeader.biYPelsPerMeter = 1024;
		bmi.bmiHeader.biClrUsed = 0;
		bmi.bmiHeader.biClrImportant = 0;

		_drawbmp = CreateDIBSection( NULL, &bmi, DIB_RGB_COLORS, (void**)(&_data), NULL, 0 );
		_drawdc = CreateCompatibleDC(NULL);
		SelectObject(_drawdc, _drawbmp);
#else
		_data = (lUInt8 *)malloc( (_bpp>>3) * _dx * _dy);
#endif
		memset( _data, 0, _rowsize * _dy );
	}
	SetClipRect( NULL );
}
void LVColorDrawBuf::InvertRect(int x0, int y0, int x1, int y1)
{
	CR_UNUSED4(x0, y0, x1, y1);
}

/// draws bitmap (1 byte per pixel) using specified palette
void LVColorDrawBuf::Draw( int x, int y, const lUInt8 * bitmap, int width, int height, lUInt32 * palette )
{
	//int buf_width = _dx; /* 2bpp */
	int initial_height = height;
	int bx = 0;
	int by = 0;
	int xx;
	int bmp_width = width;
	lUInt32 bmpcl = palette?palette[0]:GetTextColor();
	const lUInt8 * src;

	if (x<_clip.left)
	{
		width += x-_clip.left;
		bx -= x-_clip.left;
		x = _clip.left;
		if (width<=0)
			return;
	}
	if (y<_clip.top)
	{
		height += y-_clip.top;
		by -= y-_clip.top;
		y = _clip.top;
		if (_hidePartialGlyphs && height<=initial_height/2) // HIDE PARTIAL VISIBLE GLYPHS
			return;
		if (height<=0)
			return;
	}
	if (x + width > _clip.right)
	{
		width = _clip.right - x;
	}
	if (width<=0)
		return;
	if (y + height > _clip.bottom)
	{
		if (_hidePartialGlyphs && height<=initial_height/2) // HIDE PARTIAL VISIBLE GLYPHS
			return;
		int clip_bottom = _clip.bottom;
		if (_hidePartialGlyphs )
			clip_bottom = this->_dy;
		if ( y+height > clip_bottom)
			height = clip_bottom - y;
	}
	if (height<=0)
		return;

	xx = width;

	bitmap += bx + by*bmp_width;

	if ( _bpp==16 ) {

		lUInt16 bmpcl16 = rgb888to565(bmpcl);

		lUInt16 * dst;
		lUInt16 * dstline;


		for (;height;height--)
		{
			src = bitmap;
			dstline = ((lUInt16*)GetScanLine(y++)) + x;
			dst = dstline;

			for (xx = width; xx>0; --xx)
			{
				lUInt32 opaque = ((*(src++))>>4)&0x0F;
				if ( opaque>=0xF )
					*dst = bmpcl16;
				else if ( opaque>0 ) {
					lUInt32 alpha = 0xF-opaque;
					lUInt16 cl1 = (lUInt16)(((alpha*((*dst)&0xF81F) + opaque*(bmpcl16&0xF81F))>>4) & 0xF81F);
					lUInt16 cl2 = (lUInt16)(((alpha*((*dst)&0x07E0) + opaque*(bmpcl16&0x07E0))>>4) & 0x07E0);
					*dst = cl1 | cl2;
				}
				/* next pixel */
				dst++;
			}
			/* new dest line */
			bitmap += bmp_width;
		}

	} else {


		lUInt32 * dst;
		lUInt32 * dstline;


		for (;height;height--)
		{
			src = bitmap;
			dstline = ((lUInt32*)GetScanLine(y++)) + x;
			dst = dstline;

			for (xx = width; xx>0; --xx)
			{
				lUInt32 opaque = ((*(src++))>>1)&0x7F;
				if ( opaque>=0x78 )
					*dst = bmpcl;
				else if ( opaque>0 ) {
					lUInt32 alpha = 0x7F-opaque;
					lUInt32 cl1 = ((alpha*((*dst)&0xFF00FF) + opaque*(bmpcl&0xFF00FF))>>7) & 0xFF00FF;
					lUInt32 cl2 = ((alpha*((*dst)&0x00FF00) + opaque*(bmpcl&0x00FF00))>>7) & 0x00FF00;
					*dst = cl1 | cl2;
				}
				/* next pixel */
				dst++;
			}
			/* new dest line */
			bitmap += bmp_width;
		}
	}
}

#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
/// draws buffer content to DC doing color conversion if necessary
void LVColorDrawBuf::DrawTo( HDC dc, int x, int y, int options, lUInt32 * palette )
{
	if (dc!=NULL && _drawdc!=NULL)
		BitBlt( dc, x, y, _dx, _dy, _drawdc, 0, 0, SRCCOPY );
}
#endif

/// draws buffer content to another buffer doing color conversion if necessary
void LVColorDrawBuf::DrawTo( LVDrawBuf * buf, int x, int y, int options, lUInt32 * palette )
{
	CR_UNUSED(options);
	CR_UNUSED(palette);
	//
	lvRect clip;
	buf->GetClipRect(&clip);
	int bpp = buf->GetBitsPerPixel();
	for (int yy=0; yy<_dy; yy++) {
		if (y+yy >= clip.top && y+yy < clip.bottom) {
			if ( _bpp==16 ) {
				lUInt16 * src = (lUInt16 *)GetScanLine(yy);
				if (bpp == 1) {
					int shift = x & 7;
					lUInt8 * dst = buf->GetScanLine(y+yy) + (x>>3);
					for (int xx=0; xx<_dx; xx++) {
						if (x + xx >= clip.left && x + xx < clip.right) {
							//lUInt8 mask = ~((lUInt8)0xC0>>shift);
#if (GRAY_INVERSE==1)
							lUInt8 cl = (((lUInt8)(*src)&0x8000)^0x8000) >> (shift+8);
#else
							lUInt8 cl = (((lUInt8)(*src)&0x8000)) >> (shift+8);
#endif
							*dst |= cl;
						}
						if (!((shift = (shift + 1) & 7)))
							dst++;
						src++;
					}
				} else if (bpp == 2) {
					int shift = x & 3;
					lUInt8 * dst = buf->GetScanLine(y+yy) + (x>>2);
					for (int xx=0; xx < _dx; xx++) {
						if ( x+xx >= clip.left && x+xx < clip.right ) {
							//lUInt8 mask = ~((lUInt8)0xC0>>shift);
#if (GRAY_INVERSE==1)
							lUInt8 cl = (((lUInt8)(*src)&0xC000)^0xC000) >> ((shift<<1) + 8);
#else
							lUInt8 cl = (((lUInt8)(*src)&0xC000)) >> ((shift<<1) + 8);
#endif
							*dst |= cl;
						}
						if (!((shift = ((shift + 1) & 3))))
							dst++;
						src++;
					}
				} else if (bpp<=8) {
					lUInt8 * dst = buf->GetScanLine(y+yy) + x;
					for (int xx=0; xx<_dx; xx++) {
						if ( x+xx >= clip.left && x+xx < clip.right ) {
							//lUInt8 mask = ~((lUInt8)0xC0>>shift);
							*dst = (lUInt8)(*src >> 8);
						}
						dst++;
						src++;
					}
				} else if (bpp == 16) {
					lUInt16 * dst = ((lUInt16 *)buf->GetScanLine(y + yy)) + x;
					for (int xx=0; xx < _dx; xx++) {
						if (x + xx >= clip.left && x + xx < clip.right) {
							*dst = *src;
						}
						dst++;
						src++;
					}
				} else if (bpp == 32) {
					lUInt32 * dst = ((lUInt32 *)buf->GetScanLine(y + yy)) + x;
					for (int xx=0; xx<_dx; xx++) {
						if ( x+xx >= clip.left && x+xx < clip.right ) {
							*dst = rgb565to888( *src );
						}
						dst++;
						src++;
					}
				}
			} else {
				lUInt32 * src = (lUInt32 *)GetScanLine(yy);
				if (bpp==1) {
					int shift = x & 7;
					lUInt8 * dst = buf->GetScanLine(y+yy) + (x>>3);
					for (int xx=0; xx<_dx; xx++) {
						if ( x+xx >= clip.left && x+xx < clip.right ) {
							//lUInt8 mask = ~((lUInt8)0xC0>>shift);
#if (GRAY_INVERSE==1)
							lUInt8 cl = (((lUInt8)(*src)&0x80)^0x80) >> (shift);
#else
							lUInt8 cl = (((lUInt8)(*src)&0x80)) >> (shift);
#endif
							*dst |= cl;
						}
						if (!((shift = (shift + 1) & 7)))
							dst++;
						src++;
					}
				} else if (bpp==2) {
					int shift = x & 3;
					lUInt8 * dst = buf->GetScanLine(y+yy) + (x>>2);
					for (int xx=0; xx<_dx; xx++) {
						if ( x+xx >= clip.left && x+xx < clip.right ) {
							//lUInt8 mask = ~((lUInt8)0xC0>>shift);
#if (GRAY_INVERSE==1)
							lUInt8 cl = (((lUInt8)(*src)&0xC0)^0xC0) >> (shift<<1);
#else
							lUInt8 cl = (((lUInt8)(*src)&0xC0)) >> (shift<<1);
#endif
							*dst |= cl;
						}
						if (!((shift = (shift + 1) & 3)))
							dst++;
						src++;
					}
				} else if (bpp<=8) {
					lUInt8 * dst = buf->GetScanLine(y + yy) + x;
					for (int xx=0; xx<_dx; xx++) {
						if (x + xx >= clip.left && x + xx < clip.right) {
							//lUInt8 mask = ~((lUInt8)0xC0>>shift);
							*dst = (lUInt8)*src;
						}
						dst++;
						src++;
					}
				} else if (bpp == 32) {
					lUInt32 * dst = ((lUInt32 *)buf->GetScanLine(y + yy)) + x;
					for (int xx = 0; xx < _dx; xx++) {
						if (x+xx >= clip.left && x + xx < clip.right) {
							*dst = *src;
						}
						dst++;
						src++;
					}
				}
			}
		}
	}
}

/// draws rescaled buffer content to another buffer doing color conversion if necessary
void LVColorDrawBuf::DrawRescaled(LVDrawBuf * src, int x, int y, int dx, int dy, int options)
{
	CR_UNUSED(options);
	if (dx < 1 || dy < 1)
		return;
	lvRect clip;
	GetClipRect(&clip);
	int srcdx = src->GetWidth();
	int srcdy = src->GetHeight();
	bool linearInterpolation = (srcdx <= dx || srcdy <= dy);
	for (int yy=0; yy<dy; yy++) {
		if (y+yy >= clip.top && y+yy < clip.bottom)	{
			if (linearInterpolation) {
				// linear interpolation
				int srcy16 = srcdy * yy * 16 / dy;
				for (int xx=0; xx<dx; xx++)	{
					if ( x+xx >= clip.left && x+xx < clip.right ) {
						int srcx16 = srcdx * xx * 16 / dx;
						lUInt32 cl = src->GetInterpolatedColor(srcx16, srcy16);
						if (_bpp == 16) {
							lUInt16 * dst = (lUInt16 *)GetScanLine(y + yy);
							dst[x + xx] = rgb888to565(cl);
						} else {
							lUInt32 * dst = (lUInt32 *)GetScanLine(y + yy);
							dst[x + xx] = cl;
						}
					}
				}
			} else {
				// area average
				lvRect srcRect;
				srcRect.top = srcdy * yy * 16 / dy;
				srcRect.bottom = srcdy * (yy + 1) * 16 / dy;
				for (int xx=0; xx<dx; xx++)	{
					if ( x+xx >= clip.left && x+xx < clip.right ) {
						srcRect.left = srcdx * xx * 16 / dx;
						srcRect.right = srcdx * (xx + 1) * 16 / dx;
						lUInt32 cl = src->GetAvgColor(srcRect);
						if (_bpp == 16) {
							lUInt16 * dst = (lUInt16 *)GetScanLine(y + yy);
							dst[x + xx] = rgb888to565(cl);
						} else {
							lUInt32 * dst = (lUInt32 *)GetScanLine(y + yy);
							dst[x + xx] = cl;
						}
					}
				}
			}
		}
	}
}

/// returns scanline pointer
lUInt8 * LVColorDrawBuf::GetScanLine( int y )
{
	if (!_data || y<0 || y>=_dy)
		return NULL;
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
	return _data + _rowsize * (_dy-1-y);
#else
	return _data + _rowsize * y;
#endif
}

/// returns white pixel value
lUInt32 LVColorDrawBuf::GetWhiteColor()
{
	return 0xFFFFFF;
}
/// returns black pixel value
lUInt32 LVColorDrawBuf::GetBlackColor()
{
	return 0x000000;
}


/// constructor
LVColorDrawBuf::LVColorDrawBuf(int dx, int dy, int bpp)
:     LVBaseDrawBuf()
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
	,_drawdc(NULL)
	,_drawbmp(NULL)
#endif
	,_bpp(bpp)
	,_ownData(true)
{
	_rowsize = dx*(_bpp>>3);
	Resize( dx, dy );
}

/// creates wrapper around external RGBA buffer
LVColorDrawBuf::LVColorDrawBuf(int dx, int dy, lUInt8 * externalBuffer, int bpp )
:     LVBaseDrawBuf()
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
	,_drawdc(NULL)
	,_drawbmp(NULL)
#endif
	,_bpp(bpp)
	,_ownData(false)
{
	_dx = dx;
	_dy = dy;
	_rowsize = dx*(_bpp>>3);
	_data = externalBuffer;
	SetClipRect( NULL );
}

/// destructor
LVColorDrawBuf::~LVColorDrawBuf()
{
	if ( !_ownData )
		return;
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
	if (_drawdc)
		DeleteDC(_drawdc);
	if (_drawbmp)
		DeleteObject(_drawbmp);
#else
	if (_data)
		free( _data );
#endif
}

/// convert to 1-bit bitmap
void LVColorDrawBuf::ConvertToBitmap(bool flgDither)
{
	// not implemented
	CR_UNUSED(flgDither);
}
