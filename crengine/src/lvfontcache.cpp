/** \file lvfontcache.cpp
	\brief font cache implementation

	CoolReader Engine


	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#include "../include/lvfontcache.h"
#include "../include/lvstring8collection.h"
#include "../include/crlog.h"
#include "../include/lvstyles.h"		// splitPropertyValueList()

#include <stdio.h>

int LVFontDef::CalcDuplicateMatch( const LVFontDef & def ) const
{
	if (def._documentId != -1 && _documentId != def._documentId)
		return false;
	bool size_match = (_size==-1 || def._size==-1) ? true
		: (def._size == _size);
	bool weight_match = (_weight==-1 || def._weight==-1) ? true
		: (def._weight == _weight);
	bool italic_match = (_italic == def._italic || _italic==-1 || def._italic==-1);
	bool family_match = (_family==css_ff_inherit || def._family==css_ff_inherit || def._family == _family);
	bool typeface_match = (_typeface == def._typeface);
	return size_match && weight_match && italic_match && family_match && typeface_match;
}

int LVFontDef::CalcMatch( const LVFontDef & def ) const
{
	if (_documentId != -1 && _documentId != def._documentId)
		return 0;
	int size_match = (_size==-1 || def._size==-1) ? 256
		: (def._size>_size ? _size*256/def._size : def._size*256/_size );
	int weight_diff = def._weight - _weight;
	if ( weight_diff<0 )
		weight_diff = -weight_diff;
	if ( weight_diff > 800 )
		weight_diff = 800;
	int weight_match = (_weight==-1 || def._weight==-1) ? 256
		: ( 256 - weight_diff * 256 / 800 );
	int italic_match = (_italic == def._italic || _italic==-1 || def._italic==-1) ? 256 : 0;
	if ( (_italic==2 || def._italic==2) && _italic>0 && def._italic>0 )
		italic_match = 128;
	int family_match = (_family==css_ff_inherit || def._family==css_ff_inherit || def._family == _family)
		? 256
		: ( (_family==css_ff_monospace)==(def._family==css_ff_monospace) ? 64 : 0 );
	int typeface_match = (_typeface == def._typeface) ? 256 : 0;
	return
		+ (size_match     * 100)
		+ (weight_match   * 5)
		+ (italic_match   * 5)
		+ (family_match   * 100)
		+ (typeface_match * 1000);
}

int LVFontDef::CalcFallbackMatch( lString8 face, int size ) const
{
	if (_typeface != face) {
		//CRLog::trace("'%s'' != '%s'", face.c_str(), _typeface.c_str());
		return 0;
	}
	int size_match = (_size==-1 || size==-1 || _size==size) ? 256 : 0;
	int weight_match = (_weight==-1) ? 256 : ( 256 - _weight * 256 / 800 );
	int italic_match = _italic == 0 ? 256 : 0;
	return
		+ (size_match     * 100)
		+ (weight_match   * 5)
		+ (italic_match   * 5);
}





LVFontCacheItem * LVFontCache::findDuplicate( const LVFontDef * def )
{
	for (int i=0; i<_registered_list.length(); i++)
	{
		if ( _registered_list[i]->_def.CalcDuplicateMatch( *def ) )
			return _registered_list[i];
	}
	return NULL;
}

LVFontCacheItem * LVFontCache::findDocumentFontDuplicate(int documentId, lString8 name)
{
	for (int i=0; i<_registered_list.length(); i++) {
		if (_registered_list[i]->_def.getDocumentId() == documentId && _registered_list[i]->_def.getName() == name)
			return _registered_list[i];
	}
	return NULL;
}

lUInt32 LVFontCache::GetFontListHash(int documentId) {
	lUInt32 hash = 0;
	for ( int i=0; i<_registered_list.length(); i++ ) {
		int doc = _registered_list[i]->getDef()->getDocumentId();
		if (doc == -1 || doc == documentId) // skip document fonts
			hash += _registered_list[i]->getDef()->getHash();
	}
	return hash;
}

void LVFontCache::getFaceList(lString16Collection &list)
{
	list.clear();
	for ( int i=0; i<_registered_list.length(); i++ ) {
		if (_registered_list[i]->getDef()->getDocumentId() != -1)
			continue;
		lString16 name = Utf8ToUnicode( _registered_list[i]->getDef()->getTypeFace() );
		if ( !list.contains(name) )
			list.add( name );
	}
	list.sort();
}

void LVFontCache::clearFallbackFonts()
{
	for ( int i=0; i<_registered_list.length(); i++ ) {
		_registered_list[i]->getFont()->setFallbackFont(LVFontRef());
	}
}

LVFontCacheItem * LVFontCache::findFallback( lString8 face, int size )
{
	int best_index = -1;
	int best_match = -1;
	int best_instance_index = -1;
	int best_instance_match = -1;
	int i;
	for (i=0; i<_instance_list.length(); i++)
	{
		int match = _instance_list[i]->_def.CalcFallbackMatch( face, size );
		if (match > best_instance_match)
		{
			best_instance_match = match;
			best_instance_index = i;
		}
	}
	for (i=0; i<_registered_list.length(); i++)
	{
		int match = _registered_list[i]->_def.CalcFallbackMatch( face, size );
		if (match > best_match)
		{
			best_match = match;
			best_index = i;
		}
	}
	if (best_index<=0)
		return NULL;
	if (best_instance_match >= best_match)
		return _instance_list[best_instance_index];
	return _registered_list[best_index];
}

LVFontCacheItem * LVFontCache::find( const LVFontDef * fntdef )
{
	int best_index = -1;
	int best_match = -1;
	int best_instance_index = -1;
	int best_instance_match = -1;
	int i;
	LVFontDef def(*fntdef);
	lString8Collection list;
	splitPropertyValueList( fntdef->getTypeFace().c_str(), list );
	for (int nindex=0; nindex==0 || nindex<list.length(); nindex++)
	{
		if ( nindex<list.length() )
			def.setTypeFace( list[nindex] );
		else
			def.setTypeFace(lString8::empty_str);
		for (i=0; i<_instance_list.length(); i++)
		{
			int match = _instance_list[i]->_def.CalcMatch( def );
			if (match > best_instance_match)
			{
				best_instance_match = match;
				best_instance_index = i;
			}
		}
		for (i=0; i<_registered_list.length(); i++)
		{
			int match = _registered_list[i]->_def.CalcMatch( def );
			if (match > best_match)
			{
				best_match = match;
				best_index = i;
			}
		}
	}
	if (best_index<0)
		return NULL;
	if (best_instance_match >= best_match)
		return _instance_list[best_instance_index];
	return _registered_list[best_index];
}

void LVFontCache::addInstance( const LVFontDef * def, LVFontRef ref )
{
	if ( ref.isNull() )
		printf("Adding null font instance!");
	LVFontCacheItem * item = new LVFontCacheItem(*def);
	item->_fnt = ref;
	_instance_list.add( item );
}

void LVFontCache::removefont(const LVFontDef * def)
{
	int i;
		for (i=0; i<_instance_list.length(); i++)
		{
			if ( _instance_list[i]->_def.getTypeFace() == def->getTypeFace() )
			{
				_instance_list.remove(i);
			}

		}
		for (i=0; i<_registered_list.length(); i++)
		{
			if ( _registered_list[i]->_def.getTypeFace() == def->getTypeFace() )
			{
				_registered_list.remove(i);
			}
		}

}
void LVFontCache::update( const LVFontDef * def, LVFontRef ref )
{
	int i;
	if ( !ref.isNull() ) {
		for (i=0; i<_instance_list.length(); i++)
		{
			if ( _instance_list[i]->_def == *def )
			{
				if (ref.isNull())
				{
					_instance_list.erase(i, 1);
				}
				else
				{
					_instance_list[i]->_fnt = ref;
				}
				return;
			}
		}
		// add new
		//LVFontCacheItem * item;
		//item = new LVFontCacheItem(*def);
		addInstance( def, ref );
	} else {
		for (i=0; i<_registered_list.length(); i++)
		{
			if ( _registered_list[i]->_def == *def )
			{
				return;
			}
		}
		// add new
		LVFontCacheItem * item;
		item = new LVFontCacheItem(*def);
		_registered_list.add( item );
	}
}

void LVFontCache::removeDocumentFonts(int documentId)
{
	int i;
	for (i=_instance_list.length()-1; i>=0; i--) {
		if (_instance_list[i]->_def.getDocumentId() == documentId)
			delete _instance_list.remove(i);
	}
	for (i=_registered_list.length()-1; i>=0; i--) {
		if (_registered_list[i]->_def.getDocumentId() == documentId)
			delete _registered_list.remove(i);
	}
}

// garbage collector
void LVFontCache::gc()
{
	int droppedCount = 0;
	int usedCount = 0;
	for (int i=_instance_list.length()-1; i>=0; i--)
	{
		if ( _instance_list[i]->_fnt.getRefCount()<=1 )
		{
			if ( CRLog::isTraceEnabled() )
				CRLog::trace("dropping font instance %s[%d] by gc()", _instance_list[i]->getDef()->getTypeFace().c_str(), _instance_list[i]->getDef()->getSize() );
			_instance_list.erase(i,1);
			droppedCount++;
		} else {
			usedCount++;
		}
	}
	if ( CRLog::isDebugEnabled() )
		CRLog::debug("LVFontCache::gc() : %d fonts still used, %d fonts dropped", usedCount, droppedCount );
}
