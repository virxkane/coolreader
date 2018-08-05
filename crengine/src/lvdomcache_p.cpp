/*******************************************************

   CoolReader Engine

   lvdomcache_p.cpp: fast and compact XML DOM tree.
   Private class.

   (c) Vadim Lopatin, 2000-2011
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvdomcache_p.h"
#include "../include/lvautoptr.h"
#include "../include/serialbuf.h"
#include "../include/crlog.h"

#include <zlib.h>

// TODO: remove duplication
static bool _enableCacheFileContentsValidation = (bool)ENABLE_CACHE_FILE_CONTENTS_VALIDATION;

#define CACHE_FILE_MAGIC_SIZE 40
static const char CACHE_FILE_MAGIC[] = "CoolReader 3 Cache"
                                       " File v" CACHE_FILE_FORMAT_VERSION ": "
                                       "c0"
#if DOC_DATA_COMPRESSION_LEVEL==0
                                       "m0"
#else
                                       "m1"
#endif
                                        "\n";

#define LASSERT(x) \
    if (!(x)) crFatalError(1111, "assertion failed: " #x)

struct SimpleCacheFileHeader
{
    char _magic[CACHE_FILE_MAGIC_SIZE]; // magic
    lUInt32 _dirty;
    SimpleCacheFileHeader( lUInt32 dirtyFlag ) {
        memset( _magic, 0, sizeof(_magic));
        memcpy( _magic, CACHE_FILE_MAGIC, CACHE_FILE_MAGIC_SIZE );
        _dirty = dirtyFlag;
    }
};

struct CacheFileHeader : public SimpleCacheFileHeader
{
    lUInt32 _fsize;
    CacheFileItem _indexBlock; // index array block parameters,
    // duplicate of one of index records which contains
    bool validate()
    {
        if (memcmp(_magic, CACHE_FILE_MAGIC, CACHE_FILE_MAGIC_SIZE) != 0) {
            CRLog::error("CacheFileHeader::validate: magic doesn't match");
            return false;
        }
        if ( _dirty!=0 ) {
            CRLog::error("CacheFileHeader::validate: dirty flag is set");
            return false;
        }
        return true;
    }
    CacheFileHeader( CacheFileItem * indexRec, int fsize, lUInt32 dirtyFlag )
    : SimpleCacheFileHeader(dirtyFlag), _indexBlock(0,0)
    {
        if ( indexRec ) {
            memcpy( &_indexBlock, indexRec, sizeof(CacheFileItem));
        } else
            memset( &_indexBlock, 0, sizeof(CacheFileItem));
        _fsize = fsize;
    }
};

#if BUILD_LITE!=1

// FNV 64bit hash function
// from http://isthe.com/chongo/tech/comp/fnv/#gcc-O3

#define NO_FNV_GCC_OPTIMIZATION
#define FNV_64_PRIME ((lUInt64)0x100000001b3ULL)
static lUInt64 calcHash64( const lUInt8 * s, int len )
{
    const lUInt8 * endp = s + len;
    // 64 bit FNV hash function
    lUInt64 hval = 14695981039346656037ULL;
    for ( ; s<endp; s++ ) {
#if defined(NO_FNV_GCC_OPTIMIZATION)
        hval *= FNV_64_PRIME;
#else /* NO_FNV_GCC_OPTIMIZATION */
        hval += (hval << 1) + (hval << 4) + (hval << 5) +
            (hval << 7) + (hval << 8) + (hval << 40);
#endif /* NO_FNV_GCC_OPTIMIZATION */
        hval ^= *s;
    }
    return hval;
}

#endif

/// pack data from _buf to _compbuf
bool ldomPack( const lUInt8 * buf, int bufsize, lUInt8 * &dstbuf, lUInt32 & dstsize )
{
    lUInt8 tmp[PACK_BUF_SIZE]; // 64K buffer for compressed data
    int ret;
    z_stream z;
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.opaque = Z_NULL;
    ret = deflateInit( &z, DOC_DATA_COMPRESSION_LEVEL );
    if ( ret != Z_OK )
        return false;
    z.avail_in = bufsize;
    z.next_in = (unsigned char *)buf;
    z.avail_out = PACK_BUF_SIZE;
    z.next_out = tmp;
    ret = deflate( &z, Z_FINISH );
    int have = PACK_BUF_SIZE - z.avail_out;
    deflateEnd(&z);
    if ( ret!=Z_STREAM_END || have==0 || have>=PACK_BUF_SIZE || z.avail_in!=0 ) {
        // some error occured while packing, leave unpacked
        //setpacked( buf, bufsize );
        return false;
    }
    dstsize = have;
    dstbuf = (lUInt8 *)malloc(have);
    memcpy( dstbuf, tmp, have );
    return true;
}

