/*******************************************************

   CoolReader Engine 

   lvdrawbuf.cpp:  Gray bitmap buffer class

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/lvdrawbuf.h"
#include "../include/crlog.h"

#define GUARD_BYTE 0xa5
#define CHECK_GUARD_BYTE \
	{ \
        if (_bpp != 1 && _bpp != 2 && _bpp !=3 && _bpp != 4 && _bpp != 8 && _bpp != 16 && _bpp != 32) crFatalError(-5, "wrong bpp"); \
        if (_ownData && _data[_rowsize * _dy] != GUARD_BYTE) crFatalError(-5, "corrupted bitmap buffer"); \
    }

lUInt32 rgbToGray( lUInt32 color )
{
    lUInt32 r = (0xFF0000 & color) >> 16;
    lUInt32 g = (0x00FF00 & color) >> 8;
    lUInt32 b = (0x0000FF & color) >> 0;
    return ((r + g + g + b)>>2) & 0xFF;
}

lUInt8 rgbToGray( lUInt32 color, int bpp )
{
    lUInt32 r = (0xFF0000 & color) >> 16;
    lUInt32 g = (0x00FF00 & color) >> 8;
    lUInt32 b = (0x0000FF & color) >> 0;
    return (lUInt8)(((r + g + g + b)>>2) & (((1<<bpp)-1)<<(8-bpp)));
}

lUInt8 rgbToGrayMask( lUInt32 color, int bpp )
{
    switch ( bpp ) {
    case DRAW_BUF_1_BPP:
        color = rgbToGray(color) >> 7;
        color = (color&1) ? 0xFF : 0x00;
        break;
    case DRAW_BUF_2_BPP:
        color = rgbToGray(color) >> 6;
        color &= 3;
        color |= (color << 2) | (color << 4) | (color << 6);
        break;
    default:
    //case DRAW_BUF_3_BPP: // 8 colors
    //case DRAW_BUF_4_BPP: // 16 colors
    //case DRAW_BUF_8_BPP: // 256 colors
        // return 8 bit as is
        color = rgbToGray(color);
        color &= ((1<<bpp)-1)<<(8-bpp);
        return (lUInt8)color;
    }
    return (lUInt8)color;
}

void ApplyAlphaRGB( lUInt32 &dst, lUInt32 src, lUInt32 alpha )
{
    if ( alpha==0 )
        dst = src;
    else if ( alpha<255 ) {
        src &= 0xFFFFFF;
        lUInt32 opaque = 256 - alpha;
        lUInt32 n1 = (((dst & 0xFF00FF) * alpha + (src & 0xFF00FF) * opaque) >> 8) & 0xFF00FF;
        lUInt32 n2 = (((dst & 0x00FF00) * alpha + (src & 0x00FF00) * opaque) >> 8) & 0x00FF00;
        dst = n1 | n2;
    }
}

void ApplyAlphaGray( lUInt8 &dst, lUInt8 src, lUInt32 alpha, int bpp )
{
    if ( alpha==0 )
        dst = src;
    else if ( alpha<255 ) {
        int mask = ((1<<bpp)-1) << (8-bpp);
        src &= mask;
        lUInt32 opaque = 256 - alpha;
        lUInt32 n1 = ((dst * alpha + src * opaque)>>8 ) & mask;
        dst = (lUInt8)n1;
    }
}

//static const short dither_2bpp_4x4[] = {
//    5, 13,  8,  16,
//    9,  1,  12,  4,
//    7, 15,  6,  14,
//    11, 3,  10,  2,
//};

static const short dither_2bpp_8x8[] = {
0, 32, 12, 44, 2, 34, 14, 46, 
48, 16, 60, 28, 50, 18, 62, 30, 
8, 40, 4, 36, 10, 42, 6, 38, 
56, 24, 52, 20, 58, 26, 54, 22, 
3, 35, 15, 47, 1, 33, 13, 45, 
51, 19, 63, 31, 49, 17, 61, 29, 
11, 43, 7, 39, 9, 41, 5, 37, 
59, 27, 55, 23, 57, 25, 53, 21, 
};

// returns byte with higher significant bits, lower bits are 0
lUInt32 DitherNBitColor( lUInt32 color, lUInt32 x, lUInt32 y, int bits )
{
    int mask = ((1<<bits)-1)<<(8-bits);
    // gray = (r + 2*g + b)>>2
    //int cl = ((((color>>16) & 255) + ((color>>(8-1)) & (255<<1)) + ((color) & 255)) >> 2) & 255;
    int cl = ((((color>>16) & 255) + ((color>>(8-1)) & (255<<1)) + ((color) & 255)) >> 2) & 255;
    int white = (1<<bits) - 1;
    int precision = white;
    if (cl<precision)
        return 0;
    else if (cl>=255-precision)
        return mask;
    //int d = dither_2bpp_4x4[(x&3) | ( (y&3) << 2 )] - 1;
    // dither = 0..63
    int d = dither_2bpp_8x8[(x&7) | ( (y&7) << 3 )] - 1;
    int shift = bits-2;
    cl = ( (cl<<shift) + d - 32 ) >> shift;
    if ( cl>255 )
        cl = 255;
    if ( cl<0 )
        cl = 0;
    return cl & mask;
}

lUInt32 Dither2BitColor( lUInt32 color, lUInt32 x, lUInt32 y )
{
    int cl = ((((color>>16) & 255) + ((color>>8) & 255) + ((color) & 255)) * (256/3)) >> 8;
    if (cl<5)
        return 0;
    else if (cl>=250)
        return 3;
    //int d = dither_2bpp_4x4[(x&3) | ( (y&3) << 2 )] - 1;
    int d = dither_2bpp_8x8[(x&7) | ( (y&7) << 3 )] - 1;

    cl = ( cl + d - 32 );
    if (cl<5)
        return 0;
    else if (cl>=250)
        return 3;
    return (cl >> 6) & 3;
}

lUInt32 Dither1BitColor( lUInt32 color, lUInt32 x, lUInt32 y )
{
    int cl = ((((color>>16) & 255) + ((color>>8) & 255) + ((color) & 255)) * (256/3)) >> 8;
    if (cl<16)
        return 0;
    else if (cl>=240)
        return 1;
    //int d = dither_2bpp_4x4[(x&3) | ( (y&3) << 2 )] - 1;
    int d = dither_2bpp_8x8[(x&7) | ( (y&7) << 3 )] - 1;

    cl = ( cl + d - 32 );
    if (cl<5)
        return 0;
    else if (cl>=250)
        return 1;
    return (cl >> 7) & 1;
}

void LVDrawBuf::RoundRect( int x0, int y0, int x1, int y1, int borderWidth, int radius, lUInt32 color, int cornerFlags )
{
    FillRect( x0 + ((cornerFlags&1)?radius:0), y0, x1-1-((cornerFlags&2)?radius:0), y0+borderWidth, color );
    FillRect( x0, y0 + ((cornerFlags&1)?radius:0), x0+borderWidth, y1-1-((cornerFlags&4)?radius:0), color );
    FillRect( x1-borderWidth, y0 + ((cornerFlags&2)?radius:0), x1, y1-((cornerFlags&8)?radius:0), color );
    FillRect( x0 + ((cornerFlags&4)?radius:0), y1-borderWidth, x1-((cornerFlags&8)?radius:0), y1, color );
    // TODO: draw rounded corners
}
