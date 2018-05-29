/** \file lvrendpageinfo.cpp
	\brief rendered page splitting info implementation

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#include "../include/lvrendpageinfo.h"


bool LVRendPageInfo::serialize( SerialBuf & buf )
{
	if ( buf.error() )
		return false;
	buf << (lUInt32)start; /// start of page
	buf << (lUInt16)height; /// height of page, does not include footnotes
	buf << (lUInt8) type;   /// type: PAGE_TYPE_NORMAL, PAGE_TYPE_COVER
	lUInt16 len = footnotes.length();
	buf << len;
	for ( int i=0; i<len; i++ ) {
		buf << (lUInt32)footnotes[i].start;
		buf << (lUInt32)footnotes[i].height;
	}
	return !buf.error();
}

bool LVRendPageInfo::deserialize( SerialBuf & buf )
{
	if ( buf.error() )
		return false;
	lUInt32 n1;
	lUInt16 n2;
	lUInt8 n3;

	buf >> n1 >> n2 >> n3; /// start of page

	start = n1;
	height = n2;
	type = (PageType)n3;

	lUInt16 len;
	buf >> len;
	footnotes.clear();
	if ( len ) {
		footnotes.reserve(len);
		for ( int i=0; i<len; i++ ) {
			lUInt32 n1;
			lUInt32 n2;
			buf >> n1;
			buf >> n2;
			footnotes.add( LVPageFootNoteInfo( n1, n2 ) );
		}
	}
	return !buf.error();
}

