/** \file lvrendpagelist.cpp

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#include "../include/lvrendpagelist.h"
#include "../include/lvtinydom.h"
#include "../include/crlog.h"


int LVRendPageList::FindNearestPage( int y, int direction )
{
	if (!length())
		return 0;
	for (int i=0; i<length(); i++)
	{
		const LVRendPageInfo * pi = get(i);
		if (y<pi->start) {
			if (i==0 || direction>=0)
				return i;
			else
				return i-1;
		} else if (y<pi->start+pi->height) {
			if (i<length()-1 && direction>0)
				return i+1;
			else if (i==0 || direction>=0)
				return i;
			else
				return i-1;
		}
	}
	return length()-1;
}


static const char * pagelist_magic = "PageList";

bool LVRendPageList::serialize( SerialBuf & buf )
{
	if ( buf.error() )
		return false;
	buf.putMagic( pagelist_magic );
	int pos = buf.pos();
	buf << (lUInt32)length();
	for ( int i=0; i<length(); i++ ) {
		get(i)->serialize( buf );
	}
	buf.putMagic( pagelist_magic );
	buf.putCRC( buf.pos() - pos );
	return !buf.error();
}

bool LVRendPageList::deserialize( SerialBuf & buf )
{
	if ( buf.error() )
		return false;
	if ( !buf.checkMagic( pagelist_magic ) )
		return false;
	clear();
	int pos = buf.pos();
	lUInt32 len;
	buf >> len;
	clear();
	reserve(len);
	for (lUInt32 i = 0; i < len; i++) {
		LVRendPageInfo * item = new LVRendPageInfo();
		item->deserialize( buf );
		item->index = i;
		add( item );
	}
	if ( !buf.checkMagic( pagelist_magic ) )
		return false;
	buf.checkCRC( buf.pos() - pos );
	return !buf.error();
}
