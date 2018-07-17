/** \file lvdomdoc.h
	\brief fast and compact XML DOM tree: ldomDocument

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2009
	This source code is distributed under the terms of
	GNU General Public License
	See LICENSE file for details
*/


#ifndef __LV_DOMDOC_H_INCLUDED__
#define __LV_DOMDOC_H_INCLUDED__

#include "crsetup.h"
#include "lvstream.h"
#include "lvhashtable.h"
#include "lvembeddedfont.h"
#include "lvimg.h"

// TODO: replace on splitted parts
#include "../include/lvtinydom.h"

class ldomNode;
class LVRendPageList;

class ldomDocument : public lxmlDocBase
{
	friend class ldomDocumentWriter;
	friend class ldomDocumentWriterFilter;
private:
	LVTocItem m_toc;
#if BUILD_LITE!=1
	font_ref_t _def_font; // default font
	css_style_ref_t _def_style;
	lUInt32 _last_docflags;
	int _page_height;
	int _page_width;
	bool _rendered;
	ldomXRangeList _selections;
#endif

	lString16 _docStylesheetFileName;

	LVContainerRef _container;

	LVHashTable<lUInt32, ListNumberingPropsRef> lists;

	LVEmbeddedFontList _fontList;


#if BUILD_LITE!=1
	/// load document cache file content
	bool loadCacheFileContent(CacheLoadingCallback * formatCallback);

	/// save changes to cache file
	bool saveChanges();
	/// saves changes to cache file, limited by time interval (can be called again to continue after TIMEOUT)
	virtual ContinuousOperationResult saveChanges( CRTimerUtil & maxTime );
#endif

protected:

#if BUILD_LITE!=1
	void applyDocumentStyleSheet();
#endif

public:

	void forceReinitStyles() {
		dropStyles();
		_hdr.render_style_hash = 0;
		_rendered = false;
	}

#if BUILD_LITE!=1
	ListNumberingPropsRef getNodeNumberingProps( lUInt32 nodeDataIndex );
	void setNodeNumberingProps( lUInt32 nodeDataIndex, ListNumberingPropsRef v );
	void resetNodeNumberingProps();
#endif

#if BUILD_LITE!=1
	/// returns object image stream
	LVStreamRef getObjectImageStream( lString16 refName );
	/// returns object image source
	LVImageSourceRef getObjectImageSource( lString16 refName );

	bool isDefStyleSet()
	{
		return !_def_style.isNull();
	}

	/// return document's embedded font list
	LVEmbeddedFontList & getEmbeddedFontList() { return _fontList; }
	/// register embedded document fonts in font manager, if any exist in document
	void registerEmbeddedFonts();
#endif

	/// returns pointer to TOC root node
	LVTocItem * getToc() { return &m_toc; }

#if BUILD_LITE!=1
	/// save document formatting parameters after render
	void updateRenderContext();
	/// check document formatting parameters before render - whether we need to reformat; returns false if render is necessary
	bool checkRenderContext();
#endif

#if BUILD_LITE!=1
	/// try opening from cache file, find by source file name (w/o path) and crc32
	virtual bool openFromCache( CacheLoadingCallback * formatCallback );
	/// saves recent changes to mapped file
	virtual ContinuousOperationResult updateMap(CRTimerUtil & maxTime);
	/// swaps to cache file or saves changes, limited by time interval
	virtual ContinuousOperationResult swapToCache( CRTimerUtil & maxTime );
	/// saves recent changes to mapped file
	virtual bool updateMap() {
		CRTimerUtil infinite;
		return updateMap(infinite)!=CR_ERROR;
	}
#endif


	LVContainerRef getContainer() { return _container; }
	void setContainer( LVContainerRef cont ) { _container = cont; }

#if BUILD_LITE!=1
	void clearRendBlockCache() { _renderedBlockCache.clear(); }
#endif
	void clear();
	lString16 getDocStylesheetFileName() { return _docStylesheetFileName; }
	void setDocStylesheetFileName(lString16 fileName) { _docStylesheetFileName = fileName; }

	ldomDocument();
	/// creates empty document which is ready to be copy target of doc partial contents
	ldomDocument( ldomDocument & doc );

#if BUILD_LITE!=1
	/// return selections collection
	ldomXRangeList & getSelections() { return _selections; }

	/// get full document height
	int getFullHeight();
	/// returns page height setting
	int getPageHeight() { return _page_height; }
#endif
	/// saves document contents as XML to stream with specified encoding
	bool saveToStream( LVStreamRef stream, const char * codepage, bool treeLayout=false );
#if BUILD_LITE!=1
	/// get default font reference
	font_ref_t getDefaultFont() { return _def_font; }
	/// get default style reference
	css_style_ref_t getDefaultStyle() { return _def_style; }

	bool parseStyleSheet(lString16 codeBase, lString16 css);
	bool parseStyleSheet(lString16 cssFile);
#endif
	/// destructor
	virtual ~ldomDocument();
#if BUILD_LITE!=1
	/// renders (formats) document in memory
	virtual int render( LVRendPageList * pages, LVDocViewCallback * callback, int width, int dy, bool showCover, int y0, font_ref_t def_font, int def_interline_space, CRPropRef props );
	/// renders (formats) document in memory
	virtual bool setRenderProps( int width, int dy, bool showCover, int y0, font_ref_t def_font, int def_interline_space, CRPropRef props );
#endif
	/// create xpointer from pointer string
	ldomXPointer createXPointer( const lString16 & xPointerStr );
	/// create xpointer from pointer string
	ldomNode * nodeFromXPath( const lString16 & xPointerStr )
	{
		return createXPointer( xPointerStr ).getNode();
	}
	/// get element text by pointer string
	lString16 textFromXPath( const lString16 & xPointerStr )
	{
		ldomNode * node = nodeFromXPath( xPointerStr );
		if ( !node )
			return lString16::empty_str;
		return node->getText();
	}

	/// create xpointer from relative pointer string
	ldomXPointer createXPointer( ldomNode * baseNode, const lString16 & xPointerStr );
#if BUILD_LITE!=1
	/// create xpointer from doc point
	ldomXPointer createXPointer( lvPoint pt, int direction=0 );
	/// get rendered block cache object
	CVRendBlockCache & getRendBlockCache() { return _renderedBlockCache; }

	bool findText( lString16 pattern, bool caseInsensitive, bool reverse, int minY, int maxY, LVArray<ldomWord> & words, int maxCount, int maxHeight );
#endif
};

#endif	// __LV_DOMDOC_H_INCLUDED__
