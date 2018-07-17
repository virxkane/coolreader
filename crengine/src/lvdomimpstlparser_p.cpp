/*******************************************************

   CoolReader Engine

   lvdomimpstlparser_p.cpp: fast and compact XML DOM tree.
   Private class LVImportStylesheetParser.

   (c) Vadim Lopatin, 2000-2011
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvdomimpstlparser_p.h"
#include "../include/lvdomdoc.h"
#include "../include/lvstream.h"
#include "../include/lvxml.h"

bool LVImportStylesheetParser::Parse(lString16 cssFile)
{
	bool ret = false;
	if ( cssFile.empty() )
		return ret;
	
	lString16 codeBase = cssFile;
	LVExtractLastPathElement(codeBase);
	LVStreamRef cssStream = _document->getContainer()->OpenStream(cssFile.c_str(), LVOM_READ);
	if ( !cssStream.isNull() ) {
		lString16 css;
		css << LVReadTextFile( cssStream );
		int offset = _inProgress.add(cssFile);
		ret = Parse(codeBase, css) || ret;
		_inProgress.erase(offset, 1);
	}
	return ret;
}

bool LVImportStylesheetParser::Parse(lString16 codeBase, lString16 css)
{
	bool ret = false;
	if ( css.empty() )
		return ret;
	lString8 css8 = UnicodeToUtf8(css);
	const char * s = css8.c_str();
	
	_nestingLevel += 1;
	while (_nestingLevel < 11) { //arbitrary limit
		lString8 import_file;
		if ( LVProcessStyleSheetImport( s, import_file ) ) {
			lString16 importFilename = LVCombinePaths( codeBase, Utf8ToUnicode(import_file) );
			if ( !importFilename.empty() && !_inProgress.contains(importFilename) ) {
				ret = Parse(importFilename) || ret;
			}
		} else {
			break;
		}
	}
	_nestingLevel -= 1;
	return (_document->getStyleSheet()->parse(s) || ret);
}
