/** \file changeinfo.cpp
    \brief bookmark/position change info for synchronization/replication

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2007
    This source code is distributed under the terms of
    GNU General Public License
    See LICENSE file for details

    This class is unused, saved for history!
*/

#include "../include/lvtinydom.h"
#include "../include/hist.h"
#include "../include/changeinfo.h"

#define START_TAG            "# start record"
#define END_TAG              "# end record"
#define START_TAG_BYTES      "# start record\n"
#define END_TAG_BYTES        "# end record\n"
#define ACTION_TAG           "ACTION"
#define ACTION_DELETE_TAG    "DELETE"
#define ACTION_UPDATE_TAG    "UPDATE"
#define FILE_TAG             "FILE"
#define TYPE_TAG             "TYPE"
#define START_POS_TAG        "STARTPOS"
#define END_POS_TAG          "ENDPOS"
#define TIMESTAMP_TAG        "TIMESTAMP"
#define PERCENT_TAG          "PERCENT"
#define SHORTCUT_TAG         "SHORTCUT"
#define TITLE_TEXT_TAG       "TITLETEXT"
#define POS_TEXT_TAG         "POSTEXT"
#define COMMENT_TEXT_TAG     "COMMENTTEXT"

static lString8 encodeText(lString16 text16) {
    if (text16.empty())
        return lString8::empty_str;
    lString8 text = UnicodeToUtf8(text16);
    lString8 buf;
    for (int i=0; i<text.length(); i++) {
        char ch = text[i];
        switch (ch) {
        case '\\':
            buf << "\\\\";
            break;
        case '\n':
            buf << "\\n";
            break;
        case '\r':
            buf << "\\r";
            break;
        case '\t':
            buf << "\\t";
            break;
        default:
            buf << ch;
            break;
        }
    }
    return buf;
}

static lString16 decodeText(lString8 text) {
    if (text.empty())
        return lString16::empty_str;
    lString8 buf;
    bool lastControl = false;
    for (int i=0; i<text.length(); i++) {
        char ch = buf[i];
        if (lastControl) {
            switch (ch) {
            case 'r':
                buf.append(1, '\r');
                break;
            case 'n':
                buf.append(1, '\n');
                break;
            case 't':
                buf.append(1, '\t');
                break;
            default:
                buf.append(1, ch);
                break;
            }
            lastControl = false;
            continue;
        }
        if (ch == '\\') {
            lastControl = true;
            continue;
        }
        buf.append(1, ch);
    }
    return Utf8ToUnicode(buf);
}

static int findBytes(lChar8 * buf, int start, int end, const lChar8 * pattern) {
    int len = lStr_len(pattern);
    for (int i = start; i <= end - len; i++) {
        int j = 0;
        for (; j < len; j++) {
            if (buf[i+j] != pattern[j])
                break;
        }
        if (j == len)
            return i;
    }
    return -1;
}

ChangeInfo::ChangeInfo(CRBookmark * bookmark, lString16 fileName, bool deleted)
    : _bookmark(bookmark ? new CRBookmark(*bookmark) : NULL), _fileName(fileName), _deleted(deleted)
{
    _timestamp = bookmark && bookmark->getTimestamp() > 0 ? bookmark->getTimestamp() : (time_t)time(0);
}

lString8 ChangeInfo::toString() {
    lString8 buf;
    buf << START_TAG << "\n";
    buf << FILE_TAG << "=" << encodeText(_fileName) << "\n";
    buf << ACTION_TAG << "=" << (_deleted ? ACTION_DELETE_TAG : ACTION_UPDATE_TAG) << "\n";
    buf << TIMESTAMP_TAG << "=" << fmt::decimal(_timestamp * 1000) << "\n";
    if (_bookmark) {
        buf << TYPE_TAG << "=" << fmt::decimal(_bookmark->getType()) << "\n";
        buf << START_POS_TAG << "=" << encodeText(_bookmark->getStartPos()) << "\n";
        buf << END_POS_TAG << "=" << encodeText(_bookmark->getEndPos()) << "\n";
        buf << PERCENT_TAG << "=" << fmt::decimal(_bookmark->getPercent()) << "\n";
        buf << SHORTCUT_TAG << "=" << fmt::decimal(_bookmark->getShortcut()) << "\n";
        buf << TITLE_TEXT_TAG << "=" << encodeText(_bookmark->getTitleText()) << "\n";
        buf << POS_TEXT_TAG << "=" << encodeText(_bookmark->getPosText()) << "\n";
        buf << COMMENT_TEXT_TAG << "=" << encodeText(_bookmark->getCommentText()) << "\n";
    }
    buf << END_TAG << "\n";
    return buf;
}

ChangeInfo * ChangeInfo::fromString(lString8 s) {
    lString8Collection rows(s, cs8("\n"));
    if (rows.length() < 3 || rows[0] != START_TAG || rows[rows.length() - 1] != END_TAG)
        return NULL;
    ChangeInfo * ci = new ChangeInfo();
    CRBookmark bmk;
    for (int i=1; i<rows.length() - 1; i++) {
        lString8 row = rows[i];
        int p = row.pos("=");
        if (p<1)
            continue;
        lString8 name = row.substr(0, p);
        lString8 value = row.substr(p + 1);
        if (name == ACTION_TAG) {
            ci->_deleted = (value == ACTION_DELETE_TAG);
        } else if (name == FILE_TAG) {
            ci->_fileName = decodeText(value);
        } else if (name == TYPE_TAG) {
            bmk.setType(value.atoi());
        } else if (name == START_POS_TAG) {
            bmk.setStartPos(decodeText(value));
        } else if (name == END_POS_TAG) {
            bmk.setEndPos(decodeText(value));
        } else if (name == TIMESTAMP_TAG) {
            ci->_timestamp = value.atoi64() / 1000;
            bmk.setTimestamp(ci->_timestamp);
        } else if (name == PERCENT_TAG) {
            bmk.setPercent(value.atoi());
        } else if (name == SHORTCUT_TAG) {
            bmk.setShortcut(value.atoi());
        } else if (name == TITLE_TEXT_TAG) {
            bmk.setTitleText(decodeText(value));
        } else if (name == POS_TEXT_TAG) {
            bmk.setPosText(decodeText(value));
        } else if (name == COMMENT_TEXT_TAG) {
            bmk.setCommentText(decodeText(value));
        }
    }
    if (bmk.isValid())
        ci->_bookmark = new CRBookmark(bmk);
    if (ci->_fileName.empty() || ci->_timestamp == 0 || (!ci->_bookmark && !ci->_deleted)) {
        delete ci;
        return NULL;
    }
    return ci;
}

ChangeInfo * ChangeInfo::fromBytes(lChar8 * buf, int start, int end) {
    lString8 s(buf + start, end - start);
    return fromString(s);
}

bool ChangeInfo::findNextRecordBounds(lChar8 * buf, int start, int end, int & recordStart, int & recordEnd) {
    int startTagPos = findBytes(buf, start, end, START_TAG_BYTES);
    if (startTagPos < 0)
        return false;
    int endTagPos = findBytes(buf, startTagPos, end, END_TAG_BYTES);
    if (endTagPos < 0)
        return false;
    recordStart = startTagPos;
    recordEnd = endTagPos + lStr_len(END_TAG_BYTES);
    return true;
}