/// unpack data from _compbuf to _buf
bool ldomUnpack( const lUInt8 * compbuf, int compsize, lUInt8 * &dstbuf, lUInt32 & dstsize  )
{
    lUInt8 tmp[UNPACK_BUF_SIZE]; // 64K buffer for compressed data
    int ret;
    z_stream z;
    memset( &z, 0, sizeof(z) );
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.opaque = Z_NULL;
    ret = inflateInit( &z );
    if ( ret != Z_OK )
        return false;
    z.avail_in = compsize;
    z.next_in = (unsigned char *)compbuf;
    z.avail_out = UNPACK_BUF_SIZE;
    z.next_out = tmp;
    ret = inflate( &z, Z_FINISH );
    int have = UNPACK_BUF_SIZE - z.avail_out;
    inflateEnd(&z);
    if ( ret!=Z_STREAM_END || have==0 || have>=UNPACK_BUF_SIZE || z.avail_in!=0 ) {
        // some error occured while unpacking
        return false;
    }
    dstsize = have;
    dstbuf = (lUInt8 *)malloc(have);
    memcpy( dstbuf, tmp, have );
    return true;
}




// create uninitialized cache file, call open or create to initialize
CacheFile::CacheFile()
: _sectorSize( CACHE_FILE_SECTOR_SIZE ), _size(0), _indexChanged(false), _dirty(true), _map(1024)
{
}

// free resources
CacheFile::~CacheFile()
{
	if ( !_stream.isNull() ) {
		// don't flush -- leave file dirty
		//CRTimerUtil infinite;
		//flush( true, infinite );
	}
}

/// sets dirty flag value, returns true if value is changed
bool CacheFile::setDirtyFlag( bool dirty )
{
	if ( _dirty==dirty )
		return false;
	if ( !dirty ) {
		CRLog::info("CacheFile::clearing Dirty flag");
		_stream->Flush(true);
	} else {
		CRLog::info("CacheFile::setting Dirty flag");
	}
	_dirty = dirty;
	SimpleCacheFileHeader hdr(_dirty?1:0);
	_stream->SetPos(0);
	lvsize_t bytesWritten = 0;
	_stream->Write(&hdr, sizeof(hdr), &bytesWritten );
	if ( bytesWritten!=sizeof(hdr) )
		return false;
	_stream->Flush(true);
	//CRLog::trace("setDirtyFlag : hdr is saved with Dirty flag = %d", hdr._dirty);
	return true;
}

// flushes index
bool CacheFile::flush( bool clearDirtyFlag, CRTimerUtil & maxTime )
{
	if ( clearDirtyFlag ) {
		//setDirtyFlag(true);
		if ( !writeIndex() )
			return false;
		setDirtyFlag(false);
	} else {
		_stream->Flush(false, maxTime);
		//CRLog::trace("CacheFile->flush() took %d ms ", (int)timer.elapsed());
	}
	return true;
}

// reads all blocks of index and checks CRCs
bool CacheFile::validateContents()
{
	CRLog::info("Started validation of cache file contents");
	LVHashTable<lUInt32, CacheFileItem*>::pair * pair;
	for ( LVHashTable<lUInt32, CacheFileItem*>::iterator p = _map.forwardIterator(); (pair=p.next())!=NULL; ) {
		if ( pair->value->_dataType==CBT_INDEX )
			continue;
		if ( !validate(pair->value) ) {
			CRLog::error("Contents validation is failed for block type=%d index=%d", (int)pair->value->_dataType, pair->value->_dataIndex );
			return false;
		}
	}
	CRLog::info("Finished validation of cache file contents -- successful");
	return true;
}

