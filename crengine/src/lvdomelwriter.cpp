/*******************************************************

   CoolReader Engine

   ldomelwriter.cpp: fast and compact XML DOM tree:  ldomElementWriter

   (c) Vadim Lopatin, 2000-2011
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvdomelwriter.h"
#include "../include/fb2def.h"

// TODO: replace on splitted parts
#include "../include/lvtinydom.h"

#include "../include/lvdomdoc.h"

bool ldomElementWriter::IS_FIRST_BODY = false;

static lString16 getSectionHeader( ldomNode * section )
{
    lString16 header;
    if ( !section || section->getChildCount() == 0 )
        return header;
    ldomNode * child = section->getChildElementNode(0, L"title");
    if ( !child )
        return header;
    header = child->getText(L' ', 1024);
    return header;
}

static inline bool isBlockNode( ldomNode * node )
{
    if ( !node->isElement() )
        return false;
#if BUILD_LITE!=1
    switch ( node->getStyle()->display )
    {
    case css_d_block:
    case css_d_list_item:
    case css_d_table:
    case css_d_table_row:
    case css_d_inline_table:
    case css_d_table_row_group:
    case css_d_table_header_group:
    case css_d_table_footer_group:
    case css_d_table_column_group:
    case css_d_table_column:
    case css_d_table_cell:
    case css_d_table_caption:
        return true;

    case css_d_inherit:
    case css_d_inline:
    case css_d_run_in:
    case css_d_compact:
    case css_d_marker:
    case css_d_none:
        break;
    }
    return false;
#else
    return true;
#endif
}


ldomElementWriter::ldomElementWriter(ldomDocument * document, lUInt16 nsid, lUInt16 id, ldomElementWriter * parent)
	: _parent(parent), _document(document), _tocItem(NULL), _isBlock(true), _isSection(false), _stylesheetIsSet(false), _bodyEnterCalled(false)
{
	//logfile << "{c";
	_typeDef = _document->getElementTypePtr( id );
	_flags = 0;
	if ( (_typeDef && _typeDef->white_space==css_ws_pre) || (_parent && _parent->getFlags()&TXTFLG_PRE) )
		_flags |= TXTFLG_PRE;
	_isSection = (id==el_section);
	_allowText = _typeDef ? _typeDef->allow_text : (_parent?true:false);
	if (_parent)
		_element = _parent->getElement()->insertChildElement( (lUInt32)-1, nsid, id );
	else
		_element = _document->getRootNode(); //->insertChildElement( (lUInt32)-1, nsid, id );
	if ( ldomElementWriter::IS_FIRST_BODY && id==el_body ) {
		_tocItem = _document->getToc();
		//_tocItem->clear();
		ldomElementWriter::IS_FIRST_BODY = false;
	}
	//logfile << "}";
}

lUInt32 ldomElementWriter::getFlags()
{
	return _flags;
}

lString16 ldomElementWriter::getPath()
{
	if ( !_path.empty() || _element->isRoot() )
		return _path;
	_path = _parent->getPath() + "/" + _element->getXPathSegment();
	return _path;
}

void ldomElementWriter::updateTocItem()
{
	if ( !_isSection )
		return;
	// TODO: update item
	if ( _parent && _parent->_tocItem ) {
		lString16 title = getSectionHeader( _element );
		//CRLog::trace("TOC ITEM: %s", LCSTR(title));
		_tocItem = _parent->_tocItem->addChild(title, ldomXPointer(_element,0), getPath() );
	}
	_isSection = false;
}

void ldomElementWriter::onBodyEnter()
{
	_bodyEnterCalled = true;
#if BUILD_LITE!=1
	//CRLog::trace("onBodyEnter() for node %04x %s", _element->getDataIndex(), LCSTR(_element->getNodeName()));
	if ( _document->isDefStyleSet() ) {
		_element->initNodeStyle();
//        if ( _element->getStyle().isNull() ) {
//            CRLog::error("error while style initialization of element %x %s", _element->getNodeIndex(), LCSTR(_element->getNodeName()) );
//            crFatalError();
//        }
		_isBlock = isBlockNode(_element);
	} else {
	}
	if ( _isSection ) {
		if ( _parent && _parent->_isSection ) {
			_parent->updateTocItem();
		}

	}
#endif
}

void ldomElementWriter::onBodyExit()
{
	if ( _isSection )
		updateTocItem();

#if BUILD_LITE!=1
	if ( !_document->isDefStyleSet() )
		return;
	if ( !_bodyEnterCalled ) {
		onBodyEnter();
	}
//    if ( _element->getStyle().isNull() ) {
//        lString16 path;
//        ldomNode * p = _element->getParentNode();
//        while (p) {
//            path = p->getNodeName() + L"/" + path;
//            p = p->getParentNode();
//        }
//        //CRLog::error("style not initialized for element 0x%04x %s path %s", _element->getDataIndex(), LCSTR(_element->getNodeName()), LCSTR(path));
//        crFatalError();
//    }
	_element->initNodeRendMethod();

	if ( _stylesheetIsSet )
		_document->getStyleSheet()->pop();
#endif
}

void ldomElementWriter::onText( const lChar16 * text, int len, lUInt32 )
{
	//logfile << "{t";
	{
		// normal mode: store text copy
		// add text node, if not first empty space string of block node
		if ( !_isBlock || _element->getChildCount()!=0 || !IsEmptySpace( text, len ) || (_flags&TXTFLG_PRE) ) {
			lString8 s8 = UnicodeToUtf8(text, len);
			_element->insertChildText(s8);
		} else {
			//CRLog::trace("ldomElementWriter::onText: Ignoring first empty space of block item");
		}
	}
	//logfile << "}";
}

void ldomElementWriter::addAttribute( lUInt16 nsid, lUInt16 id, const wchar_t * value )
{
	getElement()->setAttributeValue(nsid, id, value);
#if BUILD_LITE!=1
	if ( id==attr_StyleSheet ) {
		_stylesheetIsSet = _element->applyNodeStylesheet();
	}
#endif
}

ldomElementWriter::~ldomElementWriter()
{
	//CRLog::trace("~ldomElementWriter for element 0x%04x %s", _element->getDataIndex(), LCSTR(_element->getNodeName()));
	//getElement()->persist();
	onBodyExit();
}
