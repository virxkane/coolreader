/** \file lvembeddedfont.h

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006

	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.

*/

#ifndef __LV_EMBEDDEDFONT_H_INCLUDED__
#define __LV_EMBEDDEDFONT_H_INCLUDED__

#include "lvptrvec.h"
#include "lvstring.h"
#include "serialbuf.h"

class LVEmbeddedFontDef {
	lString16 _url;
	lString8 _face;
	bool _bold;
	bool _italic;
public:
	LVEmbeddedFontDef(lString16 url, lString8 face, bool bold, bool italic) :
		_url(url), _face(face), _bold(bold), _italic(italic)
	{
	}
	LVEmbeddedFontDef() : _bold(false), _italic(false) {
	}

	const lString16 & getUrl() const { return _url; }
	const lString8 & getFace() const { return _face; }
	bool getBold() const { return _bold; }
	bool getItalic() const { return _italic; }
	void setFace(const lString8 &  face) { _face = face; }
	void setBold(bool bold) { _bold = bold; }
	void setItalic(bool italic) { _italic = italic; }
	bool serialize(SerialBuf & buf) const;
	bool deserialize(SerialBuf & buf);
};

class LVEmbeddedFontList : public LVPtrVector<LVEmbeddedFontDef> {
public:
	LVEmbeddedFontDef * findByUrl(lString16 url);
	void add(LVEmbeddedFontDef * def) { LVPtrVector<LVEmbeddedFontDef>::add(def); }
	bool add(lString16 url, lString8 face, bool bold, bool italic);
	bool add(lString16 url) { return add(url, lString8::empty_str, false, false); }
	bool addAll(LVEmbeddedFontList & list);
	void set(LVEmbeddedFontList & list) { clear(); addAll(list); }
	bool serialize(SerialBuf & buf) const;
	bool deserialize(SerialBuf & buf);
};

#endif	// __LV_EMBEDDEDFONT_H_INCLUDED__