// reads index from file
bool CacheFile::readIndex()
{
	CacheFileHeader hdr(NULL, _size, 0);
	_stream->SetPos(0);
	lvsize_t bytesRead = 0;
	_stream->Read(&hdr, sizeof(hdr), &bytesRead );
	if ( bytesRead!=sizeof(hdr) )
		return false;
	CRLog::info("Header read: DirtyFlag=%d", hdr._dirty);
	if ( !hdr.validate() )
		return false;
	if ( (int)hdr._fsize > _size + 4096-1 ) {
		CRLog::error("CacheFile::readIndex: file size doesn't match with header");
		return false;
	}
	if ( !hdr._indexBlock._blockFilePos )
		return true; // empty index is ok
	if ( hdr._indexBlock._blockFilePos>=(int)hdr._fsize || hdr._indexBlock._blockFilePos+hdr._indexBlock._blockSize>(int)hdr._fsize+4096-1 ) {
		CRLog::error("CacheFile::readIndex: Wrong index file position specified in header");
		return false;
	}
	if ((int)_stream->SetPos(hdr._indexBlock._blockFilePos)!=hdr._indexBlock._blockFilePos ) {
		CRLog::error("CacheFile::readIndex: cannot move file position to index block");
		return false;
	}
	int count = hdr._indexBlock._dataSize / sizeof(CacheFileItem);
	if ( count<0 || count>100000 ) {
		CRLog::error("CacheFile::readIndex: invalid number of blocks in index");
		return false;
	}
	CacheFileItem * index = new CacheFileItem[count];
	bytesRead = 0;
	lvsize_t  sz = sizeof(CacheFileItem)*count;
	_stream->Read(index, sz, &bytesRead );
	if ( bytesRead!=sz )
		return false;
	// check CRC
	lUInt64 hash = calcHash64( (lUInt8*)index, sz );
	if ( hdr._indexBlock._dataHash!=hash ) {
		CRLog::error("CacheFile::readIndex: CRC doesn't match found %08x expected %08x", hash, hdr._indexBlock._dataHash);
		delete[] index;
		return false;
	}
	for ( int i=0; i<count; i++ ) {
		if (index[i]._dataType == CBT_INDEX)
			index[i] = hdr._indexBlock;
		if ( !index[i].validate(_size) ) {
			delete[] index;
			return false;
		}
		CacheFileItem * item = new CacheFileItem();
		memcpy(item, &index[i], sizeof(CacheFileItem));
		_index.add( item );
		lUInt32 key = ((lUInt32)item->_dataType)<<16 | item->_dataIndex;
		if ( key==0 )
			_freeIndex.add( item );
		else
			_map.set( key, item );
	}
	delete[] index;
	CacheFileItem * indexitem = findBlock(CBT_INDEX, 0);
	if ( !indexitem ) {
		CRLog::error("CacheFile::readIndex: index block info doesn't match header");
		return false;
	}
	_dirty = hdr._dirty ? true : false;
	return true;
}

// writes index block
bool CacheFile::writeIndex()
{
	if ( !_indexChanged )
		return true; // no changes: no writes

	if ( _index.length()==0 )
		return updateHeader();

	// create copy of index in memory
	int count = _index.length();
	CacheFileItem * indexItem = findBlock(CBT_INDEX, 0);
	if (!indexItem) {
		int sz = sizeof(CacheFileItem) * (count * 2 + 100);
		allocBlock(CBT_INDEX, 0, sz);
		indexItem = findBlock(CBT_INDEX, 0);
		count = _index.length();
	}
	CacheFileItem * index = new CacheFileItem[count];
	int sz = count * sizeof(CacheFileItem);
	memset(index, 0, sz);
	for ( int i = 0; i < count; i++ ) {
		memcpy( &index[i], _index[i], sizeof(CacheFileItem) );
		if (index[i]._dataType == CBT_INDEX) {
			index[i]._dataHash = 0;
			index[i]._packedHash = 0;
			index[i]._dataSize = 0;
		}
	}
	bool res = write(CBT_INDEX, 0, (const lUInt8*)index, sz, false);
	delete[] index;

	indexItem = findBlock(CBT_INDEX, 0);
	if ( !res || !indexItem ) {
		CRLog::error("CacheFile::writeIndex: error while writing index!!!");
		return false;
	}

	updateHeader();
	_indexChanged = false;
	return true;
}

// writes file header
bool CacheFile::updateHeader()
{
	CacheFileItem * indexItem = NULL;
	indexItem = findBlock(CBT_INDEX, 0);
	CacheFileHeader hdr(indexItem, _size, _dirty?1:0);
	_stream->SetPos(0);
	lvsize_t bytesWritten = 0;
	_stream->Write(&hdr, sizeof(hdr), &bytesWritten );
	if ( bytesWritten!=sizeof(hdr) )
		return false;
	//CRLog::trace("updateHeader finished: Dirty flag = %d", hdr._dirty);
	return true;
}

