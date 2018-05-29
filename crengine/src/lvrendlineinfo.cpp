/** \file lvrendlineinfo.cpp

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#include "../include/lvrendlineinfo.h"
#include "../include/lvfootnote.h"


void LVRendLineInfo::clear() {
	start = -1; height = 0; flags = 0;
	if ( links!=NULL ) {
		delete links;
		links=NULL;
	}
}

void LVRendLineInfo::addLink(LVFootNote *note)
{
	if ( links==NULL )
		links = new LVFootNoteList();
	links->add( note );
	flags |= RN_SPLIT_FOOT_LINK;
}
