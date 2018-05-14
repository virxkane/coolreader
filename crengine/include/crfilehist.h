/** \file crfilehist.h
	\brief file history and bookmarks container

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2007
	This source code is distributed under the terms of
	GNU General Public License
	See LICENSE file for details
*/

#ifndef CRFILEHIST_H_INCLUDED
#define CRFILEHIST_H_INCLUDED

#include "lvptrvec.h"
#include "lvtinydom.h"

class CRFileHistRecord;

class CRFileHist {
private:
	LVPtrVector<CRFileHistRecord> _records;
	int findEntry( const lString16 & fname, const lString16 & fpath, lvsize_t sz );
	void makeTop( int index );
public:
	void limit( int maxItems )
	{
		for ( int i=_records.length()-1; i>maxItems; i-- ) {
			_records.erase( i, 1 );
		}
	}
	LVPtrVector<CRFileHistRecord> & getRecords() { return _records; }
	bool loadFromStream( LVStreamRef stream );
	bool saveToStream( LVStream * stream );
	CRFileHistRecord * savePosition( lString16 fpathname, size_t sz,
		const lString16 & title,
		const lString16 & author,
		const lString16 & series,
		ldomXPointer ptr );
	ldomXPointer restorePosition(  ldomDocument * doc, lString16 fpathname, size_t sz );
	CRFileHist()
	{
	}
	~CRFileHist()
	{
		clear();
	}
	void clear();
};

#endif	// CRFILEHIST_H_INCLUDED