//
void CacheFile::freeBlock( CacheFileItem * block )
{
	lUInt32 key = ((lUInt32)block->_dataType)<<16 | block->_dataIndex;
	_map.remove(key);
	block->_dataIndex = 0;
	block->_dataType = 0;
	block->_dataSize = 0;
	_freeIndex.add( block );
}

/// reads block as a stream
LVStreamRef CacheFile::readStream(lUInt16 type, lUInt16 index)
{
	CacheFileItem * block = findBlock(type, index);
	if (block && block->_dataSize) {
#if 0
		lUInt8 * buf = NULL;
		int size = 0;
		if (read(type, index, buf, size))
			return LVCreateMemoryStream(buf, size);
#else
		return LVStreamRef(new LVStreamFragment(_stream, block->_blockFilePos, block->_dataSize));
#endif
	}
	return LVStreamRef();
}

// searches for existing block
CacheFileItem * CacheFile::findBlock( lUInt16 type, lUInt16 index )
{
	lUInt32 key = ((lUInt32)type)<<16 | index;
	CacheFileItem * existing = _map.get( key );
	return existing;
}

// allocates index record for block, sets its new size
CacheFileItem * CacheFile::allocBlock( lUInt16 type, lUInt16 index, int size )
{
	lUInt32 key = ((lUInt32)type)<<16 | index;
	CacheFileItem * existing = _map.get( key );
	if ( existing ) {
		if ( existing->_blockSize >= size ) {
			if ( existing->_dataSize != size ) {
				existing->_dataSize = size;
				_indexChanged = true;
			}
			return existing;
		}
		// old block has not enough space: free it
		freeBlock( existing );
		existing = NULL;
	}
	// search for existing free block of proper size
	int bestSize = -1;
	//int bestIndex = -1;
	for ( int i=0; i<_freeIndex.length(); i++ ) {
		if ( _freeIndex[i] && (_freeIndex[i]->_blockSize>=size) && (bestSize==-1 || _freeIndex[i]->_blockSize<bestSize) ) {
			bestSize = _freeIndex[i]->_blockSize;
			//bestIndex = -1;
			existing = _freeIndex[i];
		}
	}
	if ( existing ) {
		_freeIndex.remove( existing );
		existing->_dataType = type;
		existing->_dataIndex = index;
		existing->_dataSize = size;
		_map.set( key, existing );
		_indexChanged = true;
		return existing;
	}
	// allocate new block
	CacheFileItem * block = new CacheFileItem( type, index );
	_map.set( key, block );
	block->_blockSize = roundSector(size);
	block->_dataSize = size;
	block->_blockIndex = _index.length();
	_index.add(block);
	block->_blockFilePos = _size;
	_size += block->_blockSize;
	_indexChanged = true;
	// really, file size is not extended
	return block;
}

/// reads and validates block
bool CacheFile::validate( CacheFileItem * block )
{
	lUInt8 * buf = NULL;
	unsigned size = 0;

	if ( (int)_stream->SetPos( block->_blockFilePos )!=block->_blockFilePos ) {
		CRLog::error("CacheFile::validate: Cannot set position for block %d:%d of size %d", block->_dataType, block->_dataIndex, (int)size);
		return false;
	}

	// read block from file
	size = block->_dataSize;
	buf = (lUInt8 *)malloc(size);
	lvsize_t bytesRead = 0;
	_stream->Read(buf, size, &bytesRead );
	if ( bytesRead!=size ) {
		CRLog::error("CacheFile::validate: Cannot read block %d:%d of size %d", block->_dataType, block->_dataIndex, (int)size);
		free(buf);
		return false;
	}

	// check CRC for file block
	lUInt64 packedhash = calcHash64( buf, size );
	if ( packedhash!=block->_packedHash ) {
		CRLog::error("CacheFile::validate: packed data CRC doesn't match for block %d:%d of size %d", block->_dataType, block->_dataIndex, (int)size);
		free(buf);
		return false;
	}
	free(buf);
	return true;
}

