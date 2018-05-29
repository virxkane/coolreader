/** \file lvfootnote.h

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#ifndef __LV_FOOTNOTE_H_INCLUDED__
#define __LV_FOOTNOTE_H_INCLUDED__

#include "lvrefcounter.h"
#include "lvstring.h"
#include "lvcompactarray.h"
#include "lvfastref.h"
#include "lvarray.h"

class LVRendLineInfo;

class LVFootNote : public LVRefCounter {
	lString16 id;
	LVCompactArray<LVRendLineInfo*, 2, 4> lines;
public:
	LVFootNote( lString16 noteId )
		: id(noteId)
	{
	}
	void addLine( LVRendLineInfo * line )
	{
		lines.add( line );
	}
	LVCompactArray<LVRendLineInfo*, 2, 4> & getLines() { return lines; }
	bool empty() { return lines.empty(); }
	void clear() { lines.clear(); }
};

typedef LVFastRef<LVFootNote> LVFootNoteRef;
typedef LVArray<LVFootNote*> LVFootNoteList;

#endif	// __LV_FOOTNOTE_H_INCLUDED__
