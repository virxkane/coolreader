/** \file lvfontcache.h
	\brief font cache interface

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006

	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#ifndef __LV_FONT_CACHE_H_INCLUDED__
#define __LV_FONT_CACHE_H_INCLUDED__

#include "lvfont.h"
#include "lvptrvec.h"
#include "lvarray.h"
#include "lvstring16collection.h"

/**
	\brief Font properties definition
*/
class LVFontDef
{
private:
	int               _size;
	int               _weight;
	int               _italic;
	css_font_family_t _family;
	lString8          _typeface;
	lString8          _name;
	int               _index;
	// for document font: _documentId, _buf, _name
	int               _documentId;
	LVByteArrayRef    _buf;
public:
	LVFontDef(const lString8 & name, int size, int weight, int italic, css_font_family_t family, const lString8 & typeface, int index=-1, int documentId=-1, LVByteArrayRef buf = LVByteArrayRef())
	: _size(size)
	, _weight(weight)
	, _italic(italic)
	, _family(family)
	, _typeface(typeface)
	, _name(name)
	, _index(index)
	, _documentId(documentId)
	, _buf(buf)
	{
	}
	LVFontDef(const LVFontDef & def)
	: _size(def._size)
	, _weight(def._weight)
	, _italic(def._italic)
	, _family(def._family)
	, _typeface(def._typeface)
	, _name(def._name)
	, _index(def._index)
	, _documentId(def._documentId)
	, _buf(def._buf)
	{
	}

	/// returns true if definitions are equal
	bool operator == ( const LVFontDef & def ) const
	{
		return ( _size == def._size || _size == -1 || def._size == -1 )
			&& ( _weight == def._weight || _weight==-1 || def._weight==-1 )
			&& ( _italic == def._italic || _italic==-1 || def._italic==-1 )
			&& _family == def._family
			&& _typeface == def._typeface
			&& _name == def._name
			&& ( _index == def._index || def._index == -1 )
			&& (_documentId == def._documentId || _documentId == -1)
			;
	}

	lUInt32 getHash() {
		return ((((_size * 31) + _weight)*31  + _italic)*31 + _family)*31 + _name.getHash();
	}

	/// returns font file name
	lString8 getName() const { return _name; }
	void setName( lString8 name) {  _name = name; }
	int getIndex() const { return _index; }
	void setIndex( int index ) { _index = index; }
	int getSize() const { return _size; }
	void setSize( int size ) { _size = size; }
	int getWeight() const { return _weight; }
	void setWeight( int weight ) { _weight = weight; }
	bool getItalic() const { return _italic!=0; }
	bool isRealItalic() const { return _italic==1; }
	void setItalic( int italic ) { _italic=italic; }
	css_font_family_t getFamily() const { return _family; }
	void setFamily( css_font_family_t family ) { _family = family; }
	lString8 getTypeFace() const { return _typeface; }
	void setTypeFace(lString8 tf) { _typeface = tf; }
	int getDocumentId() const { return _documentId; }
	void setDocumentId(int id) { _documentId = id; }
	LVByteArrayRef& getBuf() { return _buf; }
	void setBuf(const LVByteArrayRef& buf) { _buf = buf; }
	~LVFontDef() {}
	/// calculates difference between two fonts
	int CalcMatch( const LVFontDef & def ) const;
	/// difference between fonts for duplicates search
	int CalcDuplicateMatch( const LVFontDef & def ) const;
	/// calc match for fallback font search
	int CalcFallbackMatch( lString8 face, int size ) const;
};

/// font cache item
class LVFontCacheItem
{
	friend class LVFontCache;
	LVFontDef _def;
	LVFontRef _fnt;
public:
	LVFontDef * getDef() { return &_def; }
	LVFontRef & getFont() { return _fnt; }
	void setFont(LVFontRef & fnt) { _fnt = fnt; }
	LVFontCacheItem( const LVFontDef & def )
	: _def( def )
	{ }
};

/// font cache
class LVFontCache
{
	LVPtrVector< LVFontCacheItem > _registered_list;
	LVPtrVector< LVFontCacheItem > _instance_list;
public:
	void clear() { _registered_list.clear(); _instance_list.clear(); }
	void gc(); // garbage collector
	void update( const LVFontDef * def, LVFontRef ref );
	void removefont(const LVFontDef * def);
	void removeDocumentFonts(int documentId);
	int  length() const { return _registered_list.length(); }
	void addInstance( const LVFontDef * def, LVFontRef ref );
	LVPtrVector< LVFontCacheItem > * getInstances() { return &_instance_list; }
	LVFontCacheItem * find( const LVFontDef * def );
	LVFontCacheItem * findFallback( lString8 face, int size );
	LVFontCacheItem * findDuplicate( const LVFontDef * def );
	LVFontCacheItem * findDocumentFontDuplicate(int documentId, lString8 name);
	/// get hash of installed fonts and fallback font
	virtual lUInt32 GetFontListHash(int documentId);
	virtual void getFaceList( lString16Collection & list );
	virtual void clearFallbackFonts();
	LVFontCache( )
	{ }
	virtual ~LVFontCache() { }
};

#endif	// __LV_FONT_CACHE_H_INCLUDED__
