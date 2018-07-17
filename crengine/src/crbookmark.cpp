/** \file crbookmark.cpp
    \brief file history and bookmarks container

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2007
    This source code is distributed under the terms of
    GNU General Public License
    See LICENSE file for details
*/

//#include "../include/lvtinydom.h"
#include "../include/lvdomdoc.h"
#include "../include/crbookmark.h"
#include "../include/crlog.h"

lString16 CRBookmark::getChapterName( ldomXPointer ptr )
{
    //CRLog::trace("CRBookmark::getChapterName()");
	lString16 chapter;
	int lastLevel = -1;
	bool foundAnySection = false;
    lUInt16 section_id = ptr.getNode()->getDocument()->getElementNameIndex( L"section" );
	if ( !ptr.isNull() )
	{
		ldomXPointerEx p( ptr );
		p.nextText();
		while ( !p.isNull() ) {
			if ( !p.prevElement() )
				break;
            bool foundSection = p.findElementInPath( section_id ) > 0;
            //(p.toString().pos("section") >=0 );
            foundAnySection = foundAnySection || foundSection;
            if ( !foundSection && foundAnySection )
                continue;
			lString16 nname = p.getNode()->getNodeName();
            if ( !nname.compare("title") || !nname.compare("h1") || !nname.compare("h2")  || !nname.compare("h3") ) {
				if ( lastLevel!=-1 && p.getLevel()>=lastLevel )
					continue;
				lastLevel = p.getLevel();
				if ( !chapter.empty() )
                    chapter = " / " + chapter;
				chapter = p.getText(' ') + chapter;
				if ( !p.parent() )
					break;
			}
		}
	}
	return chapter;
}

CRBookmark::CRBookmark (ldomXPointer ptr )
: _startpos(lString16::empty_str)
, _endpos(lString16::empty_str)
, _percent(0)
, _type(0)
, _shortcut(0)
, _postext(lString16::empty_str)
, _titletext(lString16::empty_str)
, _commenttext(lString16::empty_str)
, _timestamp(time_t(0))
, _page(0)
{
    //
    if ( ptr.isNull() )
        return;

    //CRLog::trace("CRBookmark::CRBookmark() started");
    lString16 path;

    //CRLog::trace("CRBookmark::CRBookmark() calling ptr.toPoint");
    lvPoint pt = ptr.toPoint();
    //CRLog::trace("CRBookmark::CRBookmark() calculating percent");
    ldomDocument * doc = ptr.getNode()->getDocument();
    int h = doc->getFullHeight();
    if ( pt.y > 0 && h > 0 ) {
        if ( pt.y < h ) {
            _percent = (int)((lInt64)pt.y * 10000 / h);
        } else {
            _percent = 10000;
        }
    }
    //CRLog::trace("CRBookmark::CRBookmark() calling getChaptername");
	setTitleText( CRBookmark::getChapterName( ptr ) );
    _startpos = ptr.toString();
    _timestamp = (time_t)time(0);
    lvPoint endpt = pt;
    endpt.y += 100;
    //CRLog::trace("CRBookmark::CRBookmark() creating xpointer for endp");
    ldomXPointer endptr = doc->createXPointer( endpt );
    //CRLog::trace("CRBookmark::CRBookmark() finished");
}
