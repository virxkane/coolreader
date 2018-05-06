/** \file lvstring16hashedcollection.h
	\brief string16 hashed collection class interface

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.

	Refactoring by virxkane, 2018
*/

#ifndef __LV_STRING16HASHEDCOLLECTION_H_INCLUDED__
#define __LV_STRING16HASHEDCOLLECTION_H_INCLUDED__

#include "lvstring.h"
#include "lvstring16collection.h"

/// hashed wide string collection
class lString16HashedCollection : public lString16Collection
{
private:
	int hashSize;
	struct HashPair
	{
		int index;
		HashPair * next;
		void clear() { index=-1; next=NULL; }
	};
	HashPair * hash;
	void addHashItem( int hashIndex, int storageIndex );
	void clearHash();
	void reHash( int newSize );
public:

	/// serialize to byte array (pointer will be incremented by number of bytes written)
	void serialize( SerialBuf & buf );
	/// deserialize from byte array (pointer will be incremented by number of bytes read)
	bool deserialize( SerialBuf & buf );

	lString16HashedCollection( lString16HashedCollection & v );
	lString16HashedCollection( lUInt32 hashSize );
	~lString16HashedCollection();
	int add( const lChar16 * s );
	int find( const lChar16 * s );
};

#endif	// __LV_STRING16HASHEDCOLLECTION_H_INCLUDED__
