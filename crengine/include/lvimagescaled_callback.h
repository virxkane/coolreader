/** \file lvimagescaled_callback.h

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#ifndef __LVIMAGESCALED_CALLBACK_H_INCLUDED__
#define __LVIMAGESCALED_CALLBACK_H_INCLUDED__

#include "lvimg.h"
#include "lvbasedrawbuf.h"

class LVImageScaledDrawCallback : public LVImageDecoderCallback
{
private:
	LVImageSourceRef src;
	LVBaseDrawBuf * dst;
	int dst_x;
	int dst_y;
	int dst_dx;
	int dst_dy;
	int src_dx;
	int src_dy;
	int * xmap;
	int * ymap;
	bool dither;
	bool isNinePatch;
public:
	static int * GenMap( int src_len, int dst_len )
	{
		int  * map = new int[ dst_len ];
		for (int i=0; i<dst_len; i++)
		{
			map[ i ] = i * src_len / dst_len;
		}
		return map;
	}
	static int * GenNinePatchMap( int src_len, int dst_len, int frame1, int frame2)
	{
		int  * map = new int[ dst_len ];
		if (frame1 + frame2 > dst_len) {
			int total = frame1 + frame2;
			int extra = total - dst_len;
			int extra1 = frame1 * extra / total;
			int extra2 = frame2 * extra / total;
			frame1 -= extra1;
			frame2 -= extra2;
		}
		int srcm = src_len - frame1 - frame2 - 2;
		int dstm = dst_len - frame1 - frame2;
		if (srcm < 0)
			srcm = 0;
		for (int i=0; i<dst_len; i++)
		{
			if (i < frame1) {
				// start
				map[ i ] = i + 1;
			} else if (i >= dst_len - frame2) {
				// end
				int rx = i - (dst_len - frame2);
				map[ i ] = src_len - frame2 + rx - 1;
			} else {
				// middle
				map[ i ] = 1 + frame1 + (i - frame1) * srcm / dstm;
			}
//            CRLog::trace("frame[%d, %d] src=%d dst=%d %d -> %d", frame1, frame2, src_len, dst_len, i, map[i]);
//            if (map[i] >= src_len) {
//                CRLog::error("Wrong coords");
//            }
		}
		return map;
	}
	LVImageScaledDrawCallback(LVBaseDrawBuf * dstbuf, LVImageSourceRef img, int x, int y, int width, int height, bool dith )
	: src(img), dst(dstbuf), dst_x(x), dst_y(y), dst_dx(width), dst_dy(height), xmap(0), ymap(0), dither(dith)
	{
		src_dx = img->GetWidth();
		src_dy = img->GetHeight();
		const CR9PatchInfo * np = img->GetNinePatchInfo();
		isNinePatch = false;
		lvRect ninePatch;
		if (np) {
			isNinePatch = true;
			ninePatch = np->frame;
		}
		if ( src_dx != dst_dx || isNinePatch) {
			if (isNinePatch)
				xmap = GenNinePatchMap(src_dx, dst_dx, ninePatch.left, ninePatch.right);
			else
				xmap = GenMap( src_dx, dst_dx );
		}
		if ( src_dy != dst_dy || isNinePatch) {
			if (isNinePatch)
				ymap = GenNinePatchMap(src_dy, dst_dy, ninePatch.top, ninePatch.bottom);
			else
				ymap = GenMap( src_dy, dst_dy );
		}
	}
	virtual ~LVImageScaledDrawCallback()
	{
		if (xmap)
			delete[] xmap;
		if (ymap)
			delete[] ymap;
	}
	virtual void OnStartDecode( LVImageSource * )
	{
	}
	virtual bool OnLineDecoded( LVImageSource *, int y, lUInt32 * data )
	{
		//fprintf( stderr, "l_%d ", y );
		if (isNinePatch) {
			if (y == 0 || y == src_dy-1) // ignore first and last lines
				return true;
		}
		int yy = -1;
		int yy2 = -1;
		if (ymap) {
			for (int i = 0; i < dst_dy; i++) {
				if (ymap[i] == y) {
					if (yy == -1)
						yy = i;
					yy2 = i + 1;
				}
			}
			if (yy == -1)
				return true;
		} else {
			yy = y;
			yy2 = y+1;
		}
//        if ( ymap )
//        {
//            int yy0 = (y - 1) * dst_dy / src_dy;
//            yy = y * dst_dy / src_dy;
//            yy2 = (y+1) * dst_dy / src_dy;
//            if ( yy == yy0 )
//            {
//                //fprintf( stderr, "skip_dup " );
//                //return true; // skip duplicate drawing
//            }
//            if ( yy2 > dst_dy )
//                yy2 = dst_dy;
//        }
		lvRect clip;
		dst->GetClipRect( &clip );
		for ( ;yy<yy2; yy++ )
		{
			if ( yy+dst_y<clip.top || yy+dst_y>=clip.bottom )
				continue;
			int bpp = dst->GetBitsPerPixel();
			if ( bpp >= 24 )
			{
				lUInt32 * row = (lUInt32 *)dst->GetScanLine( yy + dst_y );
				row += dst_x;
				for (int x=0; x<dst_dx; x++)
				{
					lUInt32 cl = data[xmap ? xmap[x] : x];
					int xx = x + dst_x;
					lUInt32 alpha = (cl >> 24)&0xFF;
					if ( xx<clip.left || xx>=clip.right || alpha==0xFF )
						continue;
					if ( !alpha )
						row[ x ] = cl;
					else {
						if ((row[x] & 0xFF000000) == 0xFF000000)
							row[ x ] = cl; // copy as is if buffer pixel is transparent
						else
							ApplyAlphaRGB( row[x], cl, alpha );
					}
				}
			}
			else if ( bpp == 16 )
			{
				lUInt16 * row = (lUInt16 *)dst->GetScanLine( yy + dst_y );
				row += dst_x;
				for (int x=0; x<dst_dx; x++)
				{
					lUInt32 cl = data[xmap ? xmap[x] : x];
					int xx = x + dst_x;
					lUInt32 alpha = (cl >> 24)&0xFF;
					if ( xx<clip.left || xx>=clip.right || alpha==0xFF )
						continue;
					if ( alpha<16 ) {
						row[ x ] = rgb888to565( cl );
					} else if (alpha<0xF0) {
						lUInt32 v = rgb565to888(row[x]);
						ApplyAlphaRGB( v, cl, alpha );
						row[x] = rgb888to565(v);
					}
				}
			}
			else if ( bpp > 2 ) // 3,4,8 bpp
			{
				lUInt8 * row = (lUInt8 *)dst->GetScanLine( yy + dst_y );
				row += dst_x;
				for (int x=0; x<dst_dx; x++)
				{
					int srcx = xmap ? xmap[x] : x;
					lUInt32 cl = data[srcx];
					int xx = x + dst_x;
					lUInt32 alpha = (cl >> 24)&0xFF;
					if ( xx<clip.left || xx>=clip.right || alpha==0xFF )
						continue;
					if ( alpha ) {
						lUInt32 origColor = row[x];
						if ( bpp==3 ) {
							origColor = origColor & 0xE0;
							origColor = origColor | (origColor>>3) | (origColor>>6);
						} else {
							origColor = origColor & 0xF0;
							origColor = origColor | (origColor>>4);
						}
						origColor = origColor | (origColor<<8) | (origColor<<16);
						ApplyAlphaRGB( origColor, cl, alpha );
						cl = origColor;
					}

					lUInt8 dcl;
					if ( dither && bpp < 8) {
#if (GRAY_INVERSE==1)
						dcl = (lUInt8)DitherNBitColor( cl^0xFFFFFF, x, yy, bpp );
#else
						dcl = (lUInt8)DitherNBitColor( cl, x, yy, bpp );
#endif
					} else {
						dcl = rgbToGray( cl, bpp );
					}
					row[ x ] = dcl;
					// ApplyAlphaGray( row[x], dcl, alpha, bpp );
				}
			}
			else if ( bpp == 2 )
			{
				//fprintf( stderr, "." );
				lUInt8 * row = (lUInt8 *)dst->GetScanLine( yy+dst_y );
				//row += dst_x;
				for (int x=0; x<dst_dx; x++)
				{
					lUInt32 cl = data[xmap ? xmap[x] : x];
					int xx = x + dst_x;
					lUInt32 alpha = (cl >> 24)&0xFF;
					if ( xx<clip.left || xx>=clip.right || alpha==0xFF )
						continue;

					int byteindex = (xx >> 2);
					int bitindex = (3-(xx & 3))<<1;
					lUInt8 mask = 0xC0 >> (6 - bitindex);

					if ( alpha ) {
						lUInt32 origColor = (row[ byteindex ] & mask)>>bitindex;
						origColor = origColor | (origColor<<2);
						origColor = origColor | (origColor<<4);
						origColor = origColor | (origColor<<8) | (origColor<<16);
						ApplyAlphaRGB( origColor, cl, alpha );
						cl = origColor;
					}

					lUInt32 dcl = 0;
					if ( dither ) {
#if (GRAY_INVERSE==1)
						dcl = Dither2BitColor( cl, x, yy ) ^ 3;
#else
						dcl = Dither2BitColor( cl, x, yy );
#endif
					} else {
						dcl = rgbToGrayMask( cl, 2 ) & 3;
					}
					dcl = dcl << bitindex;
					row[ byteindex ] = (lUInt8)((row[ byteindex ] & (~mask)) | dcl);
				}
			}
			else if ( bpp == 1 )
			{
				//fprintf( stderr, "." );
				lUInt8 * row = (lUInt8 *)dst->GetScanLine( yy+dst_y );
				//row += dst_x;
				for (int x=0; x<dst_dx; x++)
				{
					lUInt32 cl = data[xmap ? xmap[x] : x];
					int xx = x + dst_x;
					lUInt32 alpha = (cl >> 24)&0xFF;
					if ( xx<clip.left || xx>=clip.right || (alpha&0x80) )
						continue;
					lUInt32 dcl = 0;
					if ( dither ) {
#if (GRAY_INVERSE==1)
						dcl = Dither1BitColor( cl, x, yy ) ^ 1;
#else
						dcl = Dither1BitColor( cl, x, yy ) ^ 0;
#endif
					} else {
						dcl = rgbToGrayMask( cl, 1 ) & 1;
					}
					int byteindex = (xx >> 3);
					int bitindex = ((xx & 7));
					lUInt8 mask = 0x80 >> (bitindex);
					dcl = dcl << (7-bitindex);
					row[ byteindex ] = (lUInt8)((row[ byteindex ] & (~mask)) | dcl);
				}
			}
			else
			{
				return false;
			}
		}
		return true;
	}
	virtual void OnEndDecode( LVImageSource *, bool )
	{
	}
};

#endif	// __LVIMAGESCALED_CALLBACK_H_INCLUDED__
