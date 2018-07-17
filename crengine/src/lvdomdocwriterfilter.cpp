/*******************************************************

   CoolReader Engine

   ldomdocwriterfilter.cpp: fast and compact XML DOM tree: ldomDocumentWriterFilter

   (c) Vadim Lopatin, 2000-2011
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvdomdocwriterfilter.h"
#include "../include/fb2def.h"
#include "../include/crlog.h"

// TODO: replace on splitted parts
#include "../include/lvtinydom.h"

#include "../include/lvdomelwriter.h"
#include "../include/lvdomdoc.h"

static bool isRightAligned(ldomNode * node) {
	lString16 style = node->getAttributeValue(attr_style);
	if (style.empty())
		return false;
	int p = style.pos("text-align: right", 0);
	return (p >= 0);
}


/** \brief callback object to fill DOM tree

	To be used with XML parser as callback object.

	Creates document according to incoming events.

	Autoclose HTML tags.
*/

void ldomDocumentWriterFilter::setClass( const lChar16 * className, bool overrideExisting )
{
	ldomNode * node = _currNode->_element;
	if ( _classAttrId==0 ) {
		_classAttrId = _document->getAttrNameIndex(L"class");
	}
	if ( overrideExisting || !node->hasAttribute(_classAttrId) ) {
		node->setAttributeValue(LXML_NS_NONE, _classAttrId, className);
	}
}

void ldomDocumentWriterFilter::appendStyle( const lChar16 * style )
{
	ldomNode * node = _currNode->_element;
	if ( _styleAttrId==0 ) {
		_styleAttrId = _document->getAttrNameIndex(L"style");
	}
	if (!_document->getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES))
		return; // disabled

	lString16 oldStyle = node->getAttributeValue(_styleAttrId);
	if ( !oldStyle.empty() && oldStyle.at(oldStyle.length()-1)!=';' )
		oldStyle << "; ";
	oldStyle << style;
	node->setAttributeValue(LXML_NS_NONE, _styleAttrId, oldStyle.c_str());
}

void ldomDocumentWriterFilter::AutoClose( lUInt16 tag_id, bool open )
{
	lUInt16 * rule = _rules[tag_id];
	if ( !rule )
		return;
	if ( open ) {
		ldomElementWriter * found = NULL;
		ldomElementWriter * p = _currNode;
		while ( p && !found ) {
			lUInt16 id = p->_element->getNodeId();
			for ( int i=0; rule[i]; i++ ) {
				if ( rule[i]==id ) {
					found = p;
					break;
				}
			}
			p = p->_parent;
		}
		// found auto-close target
		if ( found != NULL ) {
			bool done = false;
			while ( !done && _currNode ) {
				if ( _currNode == found )
					done = true;
				ldomNode * closedElement = _currNode->getElement();
				_currNode = pop( _currNode, closedElement->getNodeId() );
				//ElementCloseHandler( closedElement );
			}
		}
	} else {
		if ( !rule[0] )
			_currNode = pop( _currNode, _currNode->getElement()->getNodeId() );
	}
}

ldomNode * ldomDocumentWriterFilter::OnTagOpen( const lChar16 * nsname, const lChar16 * tagname )
{
	//CRLog::trace("OnTagOpen(%s, %s)", LCSTR(lString16(nsname)), LCSTR(lString16(tagname)));
	if ( !_tagBodyCalled ) {
		CRLog::error("OnTagOpen w/o parent's OnTagBody : %s", LCSTR(lString16(tagname)));
		crFatalError();
	}
	_tagBodyCalled = false;
	//logfile << "lxmlDocumentWriter::OnTagOpen() [" << nsname << ":" << tagname << "]";
//    if ( nsname && nsname[0] )
//        lStr_lowercase( const_cast<lChar16 *>(nsname), lStr_len(nsname) );
//    lStr_lowercase( const_cast<lChar16 *>(tagname), lStr_len(tagname) );

	// Patch for bad LIB.RU books - BR delimited paragraphs in "Fine HTML" format
	if ((tagname[0] == 'b' && tagname[1] == 'r' && tagname[2] == 0)
		|| (tagname[0] == 'd' && tagname[1] == 'd' && tagname[2] == 0)) {
		// substitute to P
		tagname = L"p";
		_libRuParagraphStart = true; // to trim leading &nbsp;
	} else {
		_libRuParagraphStart = false;
	}

	lUInt16 id = _document->getElementNameIndex(tagname);
	lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;
	AutoClose( id, true );
	_currNode = new ldomElementWriter( _document, nsid, id, _currNode );
	_flags = _currNode->getFlags();
	if ( _libRuDocumentDetected && (_flags & TXTFLG_PRE) )
		_flags |= TXTFLG_PRE_PARA_SPLITTING | TXTFLG_TRIM; // convert preformatted text into paragraphs
	//logfile << " !o!\n";
	//return _currNode->getElement();
	return _currNode->getElement();
}

void ldomDocumentWriterFilter::OnTagBody()
{
	_tagBodyCalled = true;
	if ( _currNode ) {
		_currNode->onBodyEnter();
	}
}

