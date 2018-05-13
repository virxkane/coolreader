/** \file lvcrc32.h
	\brief calculates CRC32 for buffer contents

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#ifndef __LV_CRC32_H_INCLUDED__
#define __LV_CRC32_H_INCLUDED__

#include "lvtypes.h"

/// calculates CRC32 for buffer contents
lUInt32 lv_crc32( lUInt32 prevValue, const void * buf, int size );

#endif	// __LV_CRC32_H_INCLUDED__
