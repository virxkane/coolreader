/** \file lvdomimpstlparser_p.h
	\brief fast and compact XML DOM tree
	Private header, class LVImportStylesheetParser

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2009
	This source code is distributed under the terms of
	GNU General Public License
	See LICENSE file for details
*/

#ifndef __LV_DOMIMPSTLPARSER_P_H_INCLUDED__
#define __LV_DOMIMPSTLPARSER_P_H_INCLUDED__

#include "crsetup.h"
#include "lvstring.h"
#include "lvstring16collection.h"

class ldomDocument;

class LVImportStylesheetParser
{
public:
	LVImportStylesheetParser(ldomDocument *document) :
		_document(document), _nestingLevel(0)
	{
	}
	~LVImportStylesheetParser()
	{
		_inProgress.clear();
	}
	bool Parse(lString16 cssFile);
	bool Parse(lString16 codeBase, lString16 css);
private:
	ldomDocument *_document;
	lString16Collection _inProgress;
	int _nestingLevel;
};

#endif	// __LV_DOMIMPSTLPARSER_P_H_INCLUDED__
