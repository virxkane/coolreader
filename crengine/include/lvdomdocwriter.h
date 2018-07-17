/** \file lvdomdocwriter.h
	\brief fast and compact XML DOM tree: ldomDocumentWriter

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2009
	This source code is distributed under the terms of
	GNU General Public License
	See LICENSE file for details
*/


#ifndef __LV_DOMDOCWRITER_H_INCLUDED__
#define __LV_DOMDOCWRITER_H_INCLUDED__

#include "crsetup.h"
#include "lvxml.h"

class ldomDocument;
class ldomElementWriter;

/** \brief callback object to fill DOM tree

	To be used with XML parser as callback object.

	Creates document according to incoming events.
*/
class ldomDocumentWriter : public LVXMLParserCallback
{
protected:
	//============================
	ldomDocument * _document;
	//ldomElement * _currNode;
	ldomElementWriter * _currNode;
	bool _errFlag;
	bool _headerOnly;
	bool _popStyleOnFinish;
	lUInt16 _stopTagId;
	//============================
	lUInt32 _flags;
	virtual void ElementCloseHandler( ldomNode * node );
public:
	/// returns flags
	virtual lUInt32 getFlags() { return _flags; }
	/// sets flags
	virtual void setFlags( lUInt32 flags ) { _flags = flags; }
	// overrides
	/// called when encoding directive found in document
	virtual void OnEncoding( const lChar16 * name, const lChar16 * table );
	/// called on parsing start
	virtual void OnStart(LVFileFormatParser * parser);
	/// called on parsing end
	virtual void OnStop();
	/// called on opening tag
	virtual ldomNode * OnTagOpen( const lChar16 * nsname, const lChar16 * tagname );
	/// called after > of opening tag (when entering tag body)
	virtual void OnTagBody();
	/// called on closing tag
	virtual void OnTagClose( const lChar16 * nsname, const lChar16 * tagname );
	/// called on attribute
	virtual void OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue );
	/// close tags
	ldomElementWriter * pop( ldomElementWriter * obj, lUInt16 id );
	/// called on text
	virtual void OnText( const lChar16 * text, int len, lUInt32 flags );
	/// add named BLOB data to document
	virtual bool OnBlob(lString16 name, const lUInt8 * data, int size);
	/// set document property
	virtual void OnDocProperty(const char * name, lString8 value);

	/// constructor
	ldomDocumentWriter(ldomDocument * document, bool headerOnly=false );
	/// destructor
	virtual ~ldomDocumentWriter();
};

#endif	// __LV_DOMDOCWRITER_H_INCLUDED__
