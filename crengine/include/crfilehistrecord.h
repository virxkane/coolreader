/** \file crfilehistrecord.h
	\brief file history and bookmarks container

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2007
	This source code is distributed under the terms of
	GNU General Public License
	See LICENSE file for details
*/

#ifndef CRFILEHISTRECORD_H_INCLUDED
#define CRFILEHISTRECORD_H_INCLUDED

#include "lvptrvec.h"
#include "crbookmark.h"

class CRFileHistRecord {
private:
	lString16 _fname;
	lString16 _fpath;
	lString16 _title;
	lString16 _author;
	lString16 _series;
	lvpos_t   _size;
	LVPtrVector<CRBookmark> _bookmarks;
	CRBookmark _lastpos;
public:
	/// returns first available placeholder for new bookmark, -1 if no more space
	int getLastShortcutBookmark();
	/// returns first available placeholder for new bookmark, -1 if no more space
	int getFirstFreeShortcutBookmark();
	CRBookmark * setShortcutBookmark( int shortcut, ldomXPointer ptr );
	CRBookmark * getShortcutBookmark( int shortcut );
	time_t getLastTime() { return _lastpos.getTimestamp(); }
	lString16 getLastTimeString( bool longFormat=false );
	void setLastTime( time_t t ) { _lastpos.setTimestamp(t); }
	LVPtrVector<CRBookmark>  & getBookmarks() { return _bookmarks; }
	CRBookmark * getLastPos() { return &_lastpos; }
	void setLastPos( CRBookmark * bmk );
	lString16 getTitle() { return _title; }
	lString16 getAuthor() { return _author; }
	lString16 getSeries() { return _series; }
	lString16 getFileName() { return _fname; }
	lString16 getFilePath() { return _fpath; }
	lString16 getFilePathName() { return _fpath + _fname; }
	lvpos_t   getFileSize() { return _size; }
	void setTitle( const lString16 & s ) { _title = s; }
	void setAuthor( const lString16 & s ) { _author = s; }
	void setSeries( const lString16 & s ) { _series = s; }
	void setFileName( const lString16 & s ) { _fname = s; }
	void setFilePath( const lString16 & s ) { _fpath = s; }
	void setFileSize( lvsize_t sz ) { _size = sz; }
	CRFileHistRecord()
		: _size(0)
	{
	}
	CRFileHistRecord( const CRFileHistRecord & v)
		: _fname(v._fname)
		, _fpath(v._fpath)
		, _title(v._title)
		, _author(v._author)
		, _series(v._series)
		, _size(v._size)
		, _bookmarks(v._bookmarks)
		, _lastpos(v._lastpos)
	{
	}
	~CRFileHistRecord()
	{
	}
};

#endif	// CRFILEHISTRECORD_H_INCLUDED
