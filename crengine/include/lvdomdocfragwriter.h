/** \file lvdomdocfragwriter.h
	\brief fast and compact XML DOM tree: ldomDocumentFragmentWriter

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2009
	This source code is distributed under the terms of
	GNU General Public License
	See LICENSE file for details
*/


#ifndef __LV_DOMDOCFRAGWRITER_H_INCLUDED__
#define __LV_DOMDOCFRAGWRITER_H_INCLUDED__

#include "crsetup.h"
#include "lvxml.h"
#include "lvstring16collection.h"
#include "lvhashtable.h"

class ldomNode;

class ldomDocumentFragmentWriter : public LVXMLParserCallback
{
private:
	//============================
	LVXMLParserCallback * parent;
	lString16 baseTag;
	lString16 baseTagReplacement;
	lString16 codeBase;
	lString16 filePathName;
	lString16 codeBasePrefix;
	lString16 stylesheetFile;
	lString16 tmpStylesheetFile;
	lString16Collection stylesheetLinks;
	bool insideTag;
	int styleDetectionState;
	LVHashTable<lString16, lString16> pathSubstitutions;

	ldomNode * baseElement;
	ldomNode * lastBaseElement;

	lString8 headStyleText;
	int headStyleState;

public:

	/// return content of html/head/style element
	lString8 getHeadStyleText() { return headStyleText; }

	ldomNode * getBaseElement() { return lastBaseElement; }

	lString16 convertId( lString16 id );
	lString16 convertHref( lString16 href );

	void addPathSubstitution( lString16 key, lString16 value )
	{
		pathSubstitutions.set(key, value);
	}

	virtual void setCodeBase( lString16 filePath );
	/// returns flags
	virtual lUInt32 getFlags() { return parent->getFlags(); }
	/// sets flags
	virtual void setFlags( lUInt32 flags ) { parent->setFlags(flags); }
	// overrides
	/// called when encoding directive found in document
	virtual void OnEncoding( const lChar16 * name, const lChar16 * table )
	{ parent->OnEncoding( name, table ); }
	/// called on parsing start
	virtual void OnStart(LVFileFormatParser *)
	{
		insideTag = false;
		headStyleText.clear();
		headStyleState = 0;
	}
	/// called on parsing end
	virtual void OnStop()
	{
		if ( insideTag ) {
			insideTag = false;
			if ( !baseTagReplacement.empty() ) {
				parent->OnTagClose(L"", baseTagReplacement.c_str());
			}
			baseElement = NULL;
			return;
		}
		insideTag = false;
	}
	/// called on opening tag
	virtual ldomNode * OnTagOpen( const lChar16 * nsname, const lChar16 * tagname );
	/// called after > of opening tag (when entering tag body)
	virtual void OnTagBody();
	/// called on closing tag
	virtual void OnTagClose( const lChar16 * nsname, const lChar16 * tagname );
	/// called on attribute
	virtual void OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue );
	/// called on text
	virtual void OnText( const lChar16 * text, int len, lUInt32 flags )
	{
		if (headStyleState == 1) {
			headStyleText << UnicodeToUtf8(lString16(text));
			return;
		}
		if ( insideTag )
			parent->OnText( text, len, flags );
	}
	/// add named BLOB data to document
	virtual bool OnBlob(lString16 name, const lUInt8 * data, int size) { return parent->OnBlob(name, data, size); }
	/// set document property
	virtual void OnDocProperty(const char * name, lString8 value) { parent->OnDocProperty(name, value); }
	/// constructor
	ldomDocumentFragmentWriter( LVXMLParserCallback * parentWriter, lString16 baseTagName, lString16 baseTagReplacementName, lString16 fragmentFilePath )
	: parent(parentWriter), baseTag(baseTagName), baseTagReplacement(baseTagReplacementName),
	insideTag(false), styleDetectionState(0), pathSubstitutions(100), baseElement(NULL), lastBaseElement(NULL), headStyleState(0)
	{
		setCodeBase( fragmentFilePath );
	}
	/// destructor
	virtual ~ldomDocumentFragmentWriter() { }
};

#endif	// __LV_DOMDOCFRAGWRITER_H_INCLUDED__
