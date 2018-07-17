/** \file lvdomdocwriterfilter.h
	\brief fast and compact XML DOM tree: ldomDocumentWriterFilter

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2009
	This source code is distributed under the terms of
	GNU General Public License
	See LICENSE file for details
*/


#ifndef __LV_DOMDOCWRITERFILTER_H_INCLUDED__
#define __LV_DOMDOCWRITERFILTER_H_INCLUDED__

#include "crsetup.h"
#include "lvdomdocwriter.h"

class ldomNode;
class ldomDocument;

#define MAX_ELEMENT_TYPE_ID   1024

#define UNKNOWN_ELEMENT_TYPE_ID   (MAX_ELEMENT_TYPE_ID>>1)

/** \brief callback object to fill DOM tree

	To be used with XML parser as callback object.

	Creates document according to incoming events.

	Autoclose HTML tags.
*/
class ldomDocumentWriterFilter : public ldomDocumentWriter
{
protected:
	bool _libRuDocumentDetected;
	bool _libRuParagraphStart;
	lUInt16 _styleAttrId;
	lUInt16 _classAttrId;
	lUInt16 * _rules[MAX_ELEMENT_TYPE_ID];
	bool _tagBodyCalled;
	virtual void AutoClose( lUInt16 tag_id, bool open );
	virtual void ElementCloseHandler( ldomNode * elem );
	virtual void appendStyle( const lChar16 * style );
	virtual void setClass( const lChar16 * className, bool overrideExisting=false );
public:
	/// called on attribute
	virtual void OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue );
	/// called on opening tag
	virtual ldomNode * OnTagOpen( const lChar16 * nsname, const lChar16 * tagname );
	/// called after > of opening tag (when entering tag body)
	virtual void OnTagBody();
	/// called on closing tag
	virtual void OnTagClose( const lChar16 * nsname, const lChar16 * tagname );
	/// called on text
	virtual void OnText( const lChar16 * text, int len, lUInt32 flags );
	/// constructor
	ldomDocumentWriterFilter(ldomDocument * document, bool headerOnly, const char *** rules);
	/// destructor
	virtual ~ldomDocumentWriterFilter();
};

#endif	// __LV_DOMDOCWRITERFILTER_H_INCLUDED__