// reads and allocates block in memory
bool CacheFile::read( lUInt16 type, lUInt16 dataIndex, lUInt8 * &buf, int &size )
{
	buf = NULL;
	size = 0;
	CacheFileItem * block = findBlock( type, dataIndex );
	if ( !block ) {
		CRLog::error("CacheFile::read: Block %d:%d not found in file", type, dataIndex);
		return false;
	}
	if ( (int)_stream->SetPos( block->_blockFilePos )!=block->_blockFilePos )
		return false;

	// read block from file
	size = block->_dataSize;
	buf = (lUInt8 *)malloc(size);
	lvsize_t bytesRead = 0;
	_stream->Read(buf, size, &bytesRead );
	if ( (int)bytesRead!=size ) {
		CRLog::error("CacheFile::read: Cannot read block %d:%d of size %d", type, dataIndex, (int)size);
		free(buf);
		buf = NULL;
		size = 0;
		return false;
	}

	bool compress = block->_uncompressedSize!=0;

	if ( compress ) {
		// block is compressed

		// check crc separately only for compressed data
		lUInt64 packedhash = calcHash64( buf, size );
		if ( packedhash!=block->_packedHash ) {
			CRLog::error("CacheFile::read: packed data CRC doesn't match for block %d:%d of size %d", type, dataIndex, (int)size);
			free(buf);
			buf = NULL;
			size = 0;
			return false;
		}

		// uncompress block data
		lUInt8 * uncomp_buf = NULL;
		lUInt32 uncomp_size = 0;
		if ( ldomUnpack(buf, size, uncomp_buf, uncomp_size) && uncomp_size==block->_uncompressedSize ) {
			free( buf );
			buf = uncomp_buf;
			size = uncomp_size;
		} else {
			CRLog::error("CacheFile::read: error while uncompressing data for block %d:%d of size %d", type, dataIndex, (int)size);
			free(buf);
			buf = NULL;
			size = 0;
			return false;
		}
	}

	// check CRC
	lUInt64 hash = calcHash64( buf, size );
	if (hash != block->_dataHash) {
		CRLog::error("CacheFile::read: CRC doesn't match for block %d:%d of size %d", type, dataIndex, (int)size);
		free(buf);
		buf = NULL;
		size = 0;
		return false;
	}
	// Success. Don't forget to free allocated block externally
	return true;
}

// writes block to file
bool CacheFile::write( lUInt16 type, lUInt16 dataIndex, const lUInt8 * buf, int size, bool compress )
{
	// check whether data is changed
	lUInt64 newhash = calcHash64( buf, size );
	CacheFileItem * existingblock = findBlock( type, dataIndex );

	if (existingblock) {
		bool sameSize = ((int)existingblock->_uncompressedSize==size) || (existingblock->_uncompressedSize==0 && (int)existingblock->_dataSize==size);
		if (sameSize && existingblock->_dataHash == newhash ) {
			return true;
		}
	}

#if 0
	if (existingblock)
		CRLog::trace("*    oldsz=%d oldhash=%08x", (int)existingblock->_uncompressedSize, (int)existingblock->_dataHash);
	CRLog::trace("* wr block t=%d[%d] sz=%d hash=%08x", type, dataIndex, size, newhash);
#endif
	setDirtyFlag(true);

	lUInt32 uncompressedSize = 0;
	lUInt64 newpackedhash = newhash;
#if DOC_DATA_COMPRESSION_LEVEL==0
	compress = false;
#else
	if ( compress ) {
		lUInt8 * dstbuf = NULL;
		lUInt32 dstsize = 0;
		if ( !ldomPack( buf, size, dstbuf, dstsize ) ) {
			compress = false;
		} else {
			uncompressedSize = size;
			size = dstsize;
			buf = dstbuf;
			newpackedhash = calcHash64( buf, size );
#if DEBUG_DOM_STORAGE==1
			//CRLog::trace("packed block %d:%d : %d to %d bytes (%d%%)", type, dataIndex, srcsize, dstsize, srcsize>0?(100*dstsize/srcsize):0 );
#endif
		}
	}
#endif

	CacheFileItem * block = NULL;
	if ( existingblock && existingblock->_dataSize>=size ) {
		// reuse existing block
		block = existingblock;
	} else {
		// allocate new block
		if ( existingblock )
			freeBlock( existingblock );
		block = allocBlock( type, dataIndex, size );
	}
	if ( !block )
		return false;
	if ( (int)_stream->SetPos( block->_blockFilePos )!=block->_blockFilePos )
		return false;
	// assert: size == block->_dataSize
	// actual writing of data
	block->_dataSize = size;
	lvsize_t bytesWritten = 0;
	_stream->Write(buf, size, &bytesWritten );
	if ( (int)bytesWritten!=size )
		return false;
#if CACHE_FILE_WRITE_BLOCK_PADDING==1
	int paddingSize = block->_blockSize - size; //roundSector( size ) - size
	if ( paddingSize ) {
		if ((int)block->_blockFilePos + (int)block->_dataSize >= (int)_stream->GetSize() - _sectorSize) {
			LASSERT(size + paddingSize == block->_blockSize );
//            if (paddingSize > 16384) {
//                CRLog::error("paddingSize > 16384");
//            }
//            LASSERT(paddingSize <= 16384);
			lUInt8 tmp[16384];//paddingSize];
			memset(tmp, 0xFF, paddingSize < 16384 ? paddingSize : 16384);
			do {
				int blkSize = paddingSize < 16384 ? paddingSize : 16384;
				_stream->Write(tmp, blkSize, &bytesWritten );
				paddingSize -= blkSize;
			} while (paddingSize > 0);
		}
	}
#endif
	//_stream->Flush(true);
	// update CRC
	block->_dataHash = newhash;
	block->_packedHash = newpackedhash;
	block->_uncompressedSize = uncompressedSize;

#if DOC_DATA_COMPRESSION_LEVEL!=0
	if ( compress ) {
		free( (void*)buf );
	}
#endif
	_indexChanged = true;

	//CRLog::error("CacheFile::write: block %d:%d (pos %ds, size %ds) is written (crc=%08x)", type, dataIndex, (int)block->_blockFilePos/_sectorSize, (int)(size+_sectorSize-1)/_sectorSize, block->_dataCRC);
	// success
	return true;
}

