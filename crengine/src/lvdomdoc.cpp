/*******************************************************

   CoolReader Engine

   lvdomdoc.cpp: fast and compact XML DOM tree:  ldomDocument

   (c) Vadim Lopatin, 2000-2011
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvdomdoc.h"
#include "../include/lvtinydom_defs.h"
#include "../include/lvrendpagelist.h"
#include "../include/lvrend_funcs.h"
#include "../include/crlog.h"

// TODO: replace on splitted parts
#include "../include/lvtinydom.h"

#include "../include/lvdomcache_p.h"
#include "../include/lvdomimpstlparser_p.h"

static xpath_step_t ParseXPathStep( const lChar16 * &path, lString16 & name, int & index )
{
    int pos = 0;
    const lChar16 * s = path;
    //int len = path.GetLength();
    name.clear();
    index = -1;
    int flgPrefix = 0;
    if (s && s[pos]) {
        lChar16 ch = s[pos];
        // prefix: none, '/' or '.'
        if (ch=='/') {
            flgPrefix = 1;
            ch = s[++pos];
        } else if (ch=='.') {
            flgPrefix = 2;
            ch = s[++pos];
        }
        int nstart = pos;
        if (ch>='0' && ch<='9') {
            // node or point index
            pos++;
            while (s[pos]>='0' && s[pos]<='9')
                pos++;
            if (s[pos] && s[pos!='/'] && s[pos]!='.')
                return xpath_step_error;
            lString16 sindex( path+nstart, pos-nstart );
            index = sindex.atoi();
            if (index<((flgPrefix==2)?0:1))
                return xpath_step_error;
            path += pos;
            return (flgPrefix==2) ? xpath_step_point : xpath_step_nodeindex;
        }
        while (s[pos] && s[pos]!='[' && s[pos]!='/' && s[pos]!='.')
            pos++;
        if (pos==nstart)
            return xpath_step_error;
        name = lString16( path+ nstart, pos-nstart );
        if (s[pos]=='[') {
            // index
            pos++;
            int istart = pos;
            while (s[pos] && s[pos]!=']' && s[pos]!='/' && s[pos]!='.')
                pos++;
            if (!s[pos] || pos==istart)
                return xpath_step_error;

            lString16 sindex( path+istart, pos-istart );
            index = sindex.atoi();
            pos++;
        }
        if (!s[pos] || s[pos]=='/' || s[pos]=='.') {
            path += pos;
            return (name == "text()") ? xpath_step_text : xpath_step_element; // OK!
        }
        return xpath_step_error; // error
    }
    return xpath_step_error;
}

static void writeNode( LVStream * stream, ldomNode * node, bool treeLayout )
{
    int level = 0;
    if ( treeLayout ) {
        level = node->getNodeLevel();
        for (int i=0; i<level; i++ )
            *stream << "  ";
    }
    if ( node->isText() )
    {
        lString8 txt = node->getText8();
        *stream << txt;
        if ( treeLayout )
            *stream << "\n";
    }
    else if (  node->isElement() )
    {
        lString8 elemName = UnicodeToUtf8(node->getNodeName());
        lString8 elemNsName = UnicodeToUtf8(node->getNodeNsName());
        if (!elemNsName.empty())
            elemName = elemNsName + ":" + elemName;
        if (!elemName.empty())
            *stream << "<" << elemName;
        int i;
        for (i=0; i<(int)node->getAttrCount(); i++)
        {
            const lxmlAttribute * attr = node->getAttribute(i);
            if (attr)
            {
                lString8 attrName( UnicodeToUtf8(node->getDocument()->getAttrName(attr->id)) );
                lString8 nsName( UnicodeToUtf8(node->getDocument()->getNsName(attr->nsid)) );
                lString8 attrValue( UnicodeToUtf8(node->getDocument()->getAttrValue(attr->index)) );
                *stream << " ";
                if ( nsName.length() > 0 )
                    *stream << nsName << ":";
                *stream << attrName << "=\"" << attrValue << "\"";
            }
        }

#if 0
            if (!elemName.empty())
            {
                ldomNode * elem = node;
                lvdomElementFormatRec * fmt = elem->getRenderData();
                css_style_ref_t style = elem->getStyle();
                if ( fmt ) {
                    lvRect rect;
                    elem->getAbsRect( rect );
                    *stream << L" fmt=\"";
                    *stream << L"rm:" << lString16::itoa( (int)elem->getRendMethod() ) << L" ";
                    if ( style.isNull() )
                        *stream << L"style: NULL ";
                    else {
                        *stream << L"disp:" << lString16::itoa( (int)style->display ) << L" ";
                    }
                    *stream << L"y:" << lString16::itoa( (int)fmt->getY() ) << L" ";
                    *stream << L"h:" << lString16::itoa( (int)fmt->getHeight() ) << L" ";
                    *stream << L"ay:" << lString16::itoa( (int)rect.top ) << L" ";
                    *stream << L"ah:" << lString16::itoa( (int)rect.height() ) << L" ";
                    *stream << L"\"";
                }
            }
#endif

        if ( node->getChildCount() == 0 ) {
            if (!elemName.empty())
            {
                if ( elemName[0] == '?' )
                    *stream << "?>";
                else
                    *stream << "/>";
            }
            if ( treeLayout )
                *stream << "\n";
        } else {
            if (!elemName.empty())
                *stream << ">";
            if ( treeLayout )
                *stream << "\n";
            for (i=0; i<(int)node->getChildCount(); i++)
            {
                writeNode( stream, node->getChildNode(i), treeLayout );
            }
            if ( treeLayout ) {
                for (int i=0; i<level; i++ )
                    *stream << "  ";
            }
            if (!elemName.empty())
                *stream << "</" << elemName << ">";
            if ( treeLayout )
                *stream << "\n";
        }
    }
}



/// create xpointer from pointer string
ldomXPointer ldomDocument::createXPointer( const lString16 & xPointerStr )
{
	if ( xPointerStr[0]=='#' ) {
		lString16 id = xPointerStr.substr(1);
		lUInt16 idid = getAttrValueIndex(id.c_str());
		lInt32 nodeIndex;
		if ( _idNodeMap.get(idid, nodeIndex) ) {
			ldomNode * node = getTinyNode(nodeIndex);
			if ( node && node->isElement() ) {
				return ldomXPointer(node, -1);
			}
		}
		return ldomXPointer();
	}
	return createXPointer( getRootNode(), xPointerStr );
}

/// create xpointer from doc point
ldomXPointer ldomDocument::createXPointer( lvPoint pt, int direction )
{
	//
	ldomXPointer ptr;
	if ( !getRootNode() )
		return ptr;
	ldomNode * finalNode = getRootNode()->elementFromPoint( pt, direction );
	if ( !finalNode ) {
		if ( pt.y >= getFullHeight()) {
			ldomNode * node = getRootNode()->getLastTextChild();
			return ldomXPointer(node,node ? node->getText().length() : 0);
		} else if ( pt.y <= 0 ) {
			ldomNode * node = getRootNode()->getFirstTextChild();
			return ldomXPointer(node, 0);
		}
		CRLog::trace("not final node");
		return ptr;
	}
	lvRect rc;
	finalNode->getAbsRect( rc );
	//CRLog::debug("ldomDocument::createXPointer point = (%d, %d), finalNode %08X rect = (%d,%d,%d,%d)", pt.x, pt.y, (lUInt32)finalNode, rc.left, rc.top, rc.right, rc.bottom );
	pt.x -= rc.left;
	pt.y -= rc.top;
	//if ( !r )
	//    return ptr;
	if ( finalNode->getRendMethod() != erm_final && finalNode->getRendMethod() !=  erm_list_item) {
		// not final, use as is
		if ( pt.y < (rc.bottom + rc.top) / 2 )
			return ldomXPointer( finalNode, 0 );
		else
			return ldomXPointer( finalNode, finalNode->getChildCount() );
	}
	// final, format and search
	LFormattedTextRef txtform;
	{
		RenderRectAccessor r( finalNode );
		finalNode->renderFinalBlock( txtform, &r, r.getWidth() );
	}
	int lcount = txtform->GetLineCount();
	for ( int l = 0; l<lcount; l++ ) {
		const formatted_line_t * frmline = txtform->GetLineInfo(l);
		if ( pt.y >= (int)(frmline->y + frmline->height) && l<lcount-1 )
			continue;
		//CRLog::debug("  point (%d, %d) line found [%d]: (%d..%d)", pt.x, pt.y, l, frmline->y, frmline->y+frmline->height);
		// found line, searching for word
		int wc = (int)frmline->word_count;
		int x = pt.x - frmline->x;
		for ( int w=0; w<wc; w++ ) {
			const formatted_word_t * word = &frmline->words[w];
			if ( x < word->x + word->width || w==wc-1 ) {
				const src_text_fragment_t * src = txtform->GetSrcInfo(word->src_text_index);
				//CRLog::debug(" word found [%d]: x=%d..%d, start=%d, len=%d  %08X", w, word->x, word->x + word->width, word->t.start, word->t.len, src->object);
				// found word, searching for letters
				ldomNode * node = (ldomNode *)src->object;
				if ( !node )
					continue;
				if ( src->flags & LTEXT_SRC_IS_OBJECT ) {
					// object (image)
#if 1
					// return image object itself
					return ldomXPointer(node, 0);
#else
					return ldomXPointer( node->getParentNode(),
						node->getNodeIndex() + (( x < word->x + word->width/2 ) ? 0 : 1) );
#endif
				}
				LVFont * font = (LVFont *) src->t.font;
				lUInt16 w[512];
				lUInt8 flg[512];

				lString16 str = node->getText();
				font->measureText( str.c_str()+word->t.start, word->t.len, w, flg, word->width+50, '?', src->letter_spacing);
				for ( int i=0; i<word->t.len; i++ ) {
					int xx = ( i>0 ) ? (w[i-1] + w[i])/2 : w[i]/2;
					if ( x < word->x + xx ) {
						return ldomXPointer( node, src->t.offset + word->t.start + i );
					}
				}
				return ldomXPointer( node, src->t.offset + word->t.start + word->t.len );
			}
		}
	}
	return ptr;
}

/// create xpointer from relative pointer string
ldomXPointer ldomDocument::createXPointer( ldomNode * baseNode, const lString16 & xPointerStr )
{
	//CRLog::trace( "ldomDocument::createXPointer(%s)", UnicodeToUtf8(xPointerStr).c_str() );
	if ( xPointerStr.empty() || !baseNode)
		return ldomXPointer();
	const lChar16 * str = xPointerStr.c_str();
	int index = -1;
	ldomNode * currNode = baseNode;
	lString16 name;
	lString8 ptr8 = UnicodeToUtf8(xPointerStr);
	//const char * ptr = ptr8.c_str();
	xpath_step_t step_type;

	while ( *str ) {
		//CRLog::trace( "    %s", UnicodeToUtf8(lString16(str)).c_str() );
		step_type = ParseXPathStep( str, name, index );
		//CRLog::trace( "        name=%s index=%d", UnicodeToUtf8(lString16(name)).c_str(), index );
		switch (step_type ) {
		case xpath_step_error:
			// error
			//CRLog::trace("    xpath_step_error");
			return ldomXPointer();
		case xpath_step_element:
			// element of type 'name' with 'index'        /elemname[N]/
			{
				lUInt16 id = getElementNameIndex( name.c_str() );
				ldomNode * foundItem = currNode->findChildElement(LXML_NS_ANY, id, index > 0 ? index - 1 : -1);
				if (foundItem == NULL && currNode->getChildCount() == 1) {
					// make saved pointers work properly even after moving of some part of path one element deeper
					ldomNode* node0 = currNode->getChildNode(0);
					if (node0)
						foundItem = node0->findChildElement(LXML_NS_ANY, id, index > 0 ? index - 1 : -1);
				}
//                int foundCount = 0;
//                for (unsigned i=0; i<currNode->getChildCount(); i++) {
//                    ldomNode * p = currNode->getChildNode(i);
//                    //CRLog::trace( "        node[%d] = %d %s", i, p->getNodeId(), LCSTR(p->getNodeName()) );
//                    if ( p && p->isElement() && p->getNodeId()==id ) {
//                        foundCount++;
//                        if ( foundCount==index || index==-1 ) {
//                            foundItem = p;
//                            break; // DON'T CHECK WHETHER OTHER ELEMENTS EXIST
//                        }
//                    }
//                }
//                if ( foundItem==NULL || (index==-1 && foundCount>1) ) {
//                    //CRLog::trace("    Element %d is not found. foundCount=%d", id, foundCount);
//                    return ldomXPointer(); // node not found
//                }
				if (foundItem == NULL) {
					//CRLog::trace("    Element %d is not found. foundCount=%d", id, foundCount);
					return ldomXPointer(); // node not found
				}
				// found element node
				currNode = foundItem;
			}
			break;
		case xpath_step_text:
			// text node with 'index'                     /text()[N]/
			{
				ldomNode * foundItem = NULL;
				int foundCount = 0;
				for (int i=0; i<currNode->getChildCount(); i++) {
					ldomNode * p = currNode->getChildNode(i);
					if ( p->isText() ) {
						foundCount++;
						if ( foundCount==index || index==-1 ) {
							foundItem = p;
						}
					}
				}
				if ( foundItem==NULL || (index==-1 && foundCount>1) )
					return ldomXPointer(); // node not found
				// found text node
				currNode = foundItem;
			}
			break;
		case xpath_step_nodeindex:
			// node index                                 /N/
			if ( index<=0 || index>(int)currNode->getChildCount() )
				return ldomXPointer(); // node not found: invalid index
			currNode = currNode->getChildNode( index-1 );
			break;
		case xpath_step_point:
			// point index                                .N
			if (*str)
				return ldomXPointer(); // not at end of string
			if ( currNode->isElement() ) {
				// element point
				if ( index<0 || index>(int)currNode->getChildCount() )
					return ldomXPointer();
				return ldomXPointer(currNode, index);
			} else {
				// text point
				if ( index<0 || index>(int)currNode->getText().length() )
					return ldomXPointer();
				return ldomXPointer(currNode, index);
			}
			break;
		}
	}
	return ldomXPointer( currNode, -1 ); // XPath: index==-1
}

#if BUILD_LITE!=1
int ldomDocument::getFullHeight()
{
	RenderRectAccessor rd( this->getRootNode() );
	return rd.getHeight() + rd.getY();
}
#endif

#if BUILD_LITE!=1
bool ldomDocument::findText( lString16 pattern, bool caseInsensitive, bool reverse, int minY, int maxY, LVArray<ldomWord> & words, int maxCount, int maxHeight )
{
	if ( minY<0 )
		minY = 0;
	int fh = getFullHeight();
	if ( maxY<=0 || maxY>fh )
		maxY = fh;
	ldomXPointer start = createXPointer( lvPoint(0, minY), reverse?-1:1 );
	ldomXPointer end = createXPointer( lvPoint(10000, maxY), reverse?-1:1 );
	if ( start.isNull() || end.isNull() )
		return false;
	ldomXRange range( start, end );
	CRLog::debug("ldomDocument::findText() for Y %d..%d, range %d..%d", minY, maxY, start.toPoint().y, end.toPoint().y);
	if ( range.getStart().toPoint().y==-1 ) {
		range.getStart().nextVisibleText();
		CRLog::debug("ldomDocument::findText() updated range %d..%d", range.getStart().toPoint().y, range.getEnd().toPoint().y);
	}
	if ( range.getEnd().toPoint().y==-1 ) {
		range.getEnd().prevVisibleText();
		CRLog::debug("ldomDocument::findText() updated range %d..%d", range.getStart().toPoint().y, range.getEnd().toPoint().y);
	}
	if ( range.isNull() ) {
		CRLog::debug("No text found: Range is empty");
		return false;
	}
	return range.findText( pattern, caseInsensitive, reverse, words, maxCount, maxHeight );
}
#endif

void ldomDocument::clear()
{
#if BUILD_LITE!=1
	clearRendBlockCache();
	_rendered = false;
	_urlImageMap.clear();
	_fontList.clear();
	LVFontManager::getInstance()->UnregisterDocumentFonts(_docIndex);
#endif
	//TODO: implement clear
	//_elemStorage.
}

#if BUILD_LITE!=1
bool ldomDocument::openFromCache( CacheLoadingCallback * formatCallback )
{
	if ( !openCacheFile() ) {
		CRLog::info("Cannot open document from cache. Need to read fully");
		clear();
		return false;
	}
	if ( !loadCacheFileContent(formatCallback) ) {
		CRLog::info("Error while loading document content from cache file.");
		clear();
		return false;
	}
#if 0
	LVStreamRef s = LVOpenFileStream("/tmp/test.xml", LVOM_WRITE);
	if ( !s.isNull() )
		saveToStream(s, "UTF8");
#endif
	_mapped = true;
	_rendered = true;
	return true;
}

/// load document cache file content, @see saveChanges()
bool ldomDocument::loadCacheFileContent(CacheLoadingCallback * formatCallback)
{

	CRLog::trace("ldomDocument::loadCacheFileContent()");
	{
		SerialBuf propsbuf(0, true);
		if ( !_cacheFile->read( CBT_PROP_DATA, propsbuf ) ) {
			CRLog::error("Error while reading props data");
			return false;
		}
		getProps()->deserialize( propsbuf );
		if ( propsbuf.error() ) {
			CRLog::error("Cannot decode property table for document");
			return false;
		}

		CRLog::trace("ldomDocument::loadCacheFileContent() - ID data");
		SerialBuf idbuf(0, true);
		if ( !_cacheFile->read( CBT_MAPS_DATA, idbuf ) ) {
			CRLog::error("Error while reading Id data");
			return false;
		}
		deserializeMaps( idbuf );
		if ( idbuf.error() ) {
			CRLog::error("Cannot decode ID table for document");
			return false;
		}

		CRLog::trace("ldomDocument::loadCacheFileContent() - page data");
		SerialBuf pagebuf(0, true);
		if ( !_cacheFile->read( CBT_PAGE_DATA, pagebuf ) ) {
			CRLog::error("Error while reading pages data");
			return false;
		}
		pagebuf.swap( _pagesData );
		_pagesData.setPos( 0 );
		LVRendPageList pages;
		pages.deserialize(_pagesData);
		if ( _pagesData.error() ) {
			CRLog::error("Page data deserialization is failed");
			return false;
		}
		CRLog::info("%d pages read from cache file", pages.length());
		//_pagesData.setPos( 0 );

		CRLog::trace("ldomDocument::loadCacheFileContent() - embedded font data");
		{
			SerialBuf buf(0, true);
			if ( !_cacheFile->read(CBT_FONT_DATA, buf)) {
				CRLog::error("Error while reading font data");
				return false;
			}
			if (!_fontList.deserialize(buf)) {
				CRLog::error("Error while parsing font data");
				return false;
			}
			registerEmbeddedFonts();
		}

		DocFileHeader h;
		memset(&h, 0, sizeof(h));
		SerialBuf hdrbuf(0,true);
		if ( !_cacheFile->read( CBT_REND_PARAMS, hdrbuf ) ) {
			CRLog::error("Error while reading header data");
			return false;
		} else if ( !h.deserialize(hdrbuf) ) {
			CRLog::error("Header data deserialization is failed");
			return false;
		}
		_hdr = h;
		CRLog::info("Loaded render properties: styleHash=%x, stylesheetHash=%x, docflags=%04x, width=%d, height=%d",
				_hdr.render_style_hash, _hdr.stylesheet_hash, _hdr.render_docflags, _hdr.render_dx, _hdr.render_dy);
	}

	CRLog::trace("ldomDocument::loadCacheFileContent() - node data");
	if ( !loadNodeData() ) {
		CRLog::error("Error while reading node instance data");
		return false;
	}


	CRLog::trace("ldomDocument::loadCacheFileContent() - element storage");
	if ( !_elemStorage.load() ) {
		CRLog::error("Error while loading element data");
		return false;
	}
	CRLog::trace("ldomDocument::loadCacheFileContent() - text storage");
	if ( !_textStorage.load() ) {
		CRLog::error("Error while loading text data");
		return false;
	}
	CRLog::trace("ldomDocument::loadCacheFileContent() - rect storage");
	if ( !_rectStorage.load() ) {
		CRLog::error("Error while loading rect data");
		return false;
	}
	CRLog::trace("ldomDocument::loadCacheFileContent() - node style storage");
	if ( !_styleStorage.load() ) {
		CRLog::error("Error while loading node style data");
		return false;
	}

	CRLog::trace("ldomDocument::loadCacheFileContent() - TOC");
	{
		SerialBuf tocbuf(0,true);
		if ( !_cacheFile->read( CBT_TOC_DATA, tocbuf ) ) {
			CRLog::error("Error while reading TOC data");
			return false;
		} else if ( !m_toc.deserialize(this, tocbuf) ) {
			CRLog::error("TOC data deserialization is failed");
			return false;
		}
	}

	if ( formatCallback ) {
		int fmt = getProps()->getIntDef(DOC_PROP_FILE_FORMAT_ID,
				doc_format_fb2);
		if (fmt < doc_format_fb2 || fmt > doc_format_max)
			fmt = doc_format_fb2;
		// notify about format detection, to allow setting format-specific CSS
		formatCallback->OnCacheFileFormatDetected((doc_format_t)fmt);
	}

	if ( loadStylesData() ) {
		CRLog::trace("ldomDocument::loadCacheFileContent() - using loaded styles");
		updateLoadedStyles( true );
//        lUInt32 styleHash = calcStyleHash();
//        styleHash = styleHash * 31 + calcGlobalSettingsHash();
//        CRLog::debug("Loaded style hash: %x", styleHash);
//        lUInt32 styleHash = calcStyleHash();
//        CRLog::info("Loaded style hash = %08x", styleHash);
	} else {
		CRLog::trace("ldomDocument::loadCacheFileContent() - style loading failed: will reinit ");
		updateLoadedStyles( false );
	}

	CRLog::trace("ldomDocument::loadCacheFileContent() - completed successfully");

	return true;
}

#define CHECK_EXPIRATION(s) \
	if ( maxTime.expired() ) { CRLog::info("timer expired while " s); return CR_TIMEOUT; }

/// saves changes to cache file, limited by time interval (can be called again to continue after TIMEOUT)
ContinuousOperationResult ldomDocument::saveChanges( CRTimerUtil & maxTime )
{
	if ( !_cacheFile )
		return CR_DONE;

	if (maxTime.infinite()) {
		_mapSavingStage = 0; // all stages from the beginning
		_cacheFile->setAutoSyncSize(0);
	} else {
		//CRLog::trace("setting autosync");
		_cacheFile->setAutoSyncSize(STREAM_AUTO_SYNC_SIZE);
		//CRLog::trace("setting autosync - done");
	}

	CRLog::trace("ldomDocument::saveChanges(timeout=%d stage=%d)", maxTime.interval(), _mapSavingStage);

	switch (_mapSavingStage) {
	default:
	case 0:

		if (!maxTime.infinite())
			_cacheFile->flush(false, maxTime);
		CHECK_EXPIRATION("flushing of stream")

		persist( maxTime );
		CHECK_EXPIRATION("persisting of node data")

		// fall through
	case 1:
		_mapSavingStage = 1;
		CRLog::trace("ldomDocument::saveChanges() - element storage");

		if ( !_elemStorage.save(maxTime) ) {
			CRLog::error("Error while saving element data");
			return CR_ERROR;
		}
		CHECK_EXPIRATION("saving element storate")
		// fall through
	case 2:
		_mapSavingStage = 2;
		CRLog::trace("ldomDocument::saveChanges() - text storage");
		if ( !_textStorage.save(maxTime) ) {
			CRLog::error("Error while saving text data");
			return CR_ERROR;
		}
		CHECK_EXPIRATION("saving text storate")
		// fall through
	case 3:
		_mapSavingStage = 3;
		CRLog::trace("ldomDocument::saveChanges() - rect storage");

		if ( !_rectStorage.save(maxTime) ) {
			CRLog::error("Error while saving rect data");
			return CR_ERROR;
		}
		CHECK_EXPIRATION("saving rect storate")
		// fall through
	case 41:
		_mapSavingStage = 41;
		CRLog::trace("ldomDocument::saveChanges() - blob storage data");

		if ( _blobCache.saveToCache(maxTime) == CR_ERROR ) {
			CRLog::error("Error while saving blob storage data");
			return CR_ERROR;
		}
		if (!maxTime.infinite())
			_cacheFile->flush(false, maxTime); // intermediate flush
		CHECK_EXPIRATION("saving blob storage data")
		// fall through
	case 4:
		_mapSavingStage = 4;
		CRLog::trace("ldomDocument::saveChanges() - node style storage");

		if ( !_styleStorage.save(maxTime) ) {
			CRLog::error("Error while saving node style data");
			return CR_ERROR;
		}
		if (!maxTime.infinite())
			_cacheFile->flush(false, maxTime); // intermediate flush
		CHECK_EXPIRATION("saving node style storage")
		// fall through
	case 5:
		_mapSavingStage = 5;
		CRLog::trace("ldomDocument::saveChanges() - misc data");
		{
			SerialBuf propsbuf(4096);
			getProps()->serialize( propsbuf );
			if ( !_cacheFile->write( CBT_PROP_DATA, propsbuf, COMPRESS_MISC_DATA ) ) {
				CRLog::error("Error while saving props data");
				return CR_ERROR;
			}
		}
		if (!maxTime.infinite())
			_cacheFile->flush(false, maxTime); // intermediate flush
		CHECK_EXPIRATION("saving props data")
		// fall through
	case 6:
		_mapSavingStage = 6;
		CRLog::trace("ldomDocument::saveChanges() - ID data");
		{
			SerialBuf idbuf(4096);
			serializeMaps( idbuf );
			if ( !_cacheFile->write( CBT_MAPS_DATA, idbuf, COMPRESS_MISC_DATA ) ) {
				CRLog::error("Error while saving Id data");
				return CR_ERROR;
			}
		}
		if (!maxTime.infinite())
			_cacheFile->flush(false, maxTime); // intermediate flush
		CHECK_EXPIRATION("saving ID data")
		// fall through
	case 7:
		_mapSavingStage = 7;
		if ( _pagesData.pos() ) {
			CRLog::trace("ldomDocument::saveChanges() - page data (%d bytes)", _pagesData.pos());
			if ( !_cacheFile->write( CBT_PAGE_DATA, _pagesData, COMPRESS_PAGES_DATA  ) ) {
				CRLog::error("Error while saving pages data");
				return CR_ERROR;
			}
		} else {
			CRLog::trace("ldomDocument::saveChanges() - no page data");
		}
		if (!maxTime.infinite())
			_cacheFile->flush(false, maxTime); // intermediate flush
		CHECK_EXPIRATION("saving page data")
		// fall through
	case 8:
		_mapSavingStage = 8;

		CRLog::trace("ldomDocument::saveChanges() - node data");
		if ( !saveNodeData() ) {
			CRLog::error("Error while node instance data");
			return CR_ERROR;
		}
		if (!maxTime.infinite())
			_cacheFile->flush(false, maxTime); // intermediate flush
		CHECK_EXPIRATION("saving node data")
		// fall through
	case 9:
		_mapSavingStage = 9;
		CRLog::trace("ldomDocument::saveChanges() - render info");
		{
			SerialBuf hdrbuf(0,true);
			if ( !_hdr.serialize(hdrbuf) ) {
				CRLog::error("Header data serialization is failed");
				return CR_ERROR;
			} else if ( !_cacheFile->write( CBT_REND_PARAMS, hdrbuf, false ) ) {
				CRLog::error("Error while writing header data");
				return CR_ERROR;
			}
		}
		CRLog::info("Saving render properties: styleHash=%x, stylesheetHash=%x, docflags=%04x, width=%d, height=%d",
					_hdr.render_style_hash, _hdr.stylesheet_hash, _hdr.render_docflags, _hdr.render_dx, _hdr.render_dy);


		CRLog::trace("ldomDocument::saveChanges() - TOC");
		{
			SerialBuf tocbuf(0,true);
			if ( !m_toc.serialize(tocbuf) ) {
				CRLog::error("TOC data serialization is failed");
				return CR_ERROR;
			} else if ( !_cacheFile->write( CBT_TOC_DATA, tocbuf, COMPRESS_TOC_DATA ) ) {
				CRLog::error("Error while writing TOC data");
				return CR_ERROR;
			}
		}
		if (!maxTime.infinite())
			_cacheFile->flush(false, maxTime); // intermediate flush
		CHECK_EXPIRATION("saving TOC data")
		// fall through
	case 10:
		_mapSavingStage = 10;

		if ( !saveStylesData() ) {
			CRLog::error("Error while writing style data");
			return CR_ERROR;
		}
		// fall through
	case 11:
		_mapSavingStage = 11;
		CRLog::trace("ldomDocument::saveChanges() - embedded fonts");
		{
			SerialBuf buf(4096);
			_fontList.serialize(buf);
			if (!_cacheFile->write(CBT_FONT_DATA, buf, COMPRESS_MISC_DATA) ) {
				CRLog::error("Error while saving embedded font data");
				return CR_ERROR;
			}
			CHECK_EXPIRATION("saving embedded fonts")
		}
		// fall through
	case 12:
		_mapSavingStage = 12;
		CRLog::trace("ldomDocument::saveChanges() - flush");
		{
			CRTimerUtil infinite;
			if ( !_cacheFile->flush(true, infinite) ) {
				CRLog::error("Error while updating index of cache file");
				return CR_ERROR;
			}
			CHECK_EXPIRATION("flushing")
		}
		// fall through
	case 13:
		_mapSavingStage = 13;
	}
	CRLog::trace("ldomDocument::saveChanges() - done");
	return CR_DONE;
}

/// save changes to cache file, @see loadCacheFileContent()
bool ldomDocument::saveChanges()
{
	if ( !_cacheFile )
		return true;
	CRLog::debug("ldomDocument::saveChanges() - infinite");
	CRTimerUtil timerNoLimit;
	ContinuousOperationResult res = saveChanges(timerNoLimit);
	return res!=CR_ERROR;
}

/// swaps to cache file or saves changes, limited by time interval
ContinuousOperationResult ldomDocument::swapToCache( CRTimerUtil & maxTime )
{
	CRLog::trace("ldomDocument::swapToCache entered");
	if ( _maperror )
		return CR_ERROR;
	if ( !_mapped ) {
		CRLog::trace("ldomDocument::swapToCache creating cache file");
		if ( !createCacheFile() ) {
			CRLog::error("ldomDocument::swapToCache: failed: cannot create cache file");
			_maperror = true;
			return CR_ERROR;
		}
	}
	_mapped = true;
	if (!maxTime.infinite()) {
		CRLog::info("Cache file is created, but document saving is postponed");
		return CR_TIMEOUT;
	}
	ContinuousOperationResult res = saveChanges(maxTime);
	if ( res==CR_ERROR )
	{
		CRLog::error("Error while saving changes to cache file");
		_maperror = true;
		return CR_ERROR;
	}
	CRLog::info("Successfully saved document to cache file: %dK", _cacheFile->getSize()/1024 );
	return res;
}

/// saves recent changes to mapped file
ContinuousOperationResult ldomDocument::updateMap(CRTimerUtil & maxTime)
{
	if ( !_cacheFile || !_mapped )
		return CR_DONE;

	ContinuousOperationResult res = saveChanges(maxTime);
	if ( res==CR_ERROR )
	{
		CRLog::error("Error while saving changes to cache file");
		return CR_ERROR;
	}

	if ( res==CR_DONE ) {
		CRLog::info("Cache file updated successfully");
		dumpStatistics();
	}
	return res;
}

#endif

#if BUILD_LITE!=1

/// save document formatting parameters after render
void ldomDocument::updateRenderContext()
{
	int dx = _page_width;
	int dy = _page_height;
	lUInt32 styleHash = calcStyleHash();
	lUInt32 stylesheetHash = (((_stylesheet.getHash() * 31) + calcHash(_def_style))*31 + calcHash(_def_font));
	//calcStyleHash( getRootNode(), styleHash );
	_hdr.render_style_hash = styleHash;
	_hdr.stylesheet_hash = stylesheetHash;
	_hdr.render_dx = dx;
	_hdr.render_dy = dy;
	_hdr.render_docflags = _docFlags;
	CRLog::info("Updating render properties: styleHash=%x, stylesheetHash=%x, docflags=%04x, width=%d, height=%d",
				_hdr.render_style_hash, _hdr.stylesheet_hash, _hdr.render_docflags, _hdr.render_dx, _hdr.render_dy);
}

/// check document formatting parameters before render - whether we need to reformat; returns false if render is necessary
bool ldomDocument::checkRenderContext()
{
	bool res = true;
	ldomNode * node = getRootNode();
	if (node != NULL && node->getFont().isNull()) {
		CRLog::info("checkRenderContext: style is not set for root node");
		res = false;
	}
	int dx = _page_width;
	int dy = _page_height;
	lUInt32 styleHash = calcStyleHash();
	lUInt32 stylesheetHash = (((_stylesheet.getHash() * 31) + calcHash(_def_style))*31 + calcHash(_def_font));
	//calcStyleHash( getRootNode(), styleHash );
	if ( styleHash != _hdr.render_style_hash ) {
		CRLog::info("checkRenderContext: Style hash doesn't match %x!=%x", styleHash, _hdr.render_style_hash);
		res = false;
	} else if ( stylesheetHash != _hdr.stylesheet_hash ) {
		CRLog::info("checkRenderContext: Stylesheet hash doesn't match %x!=%x", stylesheetHash, _hdr.stylesheet_hash);
		res = false;
	} else if ( _docFlags != _hdr.render_docflags ) {
		CRLog::info("checkRenderContext: Doc flags don't match %x!=%x", _docFlags, _hdr.render_docflags);
		res = false;
	} else if ( dx != (int)_hdr.render_dx ) {
		CRLog::info("checkRenderContext: Width doesn't match %x!=%x", dx, (int)_hdr.render_dx);
		res = false;
	} else if ( dy != (int)_hdr.render_dy ) {
		CRLog::info("checkRenderContext: Page height doesn't match %x!=%x", dy, (int)_hdr.render_dy);
		res = false;
	}
	if ( res ) {

		//if ( pages->length()==0 ) {
//            _pagesData.reset();
//            pages->deserialize( _pagesData );
		//}

		return true;
	}
//    _hdr.render_style_hash = styleHash;
//    _hdr.stylesheet_hash = stylesheetHash;
//    _hdr.render_dx = dx;
//    _hdr.render_dy = dy;
//    _hdr.render_docflags = _docFlags;
//    CRLog::info("New render properties: styleHash=%x, stylesheetHash=%x, docflags=%04x, width=%d, height=%d",
//                _hdr.render_style_hash, _hdr.stylesheet_hash, _hdr.render_docflags, _hdr.render_dx, _hdr.render_dy);
	return false;
}

#endif

/// register embedded document fonts in font manager, if any exist in document
void ldomDocument::registerEmbeddedFonts()
{
	if (_fontList.empty())
		return;
	int list = _fontList.length();
	lString8 lastface = lString8("");
	LVFontManager* fontMan = LVFontManager::getInstance();
	for (int i = list; i > 0; i--) {
		LVEmbeddedFontDef *item = _fontList.get(i - 1);
		lString16 url = item->getUrl();
		lString8 face = item->getFace();
		if (face.empty()) face = lastface;
		else lastface = face;
		CRLog::debug("url is %s\n", UnicodeToLocal(url).c_str());
		if (url.startsWithNoCase(lString16("res://")) || url.startsWithNoCase(lString16("file://"))) {
			if (!fontMan->RegisterExternalFont(item->getUrl(), item->getFace(), item->getBold(), item->getItalic())) {
				CRLog::error("Failed to register external font face: %s file: %s", item->getFace().c_str(), LCSTR(item->getUrl()));
			}
			continue;
		}
		else {
			if (!fontMan->RegisterDocumentFont(getDocIndex(), _container, item->getUrl(), item->getFace(), item->getBold(), item->getItalic())) {
				CRLog::error("Failed to register document font face: %s file: %s", item->getFace().c_str(), LCSTR(item->getUrl()));
			lString16Collection flist;
			fontMan->getFaceList(flist);
			int cnt = flist.length();
			lString16 fontface = lString16("");
				CRLog::debug("fontlist has %d fontfaces\n", cnt);
			for (int j = 0; j < cnt; j = j + 1) {
				fontface = flist[j];
				do { (fontface.replace(lString16(" "), lString16("\0"))); }
				while (fontface.pos(lString16(" ")) != -1);
				if (fontface.lowercase().pos(url.lowercase()) != -1) {
					CRLog::debug("****found %s\n", UnicodeToLocal(fontface).c_str());
					fontMan->setalias(face, UnicodeToLocal(flist[j]), getDocIndex(),item->getItalic(),item->getBold()) ;
					break;
				}
			}
			}
		}
	}
}

/// returns object image stream
LVStreamRef ldomDocument::getObjectImageStream( lString16 refName )
{
	LVStreamRef ref;
	if ( refName.startsWith(lString16(BLOB_NAME_PREFIX)) ) {
		return _blobCache.getBlob(refName);
	} if ( refName[0]!='#' ) {
		if ( !getContainer().isNull() ) {
			lString16 name = refName;
			if ( !getCodeBase().empty() )
				name = getCodeBase() + refName;
			ref = getContainer()->OpenStream(name.c_str(), LVOM_READ);
			if ( ref.isNull() ) {
				lString16 fname = getProps()->getStringDef( DOC_PROP_FILE_NAME, "" );
				fname = LVExtractFilenameWithoutExtension(fname);
				if ( !fname.empty() ) {
					lString16 fn = fname + "_img";
//                    if ( getContainer()->GetObjectInfo(fn) ) {

//                    }
					lString16 name = fn + "/" + refName;
					if ( !getCodeBase().empty() )
						name = getCodeBase() + name;
					ref = getContainer()->OpenStream(name.c_str(), LVOM_READ);
				}
			}
			if ( ref.isNull() )
				CRLog::error("Cannot open stream by name %s", LCSTR(name));
		}
		return ref;
	}
	lUInt16 refValueId = findAttrValueIndex( refName.c_str() + 1 );
	if ( refValueId == (lUInt16)-1 ) {
		return ref;
	}
	ldomNode * objnode = getNodeById( refValueId );
	if ( !objnode || !objnode->isElement())
		return ref;
	ref = objnode->createBase64Stream();
	return ref;
}

/// returns object image source
LVImageSourceRef ldomDocument::getObjectImageSource( lString16 refName )
{
	LVStreamRef stream = getObjectImageStream( refName );
	if (stream.isNull())
		 return LVImageSourceRef();
	return LVCreateStreamImageSource( stream );
}

void ldomDocument::resetNodeNumberingProps()
{
	lists.clear();
}

ListNumberingPropsRef ldomDocument::getNodeNumberingProps( lUInt32 nodeDataIndex )
{
	return lists.get(nodeDataIndex);
}

void ldomDocument::setNodeNumberingProps( lUInt32 nodeDataIndex, ListNumberingPropsRef v )
{
	lists.set(nodeDataIndex, v);
}

ldomDocument::ldomDocument()
: m_toc(this)
#if BUILD_LITE!=1
, _last_docflags(0)
, _page_height(0)
, _page_width(0)
, _rendered(false)
#endif
, lists(100)
{
	allocTinyElement(NULL, 0, 0);
	//new ldomElement( this, NULL, 0, 0, 0 );
	//assert( _instanceMapCount==2 );
}

/// creates empty document which is ready to be copy target of doc partial contents
ldomDocument::ldomDocument( ldomDocument & doc )
: lxmlDocBase(doc)
, m_toc(this)
#if BUILD_LITE!=1
, _def_font(doc._def_font) // default font
, _def_style(doc._def_style)
, _last_docflags(doc._last_docflags)
, _page_height(doc._page_height)
, _page_width(doc._page_width)
#endif
, _container(doc._container)
, lists(100)
{
}

bool ldomDocument::saveToStream( LVStreamRef stream, const char *, bool treeLayout )
{
	//CRLog::trace("ldomDocument::saveToStream()");
	if (!stream || !getRootNode()->getChildCount())
		return false;

	*stream.get() << UnicodeToLocal(cs16(L"\xFEFF"));
	writeNode( stream.get(), getRootNode(), treeLayout );
	return true;
}

ldomDocument::~ldomDocument()
{
	LVFontManager::getInstance()->UnregisterDocumentFonts(_docIndex);
#if BUILD_LITE!=1
	updateMap();
#endif
}

/// renders (formats) document in memory
bool ldomDocument::setRenderProps( int width, int dy, bool /*showCover*/, int /*y0*/, font_ref_t def_font, int def_interline_space, CRPropRef props )
{
	bool changed = false;
	_renderedBlockCache.clear();
	changed = _imgScalingOptions.update(props, def_font->getSize()) || changed;
	css_style_ref_t s( new css_style_rec_t );
	s->display = css_d_block;
	s->white_space = css_ws_normal;
	s->text_align = css_ta_left;
	s->text_align_last = css_ta_left;
	s->text_decoration = css_td_none;
	s->hyphenate = css_hyph_auto;
	s->color.type = css_val_unspecified;
	s->color.value = 0x000000;
	s->background_color.type = css_val_unspecified;
	s->background_color.value = 0xFFFFFF;
	//_def_style->background_color.type = color;
	//_def_style->background_color.value = 0xFFFFFF;
	s->page_break_before = css_pb_auto;
	s->page_break_after = css_pb_auto;
	s->page_break_inside = css_pb_auto;
	s->list_style_type = css_lst_disc;
	s->list_style_position = css_lsp_outside;
	s->vertical_align = css_va_baseline;
	s->font_family = def_font->getFontFamily();
	s->font_size.type = css_val_px;
	s->font_size.value = def_font->getSize();
	s->font_name = def_font->getTypeFace();
	s->font_weight = css_fw_400;
	s->font_style = css_fs_normal;
	s->text_indent.type = css_val_px;
	s->text_indent.value = 0;
	s->line_height.type = css_val_percent;
	s->line_height.value = def_interline_space;
	//lUInt32 defStyleHash = (((_stylesheet.getHash() * 31) + calcHash(_def_style))*31 + calcHash(_def_font));
	//defStyleHash = defStyleHash * 31 + getDocFlags();
	if ( _last_docflags != getDocFlags() ) {
		CRLog::trace("ldomDocument::setRenderProps() - doc flags changed");
		_last_docflags = getDocFlags();
		changed = true;
	}
	if ( calcHash(_def_style) != calcHash(s) ) {
		CRLog::trace("ldomDocument::setRenderProps() - style is changed");
		_def_style = s;
		changed = true;
	}
	if ( calcHash(_def_font) != calcHash(def_font)) {
		CRLog::trace("ldomDocument::setRenderProps() - font is changed");
		_def_font = def_font;
		changed = true;
	}
	if ( _page_height != dy ) {
		CRLog::trace("ldomDocument::setRenderProps() - page height is changed: %d != %d", _page_height, dy);
		_page_height = dy;
		changed = true;
	}
	if ( _page_width != width ) {
		CRLog::trace("ldomDocument::setRenderProps() - page width is changed");
		_page_width = width;
		changed = true;
	}
//    {
//        lUInt32 styleHash = calcStyleHash();
//        styleHash = styleHash * 31 + calcGlobalSettingsHash();
//        CRLog::debug("Style hash before set root style: %x", styleHash);
//    }
//    getRootNode()->setFont( _def_font );
//    getRootNode()->setStyle( _def_style );
//    {
//        lUInt32 styleHash = calcStyleHash();
//        styleHash = styleHash * 31 + calcGlobalSettingsHash();
//        CRLog::debug("Style hash after set root style: %x", styleHash);
//    }
	return changed;
}

