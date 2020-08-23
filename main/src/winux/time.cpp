#ifndef __GNUC__
#pragma warning( disable: 4786 )
#endif

#include "utilities.hpp"
#include "strings.hpp"
#include "time.hpp"
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <string.h>

#ifdef __GNUC__
#include <sys/time.h>
#endif

namespace winux
{
// class DateTimeL -------------------------------------------------------------------------------------------------------
DateTimeL::DateTimeL() : _millisec(0), _second(0), _minute(0), _hour(0), _day(0), _month(0), _year(0), _wday(0), _yday(0)
{

}

DateTimeL::DateTimeL( short year, short month, short day, short hour, short minute, short second, short millisec )
: _millisec(millisec), _second(second), _minute(minute), _hour(hour), _day(day), _month(month), _year(year), _wday(0), _yday(0)
{
    struct tm t = { 0 };
    t.tm_year = _year - 1900;
    t.tm_mon = _month - 1;
    t.tm_mday = _day;
    t.tm_hour = _hour;
    t.tm_min = _minute;
    t.tm_sec = _second;

    mktime(&t);

    _wday = t.tm_wday;
    _yday = t.tm_yday + 1;
}

DateTimeL::DateTimeL( ulong utcSecond ) : _millisec(0), _second(0), _minute(0), _hour(0), _day(0), _month(0), _year(0), _wday(0), _yday(0)
{
    time_t t = utcSecond;
    struct tm * ptm = localtime(&t);

    _yday = ptm->tm_yday + 1;
    _wday = ptm->tm_wday;

    _year = ptm->tm_year + 1900;
    _month = ptm->tm_mon + 1;
    _day = ptm->tm_mday;

    _hour = ptm->tm_hour;
    _minute = ptm->tm_min;
    _second = ptm->tm_sec;

}

DateTimeL::DateTimeL( uint64 utcMillisec ) : _millisec(0), _second(0), _minute(0), _hour(0), _day(0), _month(0), _year(0), _wday(0), _yday(0)
{
    _millisec = utcMillisec % 1000;

    time_t t = utcMillisec / 1000;
    struct tm * ptm = localtime(&t);

    _yday = ptm->tm_yday + 1;
    _wday = ptm->tm_wday;

    _year = ptm->tm_year + 1900;
    _month = ptm->tm_mon + 1;
    _day = ptm->tm_mday;

    _hour = ptm->tm_hour;
    _minute = ptm->tm_min;
    _second = ptm->tm_sec;

}

DateTimeL::DateTimeL( String const & dateTimeStr ) : _millisec(0), _second(0), _minute(0), _hour(0), _day(0), _month(0), _year(0), _wday(0), _yday(0)
{
    sscanf( dateTimeStr.c_str(), "%04hu-%02hu-%02huT%02hu:%02hu:%02hu.%03hu", &_year, &_month, &_day, &_hour, &_minute, &_second, &_millisec );
    struct tm t = { 0 };
    t.tm_year = _year - 1900;
    t.tm_mon = _month - 1;
    t.tm_mday = _day;
    t.tm_hour = _hour;
    t.tm_min = _minute;
    t.tm_sec = _second;

    mktime(&t);
    _wday = t.tm_wday;
    _yday = t.tm_yday + 1;
}

ulong DateTimeL::toUtcTime() const
{
    struct tm t = { 0 };
    t.tm_year = _year - 1900;
    t.tm_mon = _month - 1;
    t.tm_mday = _day;
    t.tm_hour = _hour;
    t.tm_min = _minute;
    t.tm_sec = _second;
    return (ulong)mktime(&t);
}

uint64 DateTimeL::toUtcTimeMs() const
{
    uint64 t = this->toUtcTime();
    t = t * 1000 + _millisec;
    return t;
}

winux::String DateTimeL::toString() const
{
    return Format( "%04hu-%02hu-%02huT%02hu:%02hu:%02hu.%03hu", _year, _month, _day, _hour, _minute, _second, _millisec );
}

DateTimeL & DateTimeL::fromCurrent()
{
    struct timeb tbuf = { 0 };
    ftime(&tbuf);

    struct tm * ptm = localtime(&tbuf.time);
    _yday = ptm->tm_yday + 1;
    _wday = ptm->tm_wday;

    _year = ptm->tm_year + 1900;
    _month = ptm->tm_mon + 1;
    _day = ptm->tm_mday;

    _hour = ptm->tm_hour;
    _minute = ptm->tm_min;
    _second = ptm->tm_sec;

    _millisec = tbuf.millitm;
    return *this;
}

DateTimeL & DateTimeL::fromTm( struct tm const * t )
{
    _yday = t->tm_yday + 1;
    _wday = t->tm_wday;

    _year = t->tm_year + 1900;
    _month = t->tm_mon + 1;
    _day = t->tm_mday;

    _hour = t->tm_hour;
    _minute = t->tm_min;
    _second = t->tm_sec;

    _millisec = 0;

    return *this;
}

WINUX_FUNC_IMPL(std::ostream &) operator << ( std::ostream & o, DateTimeL const & dt )
{
    o << dt.toString();
    return o;
}

#ifdef __GNUC__

WINUX_FUNC_IMPL(uint64) GetUtcTimeMs()
{
    uint64 nMillSec = 0;
    struct timeval tv;
    gettimeofday( &tv, NULL );
    nMillSec = (uint64)tv.tv_sec * 1000;
    nMillSec += tv.tv_usec / 1000;
    return nMillSec;
}

WINUX_FUNC_IMPL(uint64) GetUtcTimeUs()
{
    uint64 nMicroSec = 0;
    struct timeval tv;
    gettimeofday( &tv, NULL );
    nMicroSec = (uint64)tv.tv_sec * 1000000;
    nMicroSec += tv.tv_usec;
    return nMicroSec;
}

#else

// 1970-01-01 00:00:00的ULARGE_INTEGER描述
static ULARGE_INTEGER __Time1970( void )
{
    SYSTEMTIME st1970 = {0};
    st1970.wYear = 1970;
    st1970.wMonth = 1;
    st1970.wDay = 1;
    st1970.wHour = 0;
    st1970.wMinute = 0;
    st1970.wSecond = 0;

    FILETIME ft1970;
    ULARGE_INTEGER time1970;
    SystemTimeToFileTime( &st1970, &ft1970 );
    CopyMemory( &time1970, &ft1970, sizeof(time1970) );
    return time1970;
}

WINUX_FUNC_IMPL(uint64) GetUtcTimeMs( void )
{
    FILETIME ft;
    ULARGE_INTEGER time;
    GetSystemTimeAsFileTime(&ft);
    CopyMemory( &time, &ft, sizeof(time) );
    time.QuadPart -= __Time1970().QuadPart;
    return time.QuadPart / 10000;
}

WINUX_FUNC_IMPL(uint64) GetUtcTimeUs( void )
{
    FILETIME ft;
    ULARGE_INTEGER time;
    GetSystemTimeAsFileTime(&ft);
    CopyMemory( &time, &ft, sizeof(time) );
    time.QuadPart -= __Time1970().QuadPart;
    return time.QuadPart / 10;
}

#endif

WINUX_FUNC_IMPL(ulong) GetUtcTime( void )
{
    return (ulong)time(NULL);
}

} // namespace winux
