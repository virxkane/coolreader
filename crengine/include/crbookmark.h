/** \file crbookmark.h
	\brief file history and bookmarks container

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2007
	This source code is distributed under the terms of
	GNU General Public License
	See LICENSE file for details
*/

#ifndef CRBOOKMARK_H_INCLUDED
#define CRBOOKMARK_H_INCLUDED

#include "lvptrvec.h"
#include "lvstring.h"
#include "lvtinydom.h"

#include <time.h>

enum bmk_type {
	bmkt_lastpos,
	bmkt_pos,
	bmkt_comment,
	bmkt_correction
};

class CRBookmark {
private:
	lString16 _startpos;
	lString16 _endpos;
	int       _percent;
	int       _type;
	int       _shortcut;
	lString16 _postext;
	lString16 _titletext;
	lString16 _commenttext;
	time_t    _timestamp;
	int       _page;
public:
	static lString16 getChapterName( ldomXPointer p );

	// fake bookmark for range
	CRBookmark(lString16 startPos,lString16 endPos)
				: _startpos(startPos)
	, _endpos(endPos)
	, _percent(0)
	, _type(0)
		, _shortcut(0)
	, _postext(lString16::empty_str)
	, _titletext(lString16::empty_str)
	, _commenttext(lString16::empty_str)
	, _timestamp(time_t(0))
	,_page(0)
	{
	}
	CRBookmark(const CRBookmark & v )
	: _startpos(v._startpos)
	, _endpos(v._endpos)
	, _percent(v._percent)
	, _type(v._type)
	, _shortcut(v._shortcut)
	, _postext(v._postext)
	, _titletext(v._titletext)
	, _commenttext(v._commenttext)
	, _timestamp(v._timestamp)
	, _page(v._page)
	{
	}
	CRBookmark & operator = (const CRBookmark & v )
	{
		_startpos = v._startpos;
		_endpos = v._endpos;
		_percent = v._percent;
		_type = v._type;
		_shortcut = v._shortcut;
		_postext = v._postext;
		_titletext = v._titletext;
		_commenttext = v._commenttext;
		_timestamp = v._timestamp;
		_page = v._page;
		return *this;
	}
	CRBookmark() : _percent(0), _type(0), _shortcut(0), _timestamp(0), _page(0) { }
	CRBookmark ( ldomXPointer ptr );
	lString16 getStartPos() { return _startpos; }
	lString16 getEndPos() { return _endpos; }
	lString16 getPosText() { return _postext; }
	lString16 getTitleText() { return _titletext; }
	lString16 getCommentText() { return _commenttext; }
	int getShortcut() { return _shortcut; }
	int getType() { return _type; }
	int getPercent() { return _percent; }
	time_t getTimestamp() { return _timestamp; }
	void setStartPos(const lString16 & s ) { _startpos = s; }
	void setEndPos(const lString16 & s ) { _endpos = s; }
	void setPosText(const lString16 & s ) { _postext= s; }
	void setTitleText(const lString16 & s ) { _titletext = s; }
	void setCommentText(const lString16 & s ) { _commenttext = s; }
	void setType( int n ) { _type = n; }
	void setShortcut( int n ) { _shortcut = n; }
	void setPercent( int n ) { _percent = n; }
	void setTimestamp( time_t t ) { _timestamp = t; }
	void setBookmarkPage( int page ) { _page = page; }
	int getBookmarkPage() { return _page; }
	bool isValid() {
		if (_type < bmkt_lastpos || _type >bmkt_correction)
			return false;
		if (_startpos.empty())
			return false;
		if ((_type == bmkt_comment || _type == bmkt_correction) && _endpos.empty())
			return false;
		return true;
	}
};

#endif	// CRBOOKMARK_H_INCLUDED