#if BUILD_LITE!=1

void ldomDocument::applyDocumentStyleSheet()
{
	if ( !getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES) ) {
		CRLog::trace("applyDocumentStyleSheet() : DOC_FLAG_ENABLE_INTERNAL_STYLES is disabled");
		return;
	}
	if ( !_docStylesheetFileName.empty() ) {
		if ( getContainer().isNull() )
			return;
		if ( parseStyleSheet(_docStylesheetFileName) ) {
			CRLog::debug("applyDocumentStyleSheet() : Using document stylesheet from link/stylesheet from %s",
						 LCSTR(_docStylesheetFileName));
		}
	} else {
		ldomXPointer ss = createXPointer(cs16("/FictionBook/stylesheet"));
		if ( !ss.isNull() ) {
			lString16 css = ss.getText('\n');
			if ( !css.empty() ) {
				CRLog::debug("applyDocumentStyleSheet() : Using internal FB2 document stylesheet:\n%s", LCSTR(css));
				_stylesheet.parse(LCSTR(css));
			} else {
				CRLog::trace("applyDocumentStyleSheet() : stylesheet under /FictionBook/stylesheet is empty");
			}
		} else {
			CRLog::trace("applyDocumentStyleSheet() : No internal FB2 stylesheet found under /FictionBook/stylesheet");
		}
	}
}

