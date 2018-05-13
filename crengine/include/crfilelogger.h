/** \file crlog.h
	\brief logger class interface

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006
	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.
*/

#ifndef __CR_FILELOGGER_H_INCLUDED__
#define __CR_FILELOGGER_H_INCLUDED__

#include "../include/crlog.h"

#include <stdio.h>

#ifndef LOG_HEAP_USAGE
#define LOG_HEAP_USAGE 0
#endif

class CRFileLogger : public CRLog
{
protected:
	FILE * f;
	bool autoClose;
	bool autoFlush;
	virtual void log( const char * level, const char * msg, va_list args );
public:
	CRFileLogger( FILE * file, bool _autoClose, bool _autoFlush );
	CRFileLogger( const char * fname, bool _autoFlush );
	virtual ~CRFileLogger();
};

#endif	// __CR_FILELOGGER_H_INCLUDED__
