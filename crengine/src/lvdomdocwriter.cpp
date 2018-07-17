/*******************************************************

   CoolReader Engine

   ldomdocwriter.cpp: fast and compact XML DOM tree: ldomDocumentWriter

   (c) Vadim Lopatin, 2000-2011
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvdomdocwriter.h"
#include "../include/fb2def.h"
#include "../include/crlog.h"

// TODO: replace on splitted parts
#include "../include/lvtinydom.h"

#include "../include/lvdomelwriter.h"
#include "../include/lvdomdoc.h"

ldomElementWriter * ldomDocumentWriter::pop( ldomElementWriter * obj, lUInt16 id )
{
	//logfile << "{p";
	ldomElementWriter * tmp = obj;
	for ( ; tmp; tmp = tmp->_parent )
	{
		//logfile << "-";
		if (tmp->getElement()->getNodeId() == id)
			break;
	}
	//logfile << "1";
	if (!tmp)
	{
		//logfile << "-err}";
		return obj; // error!!!
	}
	ldomElementWriter * tmp2 = NULL;
	//logfile << "2";
	for ( tmp = obj; tmp; tmp = tmp2 )
	{
		//logfile << "-";
		tmp2 = tmp->_parent;
		bool stop = (tmp->getElement()->getNodeId() == id);
		ElementCloseHandler( tmp->getElement() );
		delete tmp;
		if ( stop )
			return tmp2;
	}
	/*
	logfile << "3 * ";
	logfile << (int)tmp << " - " << (int)tmp2 << " | cnt=";
	logfile << (int)tmp->getElement()->childCount << " - "
			<< (int)tmp2->getElement()->childCount;
	*/
	//logfile << "}";
	return tmp2;
}

// overrides
void ldomDocumentWriter::OnStart(LVFileFormatParser * parser)
{
	//logfile << "ldomDocumentWriter::OnStart()\n";
	// add document root node
	//CRLog::trace("ldomDocumentWriter::OnStart()");
	if ( !_headerOnly )
		_stopTagId = 0xFFFE;
	else {
		_stopTagId = _document->getElementNameIndex(L"description");
		//CRLog::trace( "ldomDocumentWriter() : header only, tag id=%d", _stopTagId );
	}
	LVXMLParserCallback::OnStart( parser );
	_currNode = new ldomElementWriter(_document, 0, 0, NULL);
}

void ldomDocumentWriter::OnStop()
{
	//logfile << "ldomDocumentWriter::OnStop()\n";
	while (_currNode)
		_currNode = pop( _currNode, _currNode->getElement()->getNodeId() );
}

/// called after > of opening tag (when entering tag body)
void ldomDocumentWriter::OnTagBody()
{
	// init element style
	if ( _currNode ) {
		_currNode->onBodyEnter();
	}
}

ldomNode * ldomDocumentWriter::OnTagOpen( const lChar16 * nsname, const lChar16 * tagname )
{
	//logfile << "ldomDocumentWriter::OnTagOpen() [" << nsname << ":" << tagname << "]";
	//CRLog::trace("OnTagOpen(%s)", UnicodeToUtf8(lString16(tagname)).c_str());
	lUInt16 id = _document->getElementNameIndex(tagname);
	lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;

	//if ( id==_stopTagId ) {
		//CRLog::trace("stop tag found, stopping...");
	//    _parser->Stop();
	//}
	_currNode = new ldomElementWriter( _document, nsid, id, _currNode );
	_flags = _currNode->getFlags();
	//logfile << " !o!\n";
	//return _currNode->getElement();
	return _currNode->getElement();
}

ldomDocumentWriter::~ldomDocumentWriter()
{
	while (_currNode)
		_currNode = pop( _currNode, _currNode->getElement()->getNodeId() );
#if BUILD_LITE!=1
	if ( _document->isDefStyleSet() ) {
		if ( _popStyleOnFinish )
			_document->getStyleSheet()->pop();
		_document->getRootNode()->initNodeStyle();
		_document->getRootNode()->initNodeFont();
		//if ( !_document->validateDocument() )
		//    CRLog::error("*** document style validation failed!!!");
		_document->updateRenderContext();
		_document->dumpStatistics();
	}
#endif
}

