/*******************************************************

   CoolReader Engine

   ldomdocfragwriter.cpp: fast and compact XML DOM tree:  ldomDocumentFragmentWriter

   (c) Vadim Lopatin, 2000-2011
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvdomdocfragwriter.h"
#include "../include/lvstream.h"
#include "../include/crlog.h"

lString16 ldomDocumentFragmentWriter::convertId( lString16 id )
{
    if ( !codeBasePrefix.empty() ) {
        return codeBasePrefix + "_" + id;
    }
    return id;
}

lString16 ldomDocumentFragmentWriter::convertHref( lString16 href )
{
    if ( href.pos("://")>=0 )
        return href; // fully qualified href: no conversion

    //CRLog::trace("convertHref(%s, codeBase=%s, filePathName=%s)", LCSTR(href), LCSTR(codeBase), LCSTR(filePathName));

    if (href[0] == '#') {
        lString16 replacement = pathSubstitutions.get(filePathName);
        if (replacement.empty())
            return href;
        lString16 p = cs16("#") + replacement + "_" + href.substr(1);
        //CRLog::trace("href %s -> %s", LCSTR(href), LCSTR(p));
        return p;
    }

    href = LVCombinePaths(codeBase, href);

    // resolve relative links
    lString16 p, id;
    if ( !href.split2(cs16("#"), p, id) )
        p = href;
    if ( p.empty() ) {
        //CRLog::trace("codebase = %s -> href = %s", LCSTR(codeBase), LCSTR(href));
        if ( codeBasePrefix.empty() )
            return href;
        p = codeBasePrefix;
    } else {
        lString16 replacement = pathSubstitutions.get(p);
        //CRLog::trace("href %s -> %s", LCSTR(p), LCSTR(replacement));
        if ( !replacement.empty() )
            p = replacement;
        else
            return href;
        //else
        //    p = codeBasePrefix;
        //p = LVCombinePaths( codeBase, p ); // relative to absolute path
    }
    if ( !id.empty() )
        p = p + "_" + id;

    p = cs16("#") + p;

    //CRLog::debug("converted href=%s to %s", LCSTR(href), LCSTR(p) );

    return p;
}

void ldomDocumentFragmentWriter::setCodeBase( lString16 fileName )
{
    filePathName = fileName;
    codeBasePrefix = pathSubstitutions.get(fileName);
    codeBase = LVExtractPath(filePathName);
    if ( codeBasePrefix.empty() ) {
        CRLog::trace("codeBasePrefix is empty for path %s", LCSTR(fileName));
        codeBasePrefix = pathSubstitutions.get(fileName);
    }
    stylesheetFile.clear();
}

/// called on attribute
void ldomDocumentFragmentWriter::OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue )
{
    if ( insideTag ) {
        if ( !lStr_cmp(attrname, "href") || !lStr_cmp(attrname, "src") ) {
            parent->OnAttribute(nsname, attrname, convertHref(lString16(attrvalue)).c_str() );
        } else if ( !lStr_cmp(attrname, "id") ) {
            parent->OnAttribute(nsname, attrname, convertId(lString16(attrvalue)).c_str() );
        } else if ( !lStr_cmp(attrname, "name") ) {
            //CRLog::trace("name attribute = %s", LCSTR(lString16(attrvalue)));
            parent->OnAttribute(nsname, attrname, convertId(lString16(attrvalue)).c_str() );
        } else {
            parent->OnAttribute(nsname, attrname, attrvalue);
        }
    } else {
        if ( styleDetectionState ) {
            if ( !lStr_cmp(attrname, "rel") && !lStr_cmp(attrvalue, "stylesheet") )
                styleDetectionState |= 2;
            else if ( !lStr_cmp(attrname, "type") ) {
                if ( !lStr_cmp(attrvalue, "text/css") )
                    styleDetectionState |= 4;
                else
                    styleDetectionState = 0;  // text/css type supported only
            } else if ( !lStr_cmp(attrname, "href") ) {
                styleDetectionState |= 8;
                lString16 href = attrvalue;
                if ( stylesheetFile.empty() )
                    tmpStylesheetFile = LVCombinePaths( codeBase, href );
                else
                    tmpStylesheetFile = href;
            }
            if (styleDetectionState == 15) {
                if ( !stylesheetFile.empty() )
                    stylesheetLinks.add(tmpStylesheetFile);
                else
                    stylesheetFile = tmpStylesheetFile;
                styleDetectionState = 0;
                CRLog::trace("CSS file href: %s", LCSTR(stylesheetFile));
            }
        }
    }
}

/// called on opening tag
ldomNode * ldomDocumentFragmentWriter::OnTagOpen( const lChar16 * nsname, const lChar16 * tagname )
{
    if ( insideTag ) {
        return parent->OnTagOpen(nsname, tagname);
    } else {
        if ( !lStr_cmp(tagname, "link") )
            styleDetectionState = 1;
        if ( !lStr_cmp(tagname, "style") )
            headStyleState = 1;
    }
    if ( !insideTag && baseTag==tagname ) {
        insideTag = true;
        if ( !baseTagReplacement.empty() ) {
            baseElement = parent->OnTagOpen(L"", baseTagReplacement.c_str());
            lastBaseElement = baseElement;
            if ( !stylesheetFile.empty() ) {
                parent->OnAttribute(L"", L"StyleSheet", stylesheetFile.c_str() );
                CRLog::debug("Setting StyleSheet attribute to %s for document fragment", LCSTR(stylesheetFile) );
            }
            if ( !codeBasePrefix.empty() )
                parent->OnAttribute(L"", L"id", codeBasePrefix.c_str() );
            parent->OnTagBody();
            if ( !headStyleText.empty() || stylesheetLinks.length() > 0 ) {
                parent->OnTagOpen(L"", L"stylesheet");
                parent->OnAttribute(L"", L"href", codeBase.c_str() );
                lString16 imports;
                for (int i = 0; i < stylesheetLinks.length(); i++) {
                    lString16 import("@import url(\"");
                    import << stylesheetLinks.at(i);
                    import << "\");\n";
                    imports << import;
                }
                stylesheetLinks.clear();
                lString16 styleText = imports + headStyleText.c_str();
                parent->OnTagBody();
                parent->OnText(styleText.c_str(), styleText.length(), 0);
                parent->OnTagClose(L"", L"stylesheet");
            }
            // add base tag, too (e.g., in CSS, styles are often defined for body tag"
            parent->OnTagOpen(L"", baseTag.c_str());
            parent->OnTagBody();
            return baseElement;
        }
    }
    return NULL;
}

/// called on closing tag
void ldomDocumentFragmentWriter::OnTagClose( const lChar16 * nsname, const lChar16 * tagname )
{
    styleDetectionState = headStyleState = 0;
    if ( insideTag && baseTag==tagname ) {
        insideTag = false;
        if ( !baseTagReplacement.empty() ) {
            parent->OnTagClose(L"", baseTag.c_str());
            parent->OnTagClose(L"", baseTagReplacement.c_str());
        }
        baseElement = NULL;
        return;
    }
    if ( insideTag )
        parent->OnTagClose(nsname, tagname);
}

/// called after > of opening tag (when entering tag body) or just before /> closing tag for empty tags
void ldomDocumentFragmentWriter::OnTagBody()
{
    if ( insideTag ) {
        parent->OnTagBody();
    }
    if ( styleDetectionState == 11 ) {
        // incomplete <link rel="stylesheet", href="..." />; assuming type="text/css"
        if ( !stylesheetFile.empty() )
            stylesheetLinks.add(tmpStylesheetFile);
        else
            stylesheetFile = tmpStylesheetFile;
        styleDetectionState = 0;
    } else
        styleDetectionState = 0;
}
