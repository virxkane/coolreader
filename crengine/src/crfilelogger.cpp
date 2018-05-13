/*******************************************************

   CoolReader Engine

   crfilelogger.cpp:  logger class implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/crfilelogger.h"
#include "../include/lvtypes.h"

#include <time.h>
#ifdef LINUX
#include <sys/time.h>
#endif


static lUInt64 GetCurrentTimeMillis();


void CRFileLogger::log(const char *level, const char *msg, va_list args)
{
	if ( !f )
		return;
#ifdef LINUX
	struct timeval tval;
	gettimeofday( &tval, NULL );
	int ms = tval.tv_usec;
	time_t t = tval.tv_sec;
#if LOG_HEAP_USAGE
	struct mallinfo mi = mallinfo();
	int memusage = mi.arena;
#endif
#else
	lUInt64 ts = GetCurrentTimeMillis();
	//time_t t = (time_t)time(0);
	time_t t = ts / 1000;
	int ms = (ts % 1000) * 1000;
#if LOG_HEAP_USAGE
	int memusage = 0;
#endif
#endif
	tm * bt = localtime(&t);
#if LOG_HEAP_USAGE
	fprintf(f, "%04d/%02d/%02d %02d:%02d:%02d.%04d [%d] %s ", bt->tm_year+1900, bt->tm_mon+1, bt->tm_mday, bt->tm_hour, bt->tm_min, bt->tm_sec, ms/100, memusage, level);
#else
	fprintf(f, "%04d/%02d/%02d %02d:%02d:%02d.%04d %s ", bt->tm_year+1900, bt->tm_mon+1, bt->tm_mday, bt->tm_hour, bt->tm_min, bt->tm_sec, ms/100, level);
#endif
	vfprintf( f, msg, args );
	fprintf(f, "\n" );
	if ( autoFlush )
		fflush( f );
}

CRFileLogger::CRFileLogger(FILE *file, bool _autoClose, bool _autoFlush)
	: f(file), autoClose(_autoClose), autoFlush( _autoFlush )
{
	info( "Started logging" );
}

CRFileLogger::CRFileLogger(const char *fname, bool _autoFlush)
	: f(fopen( fname, "wt" )), autoClose(true), autoFlush( _autoFlush )
{
	static unsigned char utf8sign[] = {0xEF, 0xBB, 0xBF};
	static const char * log_level_names[] = {
		"FATAL",
		"ERROR",
		"WARN",
		"INFO",
		"DEBUG",
		"TRACE",
	};
	fwrite( utf8sign, 3, 1, f);
	info( "Started logging. Level=%s", log_level_names[getLogLevel()] );
}

CRFileLogger::~CRFileLogger() {
	if ( f && autoClose ) {
		info( "Stopped logging" );
		fclose( f );
	}
	f = NULL;
}

#ifdef _WIN32
static bool __timerInitialized = false;
static double __timeTicksPerMillis;
static lUInt64 __timeStart;
static lUInt64 __timeAbsolute;
static lUInt64 __startTimeMillis;
#endif

static void CRReinitTimer() {
#ifdef _WIN32
    LARGE_INTEGER tps;
    QueryPerformanceFrequency(&tps);
    __timeTicksPerMillis = (double)(tps.QuadPart / 1000L);
    LARGE_INTEGER queryTime;
    QueryPerformanceCounter(&queryTime);
    __timeStart = (lUInt64)(queryTime.QuadPart / __timeTicksPerMillis);
    __timerInitialized = true;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    __startTimeMillis = (ft.dwLowDateTime | (((lUInt64)ft.dwHighDateTime) << 32)) / 10000;
#else
    // do nothing. it's for win32 only
#endif
}


static lUInt64 GetCurrentTimeMillis() {
#if defined(LINUX) || defined(ANDROID) || defined(_LINUX)
    timeval ts;
    gettimeofday(&ts, NULL);
    return ts.tv_sec * (lUInt64)1000 + ts.tv_usec / 1000;
#else
 #ifdef _WIN32
    if (!__timerInitialized) {
        CRReinitTimer();
        return __startTimeMillis;
    } else {
        LARGE_INTEGER queryTime;
        QueryPerformanceCounter(&queryTime);
        __timeAbsolute = (lUInt64)(queryTime.QuadPart / __timeTicksPerMillis);
        return __startTimeMillis + (lUInt64)(__timeAbsolute - __timeStart);
    }
 #else
 #error * You should define GetCurrentTimeMillis() *
 #endif
#endif
}
