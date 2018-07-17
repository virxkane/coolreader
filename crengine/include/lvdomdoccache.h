/** \file lvdomdoccache.h
	\brief fast and compact XML DOM tree: document cache

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2009
	This source code is distributed under the terms of
	GNU General Public License
	See LICENSE file for details
*/


#ifndef __LV_DOMDOCCACHE_H_INCLUDED__
#define __LV_DOMDOCCACHE_H_INCLUDED__

#include "crsetup.h"
#include "lvstring.h"
#include "lvstream.h"

/// document cache
class ldomDocCache
{
public:
	/// open existing cache file stream
	static LVStreamRef openExisting( lString16 filename, lUInt32 crc, lUInt32 docFlags );
	/// create new cache file
	static LVStreamRef createNew( lString16 filename, lUInt32 crc, lUInt32 docFlags, lUInt32 fileSize );
	/// init document cache
	static bool init( lString16 cacheDir, lvsize_t maxSize );
	/// close document cache manager
	static bool close();
	/// delete all cache files
	static bool clear();
	/// returns true if cache is enabled (successfully initialized)
	static bool enabled();
};

#endif	// __LV_DOMDOCCACHE_H_INCLUDED__