void ldomDocumentWriter::OnTagClose( const lChar16 *, const lChar16 * tagname )
{
	//logfile << "ldomDocumentWriter::OnTagClose() [" << nsname << ":" << tagname << "]";
	if (!_currNode)
	{
		_errFlag = true;
		//logfile << " !c-err!\n";
		return;
	}
	if (tagname[0] == 'l' && _currNode && !lStr_cmp(tagname, "link") ) {
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

	bool isStyleSheetTag = !lStr_cmp(tagname, "stylesheet");
	if ( isStyleSheetTag ) {
		ldomNode *parentNode = _currNode->getElement()->getParentNode();
		if (parentNode && parentNode->isNodeName("DocFragment")) {
			_document->parseStyleSheet(_currNode->getElement()->getAttributeValue(attr_href),
									   _currNode->getElement()->getText());
			isStyleSheetTag = false;
		}
	}

	lUInt16 id = _document->getElementNameIndex(tagname);
	//lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;
	_errFlag |= (id != _currNode->getElement()->getNodeId());
	_currNode = pop( _currNode, id );

	if ( _currNode )
		_flags = _currNode->getFlags();

	if ( id==_stopTagId ) {
		//CRLog::trace("stop tag found, stopping...");
		_parser->Stop();
	}

	if ( isStyleSheetTag ) {
		//CRLog::trace("</stylesheet> found");
#if BUILD_LITE!=1
		if ( !_popStyleOnFinish ) {
			//CRLog::trace("saving current stylesheet before applying of document stylesheet");
			_document->getStyleSheet()->push();
			_popStyleOnFinish = true;
			_document->applyDocumentStyleSheet();
		}
#endif
	}
	//logfile << " !c!\n";
}

void ldomDocumentWriter::OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue )
{
	//logfile << "ldomDocumentWriter::OnAttribute() [" << nsname << ":" << attrname << "]";
	lUInt16 attr_ns = (nsname && nsname[0]) ? _document->getNsNameIndex( nsname ) : 0;
	lUInt16 attr_id = (attrname && attrname[0]) ? _document->getAttrNameIndex( attrname ) : 0;
	_currNode->addAttribute( attr_ns, attr_id, attrvalue );

	//logfile << " !a!\n";
}

void ldomDocumentWriter::OnText( const lChar16 * text, int len, lUInt32 flags )
{
	//logfile << "ldomDocumentWriter::OnText() fpos=" << fpos;
	if (_currNode)
	{
		if ( (_flags & XML_FLAG_NO_SPACE_TEXT)
			 && IsEmptySpace(text, len)  && !(flags & TXTFLG_PRE))
			 return;
		if (_currNode->_allowText)
			_currNode->onText( text, len, flags );
	}
	//logfile << " !t!\n";
}

bool ldomDocumentWriter::OnBlob(lString16 name, const lUInt8* data, int size)
{
	return _document->addBlob(name, data, size); 
}

void ldomDocumentWriter::OnDocProperty(const char* name, lString8 value)
{
	_document->getProps()->setString(name, value);
}

void ldomDocumentWriter::ElementCloseHandler(ldomNode* node)
{
	node->persist();
}

void ldomDocumentWriter::OnEncoding( const lChar16 *, const lChar16 *)
{
}

ldomDocumentWriter::ldomDocumentWriter(ldomDocument * document, bool headerOnly)
	: _document(document), _currNode(NULL), _errFlag(false), _headerOnly(headerOnly), _popStyleOnFinish(false), _flags(0)
{
	_stopTagId = 0xFFFE;
	ldomElementWriter::IS_FIRST_BODY = true;

#if BUILD_LITE!=1
	if ( _document->isDefStyleSet() ) {
		_document->getRootNode()->initNodeStyle();
		_document->getRootNode()->setRendMethod(erm_block);
	}
#endif

	//CRLog::trace("ldomDocumentWriter() headerOnly=%s", _headerOnly?"true":"false");
}
