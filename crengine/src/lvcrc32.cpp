/*******************************************************

   CoolReader Engine

   lvcrc32.cpp:  calculates CRC32 for buffer contents

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvcrc32.h"

#if (USE_ZLIB==1)
#include <zlib.h>
#endif

/// calculates CRC32 for buffer contents
lUInt32 lv_crc32( lUInt32 prevValue, const void * buf, int size )
{
#if (USE_ZLIB==1)
    return crc32( prevValue, (const lUInt8 *)buf, size );
#else
    // TODO:
    return 0;
#endif
}
