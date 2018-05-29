/** \file lvrendpagecontext.h
	\brief page splitter interface

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#ifndef __LV_RENDPAGECONTEXT_H_INCLUDED__
#define __LV_RENDPAGECONTEXT_H_INCLUDED__

#include "lvstring.h"
#include "lvhashtable.h"
#include "lvptrvec.h"
#include "lvfootnote.h"
#include "lvrendlineinfo.h"

#ifndef RENDER_PROGRESS_INTERVAL_MILLIS
#define RENDER_PROGRESS_INTERVAL_MILLIS 1200
#endif
#ifndef RENDER_PROGRESS_INTERVAL_PERCENT
#define RENDER_PROGRESS_INTERVAL_PERCENT 2
#endif

class LVRendPageList;
class LVDocViewCallback;
class LVRendPageContext
{
	LVPtrVector<LVRendLineInfo> lines;

	LVDocViewCallback * callback;
	int totalFinalBlocks;
	int renderedFinalBlocks;
	int lastPercent;
	CRTimerUtil progressTimeout;


	// page start line
	//LVRendLineInfoBase pagestart;
	// page end candidate line
	//LVRendLineInfoBase pageend;
	// next line after page end candidate
	//LVRendLineInfoBase next;
	// last fit line
	//LVRendLineInfoBase last;
	// page list to fill
	LVRendPageList * page_list;
	// page height
	int          page_h;

	LVHashTable<lString16, LVFootNoteRef> footNotes;

	LVFootNote * curr_note;

	LVFootNote * getOrCreateFootNote( lString16 id )
	{
		LVFootNoteRef ref = footNotes.get(id);
		if ( ref.isNull() ) {
			ref = LVFootNoteRef( new LVFootNote( id ) );
			footNotes.set( id, ref );
		}
		return ref.get();
	}

	void split();
public:


	void setCallback(LVDocViewCallback * cb, int _totalFinalBlocks) {
		callback = cb; totalFinalBlocks=_totalFinalBlocks;
		progressTimeout.restart(RENDER_PROGRESS_INTERVAL_MILLIS);
	}
	bool updateRenderProgress( int numFinalBlocksRendered );

	/// append footnote link to last added line
	void addLink( lString16 id );

	/// mark start of foot note
	void enterFootNote( lString16 id );

	/// mark end of foot note
	void leaveFootNote();

	/// returns page height
	int getPageHeight() { return page_h; }

	/// returns page list pointer
	LVRendPageList * getPageList() { return page_list; }

	LVRendPageContext(LVRendPageList * pageList, int pageHeight);

	/// add source line
	void AddLine( int starty, int endy, int flags );

	void Finalize();
};

#endif	// __LV_RENDPAGECONTEXT_H_INCLUDED__