bool ldomDocument::parseStyleSheet(lString16 codeBase, lString16 css)
{
	LVImportStylesheetParser parser(this);
	return parser.Parse(codeBase, css);
}

bool ldomDocument::parseStyleSheet(lString16 cssFile)
{
	LVImportStylesheetParser parser(this);
	return parser.Parse(cssFile);
}

int ldomDocument::render( LVRendPageList * pages, LVDocViewCallback * callback, int width, int dy, bool showCover, int y0, font_ref_t def_font, int def_interline_space, CRPropRef props )
{
	CRLog::info("Render is called for width %d, pageHeight=%d, fontFace=%s, docFlags=%d", width, dy, def_font->getTypeFace().c_str(), getDocFlags() );
	CRLog::trace("initializing default style...");
	//persist();
//    {
//        lUInt32 styleHash = calcStyleHash();
//        styleHash = styleHash * 31 + calcGlobalSettingsHash();
//        CRLog::debug("Style hash before setRenderProps: %x", styleHash);
//    } //bool propsChanged =
	setRenderProps( width, dy, showCover, y0, def_font, def_interline_space, props );

	// update styles
//    if ( getRootNode()->getStyle().isNull() || getRootNode()->getFont().isNull()
//        || _docFlags != _hdr.render_docflags
//        || width!=_hdr.render_dx || dy!=_hdr.render_dy || defStyleHash!=_hdr.stylesheet_hash ) {
//        CRLog::trace("init format data...");
//        getRootNode()->recurseElements( initFormatData );
//    } else {
//        CRLog::trace("reusing existing format data...");
//    }

	if ( !checkRenderContext() ) {
		CRLog::info("rendering context is changed - full render required...");
		CRLog::trace("init format data...");
		//CRLog::trace("validate 1...");
		//validateDocument();
		CRLog::trace("Dropping existing styles...");
		//CRLog::debug( "root style before drop style %d", getNodeStyleIndex(getRootNode()->getDataIndex()));
		dropStyles();
		//CRLog::debug( "root style after drop style %d", getNodeStyleIndex(getRootNode()->getDataIndex()));

		//ldomNode * root = getRootNode();
		//css_style_ref_t roots = root->getStyle();
		//CRLog::trace("validate 2...");
		//validateDocument();

		CRLog::trace("Save stylesheet...");
		_stylesheet.push();
		CRLog::trace("Init node styles...");
		applyDocumentStyleSheet();
		getRootNode()->initNodeStyleRecursive();
		CRLog::trace("Restoring stylesheet...");
		_stylesheet.pop();

		CRLog::trace("init render method...");
		getRootNode()->initNodeRendMethodRecursive();

//        getRootNode()->setFont( _def_font );
//        getRootNode()->setStyle( _def_style );
		updateRenderContext();

		// DEBUG dump of render methods
		//dumpRendMethods( getRootNode(), cs16(" - ") );
//        lUInt32 styleHash = calcStyleHash();
//        styleHash = styleHash * 31 + calcGlobalSettingsHash();
//        CRLog::debug("Style hash: %x", styleHash);

		_rendered = false;
	}
	if ( !_rendered ) {
		pages->clear();
		if ( showCover )
			pages->add( new LVRendPageInfo( _page_height ) );
		LVRendPageContext context( pages, _page_height );
		int numFinalBlocks = calcFinalBlocks();
		CRLog::info("Final block count: %d", numFinalBlocks);
		context.setCallback(callback, numFinalBlocks);
		//updateStyles();
		CRLog::trace("rendering...");
		int height = renderBlockElement( context, getRootNode(),
			0, y0, width ) + y0;
		_rendered = true;
	#if 0 //def _DEBUG
		LVStreamRef ostream = LVOpenFileStream( "test_save_after_init_rend_method.xml", LVOM_WRITE );
		saveToStream( ostream, "utf-16" );
	#endif
		gc();
		CRLog::trace("finalizing... fonts.length=%d", _fonts.length());
		context.Finalize();
		updateRenderContext();
		_pagesData.reset();
		pages->serialize( _pagesData );

		if ( callback ) {
			callback->OnFormatEnd();
		}

		//saveChanges();

		//persist();
		dumpStatistics();
		return height;
	} else {
		CRLog::info("rendering context is not changed - no render!");
		if ( _pagesData.pos() ) {
			_pagesData.setPos(0);
			pages->deserialize( _pagesData );
		}
		CRLog::info("%d rendered pages found", pages->length() );
		return getFullHeight();
	}

}

#endif