void ldomDocumentWriterFilter::ElementCloseHandler( ldomNode * node )
{
	ldomNode * parent = node->getParentNode();
	lUInt16 id = node->getNodeId();
	if ( parent ) {
		if ( parent->getLastChild() != node )
			return;
		if ( id==el_table ) {
			if (isRightAligned(node) && node->getAttributeValue(attr_width) == "30%") {
				// LIB.RU TOC detected: remove it
				//parent = parent->modify();

				//parent->removeLastChild();
			}
		} else if ( id==el_pre && _libRuDocumentDetected ) {
			// for LIB.ru - replace PRE element with DIV (section?)
			if ( node->getChildCount()==0 ) {
				//parent = parent->modify();

				//parent->removeLastChild(); // remove empty PRE element
			}
			//else if ( node->getLastChild()->getNodeId()==el_div && node->getLastChild()->getChildCount() &&
			//          ((ldomElement*)node->getLastChild())->getLastChild()->getNodeId()==el_form )
			//    parent->removeLastChild(); // remove lib.ru final section
			else
				node->setNodeId( el_div );
		} else if ( id==el_div ) {
//            CRLog::trace("DIV attr align = %s", LCSTR(node->getAttributeValue(attr_align)));
//            CRLog::trace("DIV attr count = %d", node->getAttrCount());
//            int alignId = node->getDocument()->getAttrNameIndex("align");
//            CRLog::trace("align= %d %d", alignId, attr_align);
//            for (int i = 0; i < node->getAttrCount(); i++)
//                CRLog::trace("DIV attr %s", LCSTR(node->getAttributeName(i)));
			if (isRightAligned(node)) {
				ldomNode * child = node->getLastChild();
				if ( child && child->getNodeId()==el_form )  {
					// LIB.RU form detected: remove it
					//parent = parent->modify();

					parent->removeLastChild();
					_libRuDocumentDetected = true;
				}
			}
		}
	}
	if (!_libRuDocumentDetected)
		node->persist();
}

void ldomDocumentWriterFilter::OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue )
{
	//logfile << "ldomDocumentWriter::OnAttribute() [" << nsname << ":" << attrname << "]";
	//if ( nsname && nsname[0] )
	//    lStr_lowercase( const_cast<lChar16 *>(nsname), lStr_len(nsname) );
	//lStr_lowercase( const_cast<lChar16 *>(attrname), lStr_len(attrname) );

	//CRLog::trace("OnAttribute(%s, %s)", LCSTR(lString16(attrname)), LCSTR(lString16(attrvalue)));

	if ( !lStr_cmp(attrname, "align") ) {
		if ( !lStr_cmp(attrvalue, "justify") )
			appendStyle( L"text-align: justify" );
		else if ( !lStr_cmp(attrvalue, "left") )
			appendStyle( L"text-align: left" );
		else if ( !lStr_cmp(attrvalue, "right") )
			appendStyle( L"text-align: right" );
		else if ( !lStr_cmp(attrvalue, "center") )
			appendStyle( L"text-align: center" );
	   return;
	}
	lUInt16 attr_ns = (nsname && nsname[0]) ? _document->getNsNameIndex( nsname ) : 0;
	lUInt16 attr_id = (attrname && attrname[0]) ? _document->getAttrNameIndex( attrname ) : 0;

	_currNode->addAttribute( attr_ns, attr_id, attrvalue );

	//logfile << " !a!\n";
}

/// called on closing tag
void ldomDocumentWriterFilter::OnTagClose( const lChar16 * /*nsname*/, const lChar16 * tagname )
{
	if ( !_tagBodyCalled ) {
		CRLog::error("OnTagClose w/o parent's OnTagBody : %s", LCSTR(lString16(tagname)));
		crFatalError();
	}
	//logfile << "ldomDocumentWriter::OnTagClose() [" << nsname << ":" << tagname << "]";
//    if ( nsname && nsname[0] )
//        lStr_lowercase( const_cast<lChar16 *>(nsname), lStr_len(nsname) );
//    lStr_lowercase( const_cast<lChar16 *>(tagname), lStr_len(tagname) );
	if (!_currNode)
	{
		_errFlag = true;
		//logfile << " !c-err!\n";
		return;
	}


	if (tagname[0] == 'l' && _currNode && !lStr_cmp(tagname, "link")) {
		// link node
		if ( _currNode && _currNode->getElement() && _currNode->getElement()->isNodeName("link") &&
			 _currNode->getElement()->getParentNode() && _currNode->getElement()->getParentNode()->isNodeName("head") &&
			 _currNode->getElement()->getAttributeValue("rel") == "stylesheet" &&
			 _currNode->getElement()->getAttributeValue("type") == "text/css" ) {
			lString16 href = _currNode->getElement()->getAttributeValue("href");
			lString16 stylesheetFile = LVCombinePaths( _document->getCodeBase(), href );
			CRLog::debug("Internal stylesheet file: %s", LCSTR(stylesheetFile));
			_document->setDocStylesheetFileName(stylesheetFile);
			_document->applyDocumentStyleSheet();
		}
	}

	lUInt16 id = _document->getElementNameIndex(tagname);

	// HTML title detection
	if ( id==el_title && _currNode->_element->getParentNode()!= NULL && _currNode->_element->getParentNode()->getNodeId()==el_head ) {
		lString16 s = _currNode->_element->getText();
		s.trim();
		if ( !s.empty() ) {
			// TODO: split authors, title & series
			_document->getProps()->setString( DOC_PROP_TITLE, s );
		}
	}
	//======== START FILTER CODE ============
	AutoClose( _currNode->_element->getNodeId(), false );
	//======== END FILTER CODE ==============
	//lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;
	// save closed element
	ldomNode * closedElement = _currNode->getElement();
	_errFlag |= (id != closedElement->getNodeId());
	_currNode = pop( _currNode, id );


	if ( _currNode ) {
		_flags = _currNode->getFlags();
		if ( _libRuDocumentDetected && (_flags & TXTFLG_PRE) )
			_flags |= TXTFLG_PRE_PARA_SPLITTING | TXTFLG_TRIM; // convert preformatted text into paragraphs
	}

	//=============================================================
	// LIB.RU patch: remove table of contents
	//ElementCloseHandler( closedElement );
	//=============================================================

	if ( id==_stopTagId ) {
		//CRLog::trace("stop tag found, stopping...");
		_parser->Stop();
	}
	//logfile << " !c!\n";
}

