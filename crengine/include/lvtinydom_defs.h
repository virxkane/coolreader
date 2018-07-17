/** \file lvtinydom_defs.h
	\brief fast and compact XML DOM tree
	Some definitions and macros

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2009
	This source code is distributed under the terms of
	GNU General Public License
	See LICENSE file for details
*/

#ifndef __LV_TINYDOM_DEFS_H_INCLUDED__
#define __LV_TINYDOM_DEFS_H_INCLUDED__

/// increment following value to force re-formatting of old book after load
#define FORMATTING_VERSION_ID 0x0003

#ifndef DOC_DATA_COMPRESSION_LEVEL
/// data compression level (0=no compression, 1=fast compressions, 3=normal compression)
#define DOC_DATA_COMPRESSION_LEVEL 1 // 0, 1, 3 (0=no compression)
#endif

//=====================================================
// Document data caching parameters
//=====================================================

#ifndef DOC_BUFFER_SIZE
#define DOC_BUFFER_SIZE 0xA00000 // default buffer size
#endif

//--------------------------------------------------------
// cache memory sizes
//--------------------------------------------------------
#ifndef ENABLED_BLOCK_WRITE_CACHE
#define ENABLED_BLOCK_WRITE_CACHE 1
#endif

#define WRITE_CACHE_TOTAL_SIZE    (10*DOC_BUFFER_SIZE/100)

#define TEXT_CACHE_UNPACKED_SPACE (25*DOC_BUFFER_SIZE/100)
#define TEXT_CACHE_CHUNK_SIZE     0x008000 // 32K
#define ELEM_CACHE_UNPACKED_SPACE (45*DOC_BUFFER_SIZE/100)
#define ELEM_CACHE_CHUNK_SIZE     0x004000 // 16K
#define RECT_CACHE_UNPACKED_SPACE (15*DOC_BUFFER_SIZE/100)
#define RECT_CACHE_CHUNK_SIZE     0x008000 // 32K
#define STYLE_CACHE_UNPACKED_SPACE (10*DOC_BUFFER_SIZE/100)
#define STYLE_CACHE_CHUNK_SIZE    0x00C000 // 48K
//--------------------------------------------------------

#define COMPRESS_NODE_DATA          true
#define COMPRESS_NODE_STORAGE_DATA  true
#define COMPRESS_MISC_DATA          true
#define COMPRESS_PAGES_DATA         true
#define COMPRESS_TOC_DATA           true
#define COMPRESS_STYLE_DATA         true

/// set t 1 to log storage reads/writes
#define DEBUG_DOM_STORAGE 0
//#define TRACE_AUTOBOX

#define RECT_DATA_CHUNK_ITEMS_SHIFT 11
#define STYLE_DATA_CHUNK_ITEMS_SHIFT 12

// calculated parameters
#define WRITE_CACHE_BLOCK_SIZE 0x4000
#define WRITE_CACHE_BLOCK_COUNT (WRITE_CACHE_TOTAL_SIZE/WRITE_CACHE_BLOCK_SIZE)
#define TEST_BLOCK_STREAM 0

#define RECT_DATA_CHUNK_ITEMS (1<<RECT_DATA_CHUNK_ITEMS_SHIFT)
#define RECT_DATA_CHUNK_SIZE (RECT_DATA_CHUNK_ITEMS*sizeof(lvdomElementFormatRec))
#define RECT_DATA_CHUNK_MASK (RECT_DATA_CHUNK_ITEMS-1)

#define STYLE_DATA_CHUNK_ITEMS (1<<STYLE_DATA_CHUNK_ITEMS_SHIFT)
#define STYLE_DATA_CHUNK_SIZE (STYLE_DATA_CHUNK_ITEMS*sizeof(ldomNodeStyleInfo))
#define STYLE_DATA_CHUNK_MASK (STYLE_DATA_CHUNK_ITEMS-1)


#define STYLE_HASH_TABLE_SIZE     512
#define FONT_HASH_TABLE_SIZE      256

#endif	// __LV_TINYDOM_DEFS_H_INCLUDED__