/// writes content of serial buffer
bool CacheFile::write( lUInt16 type, lUInt16 index, SerialBuf & buf, bool compress )
{
	return write( type, index, buf.buf(), buf.pos(), compress );
}

/// reads content of serial buffer
bool CacheFile::read( lUInt16 type, lUInt16 index, SerialBuf & buf )
{
	lUInt8 * tmp = NULL;
	int size = 0;
	bool res = read( type, index, tmp, size );
	if ( res ) {
		buf.set( tmp, size );
	}
	buf.setPos(0);
	return res;
}

// try open existing cache file
bool CacheFile::open( lString16 filename )
{
	LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_APPEND );
	if ( !stream ) {
		CRLog::error( "CacheFile::open: cannot open file %s", LCSTR(filename));
		return false;
	}
	crSetFileToRemoveOnFatalError(LCSTR(filename));
	return open(stream);
}


// try open existing cache file
bool CacheFile::open( LVStreamRef stream )
{
	_stream = stream;
	_size = _stream->GetSize();
	//_stream->setAutoSyncSize(STREAM_AUTO_SYNC_SIZE);

	if ( !readIndex() ) {
		CRLog::error("CacheFile::open : cannot read index from file");
		return false;
	}
	if (_enableCacheFileContentsValidation && !validateContents() ) {
		CRLog::error("CacheFile::open : file contents validation failed");
		return false;
	}
	return true;
}

bool CacheFile::create( lString16 filename )
{
	LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_APPEND );
	if ( _stream.isNull() ) {
		CRLog::error( "CacheFile::create: cannot create file %s", LCSTR(filename));
		return false;
	}
	crSetFileToRemoveOnFatalError(LCSTR(filename));
	return create(stream);
}

bool CacheFile::create( LVStreamRef stream )
{
	_stream = stream;
	//_stream->setAutoSyncSize(STREAM_AUTO_SYNC_SIZE);
	if ( _stream->SetPos(0)!=0 ) {
		CRLog::error( "CacheFile::create: cannot seek file");
		_stream.Clear();
		return false;
	}

	_size = _sectorSize;
	LVArray<lUInt8> sector0(_sectorSize, 0);
	lvsize_t bytesWritten = 0;
	_stream->Write(sector0.get(), _sectorSize, &bytesWritten );
	if ( (int)bytesWritten!=_sectorSize ) {
		_stream.Clear();
		return false;
	}
	if (!updateHeader()) {
		_stream.Clear();
		return false;
	}
	return true;
}

// class CacheFileItem
bool CacheFileItem::validate(int fsize)
{
	if ( _magic!=CACHE_FILE_ITEM_MAGIC ) {
		CRLog::error("CacheFileItem::validate: block magic doesn't match");
		return false;
	}
	if ( _dataSize>_blockSize || _blockSize<0 || _dataSize<0 || _blockFilePos+_dataSize>fsize || _blockFilePos<CACHE_FILE_SECTOR_SIZE) {
		CRLog::error("CacheFileItem::validate: invalid block size or position");
		return false;
	}
	return true;
}
