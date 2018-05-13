/*******************************************************

   CoolReader Engine

   crlog.cpp:  logger class implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/crlog.h"
#include "../include/crfilelogger.h"

CRLog * CRLog::CRLOG = NULL;

void CRLog::setLogger( CRLog * logger )
{
    if ( CRLOG!=NULL ) {
        delete CRLOG;
    }
    CRLOG = logger;
}

void CRLog::setLogLevel( CRLog::log_level level )
{
    if ( !CRLOG )
        return;
    warn( "Changing log level from %d to %d", (int)CRLOG->curr_level, (int)level );
    CRLOG->curr_level = level;
}

CRLog::log_level CRLog::getLogLevel()
{
    if ( !CRLOG )
        return LL_INFO;
    return CRLOG->curr_level;
}

bool CRLog::isLogLevelEnabled( CRLog::log_level level )
{
    if ( !CRLOG )
        return false;
    return (CRLOG->curr_level >= level);
}

void CRLog::fatal( const char * msg, ... )
{
    if ( !CRLOG )
        return;
    va_list args;
    va_start( args, msg );
    CRLOG->log( "FATAL", msg, args );
    va_end(args);
}

void CRLog::error( const char * msg, ... )
{
    if ( !CRLOG || CRLOG->curr_level<LL_ERROR )
        return;
    va_list args;
    va_start( args, msg );
    CRLOG->log( "ERROR", msg, args );
    va_end(args);
}

void CRLog::warn( const char * msg, ... )
{
    if ( !CRLOG || CRLOG->curr_level<LL_WARN )
        return;
    va_list args;
    va_start( args, msg );
    CRLOG->log( "WARN", msg, args );
    va_end(args);
}

void CRLog::info( const char * msg, ... )
{
    if ( !CRLOG || CRLOG->curr_level<LL_INFO )
        return;
    va_list args;
    va_start( args, msg );
    CRLOG->log( "INFO", msg, args );
    va_end(args);
}

void CRLog::debug( const char * msg, ... )
{
    if ( !CRLOG || CRLOG->curr_level<LL_DEBUG )
        return;
    va_list args;
    va_start( args, msg );
    CRLOG->log( "DEBUG", msg, args );
    va_end(args);
}

void CRLog::trace( const char * msg, ... )
{
    if ( !CRLOG || CRLOG->curr_level<LL_TRACE )
        return;
    va_list args;
    va_start( args, msg );
    CRLOG->log( "TRACE", msg, args );
    va_end(args);
}

CRLog::CRLog()
    : curr_level(LL_INFO)
{
}

CRLog::~CRLog()
{
}

void CRLog::setFileLogger( const char * fname, bool autoFlush )
{
    setLogger( new CRFileLogger( fname, autoFlush ) );
}

void CRLog::setStdoutLogger()
{
    setLogger( new CRFileLogger( (FILE*)stdout, false, true ) );
}

void CRLog::setStderrLogger()
{
    setLogger( new CRFileLogger( (FILE*)stderr, false, true ) );
}
