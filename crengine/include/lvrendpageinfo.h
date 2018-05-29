/** \file lvrendpageinfo.h
	\brief rendered page splitting info interface

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#ifndef __LV_RENDPAGEINFO_H_INCLUDED__
#define __LV_RENDPAGEINFO_H_INCLUDED__

#include "lvtypes.h"
#include "lvcompactarray.h"
#include "serialbuf.h"

/// rendered page splitting info
class LVRendPageInfo {
public:
	enum PageType {
		PAGE_TYPE_NORMAL = 0,
		PAGE_TYPE_COVER = 1
	};
	/// footnote fragment inside page
	class LVPageFootNoteInfo {
	public:
		int start;
		int height;
		LVPageFootNoteInfo()
		: start(0), height(0)
		{ }
		LVPageFootNoteInfo( int s, int h )
		: start(s), height(h)
		{ }
	};
public:
	int start; /// start of page
	int index;  /// index of page
	lInt16 height; /// height of page, does not include footnotes
	PageType type;   /// type: PAGE_TYPE_NORMAL, PAGE_TYPE_COVER
	LVCompactArray<LVPageFootNoteInfo, 1, 4> footnotes; /// footnote fragment list for page
	LVRendPageInfo(int pageStart, lUInt16 pageHeight, int pageIndex)
	: start(pageStart), index(pageIndex), height(pageHeight), type(PAGE_TYPE_NORMAL) {}
	LVRendPageInfo(lUInt16 coverHeight)
	: start(0), index(0), height(coverHeight), type(PAGE_TYPE_COVER) {}
	LVRendPageInfo()
	: start(0), index(0), height(0), type(PAGE_TYPE_NORMAL) { }
	bool serialize( SerialBuf & buf );
	bool deserialize( SerialBuf & buf );
};

#endif	// __LV_RENDPAGEINFO_H_INCLUDED__
