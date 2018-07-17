/** \file lvdomelwriter.h
	\brief fast and compact XML DOM tree: ldomElementWriter

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2009
	This source code is distributed under the terms of
	GNU General Public License
	See LICENSE file for details

*/


#ifndef __LV_DOMELWRITER_H_INCLUDED__
#define __LV_DOMELWRITER_H_INCLUDED__

#include "crsetup.h"
#include "lvstring.h"
#include "dtddef.h"

class ldomDocument;
class ldomNode;
class LVTocItem;

class ldomElementWriter
{
	ldomElementWriter * _parent;
	ldomDocument * _document;

	ldomNode * _element;
	LVTocItem * _tocItem;
	lString16 _path;
	const css_elem_def_props_t * _typeDef;
	bool _allowText;
	bool _isBlock;
	bool _isSection;
	bool _stylesheetIsSet;
	bool _bodyEnterCalled;
	lUInt32 _flags;
	lUInt32 getFlags();
	void updateTocItem();
	void onBodyEnter();
	void onBodyExit();
	ldomNode * getElement()
	{
		return _element;
	}
	lString16 getPath();
	void onText( const lChar16 * text, int len, lUInt32 flags );
	void addAttribute( lUInt16 nsid, lUInt16 id, const wchar_t * value );
	//lxmlElementWriter * pop( lUInt16 id );

	ldomElementWriter(ldomDocument * document, lUInt16 nsid, lUInt16 id, ldomElementWriter * parent);
	~ldomElementWriter();

	friend class ldomDocumentWriter;
	friend class ldomDocumentWriterFilter;
	//friend ldomElementWriter * pop( ldomElementWriter * obj, lUInt16 id );
	static bool IS_FIRST_BODY;
};

#endif	// __LV_DOMELWRITER_H_INCLUDED__
