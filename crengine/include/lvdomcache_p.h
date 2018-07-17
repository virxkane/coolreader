/** \file lvdomcache_p.h
	\brief fast and compact XML DOM tree
	Private header.

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2009
	This source code is distributed under the terms of
	GNU General Public License
	See LICENSE file for details
*/

#ifndef __LV_DOMCACHE_P_H_INCLUDED__
#define __LV_DOMCACHE_P_H_INCLUDED__

#include "crsetup.h"
#include "lvstream.h"
#include "lvhashtable.h"

#ifndef STREAM_AUTO_SYNC_SIZE
#define STREAM_AUTO_SYNC_SIZE 300000
#endif //STREAM_AUTO_SYNC_SIZE

//#define CACHE_FILE_SECTOR_SIZE 4096
#define CACHE_FILE_SECTOR_SIZE 1024
#define CACHE_FILE_WRITE_BLOCK_PADDING 1

#define CACHE_FILE_ITEM_MAGIC 0xC007B00C

#define PACK_BUF_SIZE   0x10000
#define UNPACK_BUF_SIZE 0x40000

/// change in case of incompatible changes in swap/cache file format to avoid using incompatible swap file
// increment to force complete reload/reparsing of old file
#define CACHE_FILE_FORMAT_VERSION "3.12.53"

/// set to 1 to enable crc check of all blocks of cache file on open
#ifndef ENABLE_CACHE_FILE_CONTENTS_VALIDATION
#define ENABLE_CACHE_FILE_CONTENTS_VALIDATION 1
#endif

enum CacheFileBlockType {
	CBT_FREE = 0,
	CBT_INDEX = 1,
	CBT_TEXT_DATA,
	CBT_ELEM_DATA,
	CBT_RECT_DATA, //4
	CBT_ELEM_STYLE_DATA,
	CBT_MAPS_DATA,
	CBT_PAGE_DATA, //7
	CBT_PROP_DATA,
	CBT_NODE_INDEX,
	CBT_ELEM_NODE,
	CBT_TEXT_NODE,
	CBT_REND_PARAMS, //12
	CBT_TOC_DATA,
	CBT_STYLE_DATA,
	CBT_BLOB_INDEX, //15
	CBT_BLOB_DATA,
	CBT_FONT_DATA  //17
};

struct CacheFileItem
{
	lUInt32 _magic;    // magic number
	lUInt16 _dataType;     // data type
	lUInt16 _dataIndex;    // additional data index, for internal usage for data type
	int _blockIndex;   // sequential number of block
	int _blockFilePos; // start of block
	int _blockSize;    // size of block within file
	int _dataSize;     // used data size inside block (<= block size)
	lUInt64 _dataHash; // additional hash of data
	lUInt64 _packedHash; // additional hash of packed data
	lUInt32 _uncompressedSize;   // size of uncompressed block, if compression is applied, 0 if no compression
	bool validate( int fsize );
	CacheFileItem()
	{
	}
	CacheFileItem( lUInt16 dataType, lUInt16 dataIndex )
	: _magic(CACHE_FILE_ITEM_MAGIC)
	, _dataType(dataType)   // data type
	, _dataIndex(dataIndex) // additional data index, for internal usage for data type
	, _blockIndex(0)        // sequential number of block
	, _blockFilePos(0)      // start of block
	, _blockSize(0)         // size of block within file
	, _dataSize(0)          // used data size inside block (<= block size)
	, _dataHash(0)          // hash of data
	, _packedHash(0) // additional hash of packed data
	, _uncompressedSize(0)  // size of uncompressed block, if compression is applied, 0 if no compression
	{
	}
};

/**
 * Cache file implementation.
 */
class CacheFile
{
	int _sectorSize; // block position and size granularity
	int _size;
	bool _indexChanged;
	bool _dirty;
	LVStreamRef _stream; // file stream
	LVPtrVector<CacheFileItem, true> _index; // full file block index
	LVPtrVector<CacheFileItem, false> _freeIndex; // free file block index
	LVHashTable<lUInt32, CacheFileItem*> _map; // hash map for fast search
	// searches for existing block
	CacheFileItem * findBlock( lUInt16 type, lUInt16 index );
	// alocates block at index, reuses existing one, if possible
	CacheFileItem * allocBlock( lUInt16 type, lUInt16 index, int size );
	// mark block as free, for later reusing
	void freeBlock( CacheFileItem * block );
	// writes file header
	bool updateHeader();
	// writes index block
	bool writeIndex();
	// reads index from file
	bool readIndex();
	// reads all blocks of index and checks CRCs
	bool validateContents();
public:
	// return current file size
	int getSize() { return _size; }
	// create uninitialized cache file, call open or create to initialize
	CacheFile();
	// free resources
	~CacheFile();
	// try open existing cache file
	bool open( lString16 filename );
	// try open existing cache file from stream
	bool open( LVStreamRef stream );
	// create new cache file
	bool create( lString16 filename );
	// create new cache file in stream
	bool create( LVStreamRef stream );
	/// writes block to file
	bool write( lUInt16 type, lUInt16 dataIndex, const lUInt8 * buf, int size, bool compress );
	/// reads and allocates block in memory
	bool read( lUInt16 type, lUInt16 dataIndex, lUInt8 * &buf, int &size );
	/// reads and validates block
	bool validate( CacheFileItem * block );
	/// writes content of serial buffer
	bool write( lUInt16 type, lUInt16 index, SerialBuf & buf, bool compress );
	/// reads content of serial buffer
	bool read( lUInt16 type, lUInt16 index, SerialBuf & buf );
	/// writes content of serial buffer
	bool write( lUInt16 type, SerialBuf & buf, bool compress )
	{
		return write( type, 0, buf, compress);
	}
	/// reads content of serial buffer
	bool read( lUInt16 type, SerialBuf & buf )
	{
		return read(type, 0, buf);
	}
	/// reads block as a stream
	LVStreamRef readStream(lUInt16 type, lUInt16 index);

	/// sets dirty flag value, returns true if value is changed
	bool setDirtyFlag( bool dirty );
	// flushes index
	bool flush( bool clearDirtyFlag, CRTimerUtil & maxTime );
	int roundSector( int n )
	{
		return (n + (_sectorSize-1)) & ~(_sectorSize-1);
	}
	void setAutoSyncSize(int sz) {
		_stream->setAutoSyncSize(sz);
	}
};

#endif	// __LV_DOMCACHE_P_H_INCLUDED__
