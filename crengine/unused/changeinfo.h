/** \file changeinfo.h
    \brief bookmark/position change info for synchronization/replication

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2007
    This source code is distributed under the terms of
    GNU General Public License
    See LICENSE file for details

    This class is unused, saved for history!
*/

#ifndef CHANGEINFO_H_INCLUDED
#define CHANGEINFO_H_INCLUDED

#include "lvptrvec.h"
#include "hist.h"
#include <time.h>

/// bookmark/position change info for synchronization/replication
class ChangeInfo {
    CRBookmark * _bookmark;
    lString16 _fileName;
    bool _deleted;
    time_t _timestamp;

public:
    ChangeInfo() : _bookmark(NULL), _deleted(false), _timestamp(0) {
    }

    ChangeInfo(CRBookmark * bookmark, lString16 fileName, bool deleted);

    ~ChangeInfo() {
        if (_bookmark)
            delete _bookmark;
    }

    CRBookmark * getBookmark() { return _bookmark; }

    lString16 getFileName() { return _fileName; }

    bool isDeleted() { return _deleted; }

    time_t getTimestamp() { return _timestamp; }

    lString8 toString();

    static ChangeInfo * fromString(lString8 s);

    static ChangeInfo * fromBytes(lChar8 * buf, int start, int end);

    static bool findNextRecordBounds(lChar8 * buf, int start, int end, int & recordStart, int & recordEnd);
};

class ChangeInfoList : public LVPtrVector<ChangeInfo> {

};

#endif	// CHANGEINFO_H_INCLUDED
