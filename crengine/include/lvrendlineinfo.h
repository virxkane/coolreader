/** \file lvrendlineinfo.h

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#ifndef __LV_RENDLINEINFO_H_INCLUDED__
#define __LV_RENDLINEINFO_H_INCLUDED__

#include "lvtypes.h"
#include "lvarray.h"


/// &7 values
#define RN_SPLIT_AUTO   0
#define RN_SPLIT_AVOID  1
#define RN_SPLIT_ALWAYS 2
/// right-shift
#define RN_SPLIT_BEFORE 0
#define RN_SPLIT_AFTER  3

#define RN_SPLIT_BEFORE_AUTO   (RN_SPLIT_AUTO<<RN_SPLIT_BEFORE)
#define RN_SPLIT_BEFORE_AVOID  (RN_SPLIT_AVOID<<RN_SPLIT_BEFORE)
#define RN_SPLIT_BEFORE_ALWAYS (RN_SPLIT_ALWAYS<<RN_SPLIT_BEFORE)
#define RN_SPLIT_AFTER_AUTO    (RN_SPLIT_AUTO<<RN_SPLIT_AFTER)
#define RN_SPLIT_AFTER_AVOID   (RN_SPLIT_AVOID<<RN_SPLIT_AFTER)
#define RN_SPLIT_AFTER_ALWAYS  (RN_SPLIT_ALWAYS<<RN_SPLIT_AFTER)

#define RN_SPLIT_FOOT_NOTE 0x100
#define RN_SPLIT_FOOT_LINK 0x200


class LVFootNote;
typedef LVArray<LVFootNote*> LVFootNoteList;

class LVRendLineInfo {
	friend struct PageSplitState;
	LVFootNoteList * links; // 4 bytes
	int start;              // 4 bytes
	lInt16 height;          // 2 bytes
public:
	lInt16 flags;           // 2 bytes
	int getSplitBefore() const { return (flags>>RN_SPLIT_BEFORE)&7; }
	int getSplitAfter() const { return (flags>>RN_SPLIT_AFTER)&7; }
/*
	LVRendLineInfo & operator = ( const LVRendLineInfoBase & v )
	{
		start = v.start;
		end = v.end;
		flags = v.flags;
		return *this;
	}
*/
	bool empty() const {
		return start==-1;
	}

	void clear();

	inline int getEnd() const { return start + height; }
	inline int getStart() const { return start; }
	inline int getHeight() const { return height; }

	LVRendLineInfo() : links(NULL), start(-1), height(0), flags(0) { }
	LVRendLineInfo( int line_start, int line_end, lUInt16 line_flags )
	: links(NULL), start(line_start), height((lUInt16)(line_end-line_start)), flags(line_flags)
	{
	}
	LVFootNoteList * getLinks() { return links; }
	~LVRendLineInfo()
	{
		clear();
	}
	void addLink( LVFootNote * note );
};

#endif	// __LV_RENDLINEINFO_H_INCLUDED__