/// called on text
void ldomDocumentWriterFilter::OnText( const lChar16 * text, int len, lUInt32 flags )
{
	//logfile << "lxmlDocumentWriter::OnText() fpos=" << fpos;
	if (_currNode)
	{
		AutoClose( _currNode->_element->getNodeId(), false );
		if ( (_flags & XML_FLAG_NO_SPACE_TEXT)
			 && IsEmptySpace(text, len) && !(flags & TXTFLG_PRE))
			 return;
		bool autoPara = _libRuDocumentDetected && (flags & TXTFLG_PRE);
		if (_currNode->_allowText) {
			if ( _libRuParagraphStart ) {
				bool cleaned = false;
				while ( *text==160 && len > 0 ) {
					cleaned = true;
					text++;
					len--;
					while ( *text==' ' && len > 0 ) {
						text++;
						len--;
					}
				}
				if ( cleaned ) {
					setClass(L"justindent");
					//appendStyle(L"text-indent: 1.3em; text-align: justify");
				}
				_libRuParagraphStart = false;
			}
			int leftSpace = 0;
			const lChar16 * paraTag = NULL;
			bool isHr = false;
			if ( autoPara ) {
				while ( (*text==' ' || *text=='\t' || *text==160) && len > 0 ) {
					text++;
					len--;
					leftSpace += (*text == '\t') ? 8 : 1;
				}
				paraTag = leftSpace > 8 ? L"h2" : L"p";
				lChar16 ch = 0;
				bool sameCh = true;
				for ( int i=0; i<len; i++ ) {
					if ( !ch )
						ch = text[i];
					else if ( ch != text[i] ) {
						sameCh = false;
						break;
					}
				}
				if ( !ch )
					sameCh = false;
				if ( (ch=='-' || ch=='=' || ch=='_' || ch=='*' || ch=='#') && sameCh )
					isHr = true;
			}
			if ( isHr ) {
				OnTagOpen( NULL, L"hr" );
				OnTagBody();
				OnTagClose( NULL, L"hr" );
			} else if ( len > 0 ) {
				if ( autoPara ) {
					OnTagOpen( NULL, paraTag );
					OnTagBody();
				}
				_currNode->onText( text, len, flags );
				if ( autoPara )
					OnTagClose( NULL, paraTag );
			}
		}
	}
	//logfile << " !t!\n";
}

ldomDocumentWriterFilter::ldomDocumentWriterFilter(ldomDocument * document, bool headerOnly, const char *** rules )
: ldomDocumentWriter( document, headerOnly )
, _libRuDocumentDetected(false)
, _libRuParagraphStart(false)
, _styleAttrId(0)
, _classAttrId(0)
, _tagBodyCalled(true)
{
	lUInt16 i;
	for ( i=0; i<MAX_ELEMENT_TYPE_ID; i++ )
		_rules[i] = NULL;
	lUInt16 items[MAX_ELEMENT_TYPE_ID];
	for ( i=0; rules[i]; i++ ) {
		const char ** rule = rules[i];
		lUInt16 j;
		for ( j=0; rule[j] && j<MAX_ELEMENT_TYPE_ID; j++ ) {
			const char * s = rule[j];
			items[j] = _document->getElementNameIndex( lString16(s).c_str() );
		}
		if ( j>=1 ) {
			lUInt16 id = items[0];
			_rules[ id ] = new lUInt16[j];
			for ( int k=0; k<j; k++ ) {
				_rules[id][k] = k==j-1 ? 0 : items[k+1];
			}
		}
	}
}

ldomDocumentWriterFilter::~ldomDocumentWriterFilter()
{

	for ( int i=0; i<MAX_ELEMENT_TYPE_ID; i++ ) {
		if ( _rules[i] )
			delete[] _rules[i];
	}
}
