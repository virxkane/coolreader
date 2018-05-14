/** \file crfilehistrecord.cpp
    \brief file history and bookmarks container

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2007
    This source code is distributed under the terms of
    GNU General Public License
    See LICENSE file for details
*/

#include "../include/crfilehistrecord.h"
#include "../include/lvtinydom.h"
#include "../include/crlog.h"

#include <time.h>

CRBookmark * CRFileHistRecord::setShortcutBookmark( int shortcut, ldomXPointer ptr )
{
    if ( ptr.isNull() )
        return NULL;
    CRBookmark * bmk = new CRBookmark( ptr );
    bmk->setType( bmkt_pos );
    bmk->setShortcut( shortcut );
    for ( int i=0; i<_bookmarks.length(); i++ ) {
        if ( _bookmarks[i]->getShortcut() == shortcut ) {
            _bookmarks[i] = bmk;
            return bmk;
        }
    }
    _bookmarks.insert( 0, bmk );
    return bmk;
}

CRBookmark * CRFileHistRecord::getShortcutBookmark( int shortcut )
{
    for ( int i=0; i<_bookmarks.length(); i++ ) {
        if ( _bookmarks[i]->getShortcut() == shortcut && _bookmarks[i]->getType() == bmkt_pos )
            return _bookmarks[i];
    }
    return NULL;
}

#define MAX_SHORTCUT_BOOKMARKS 64

/// returns first available placeholder for new bookmark, -1 if no more space
int CRFileHistRecord::getLastShortcutBookmark()
{
    int last = -1;
    for ( int i=0; i<_bookmarks.length(); i++ ) {
        if ( _bookmarks[i]->getShortcut()>0 && _bookmarks[i]->getShortcut() > last && _bookmarks[i]->getShortcut() < MAX_SHORTCUT_BOOKMARKS
                && _bookmarks[i]->getType() == bmkt_pos )
            last = _bookmarks[i]->getShortcut();
    }
    return last;
}

/// returns first available placeholder for new bookmark, -1 if no more space
int CRFileHistRecord::getFirstFreeShortcutBookmark()
{
    //int last = -1;
    char flags[MAX_SHORTCUT_BOOKMARKS+1];
    memset( flags, 0, sizeof(flags) );
    for ( int i=0; i<_bookmarks.length(); i++ ) {
        if ( _bookmarks[i]->getShortcut()>0 && _bookmarks[i]->getShortcut() < MAX_SHORTCUT_BOOKMARKS && _bookmarks[i]->getType() == bmkt_pos )
            flags[ _bookmarks[i]->getShortcut() ] = 1;
    }
    for ( int j=1; j<MAX_SHORTCUT_BOOKMARKS; j++ ) {
        if ( flags[j]==0 )
            return j;
    }
    return -1;
}

void CRFileHistRecord::setLastPos( CRBookmark * bmk )
{
    _lastpos = *bmk;
}

lString16 CRFileHistRecord::getLastTimeString( bool longFormat )
{

    time_t t = getLastTime();
    tm * bt = localtime(&t);
    char str[20];
    if ( !longFormat )
        sprintf(str, "%02d.%02d.%04d", bt->tm_mday, 1+bt->tm_mon, 1900+bt->tm_year );
    else
        sprintf(str, "%02d.%02d.%04d %02d:%02d", bt->tm_mday, 1+bt->tm_mon, 1900+bt->tm_year, bt->tm_hour, bt->tm_min);
    return Utf8ToUnicode( lString8( str ) );
}
