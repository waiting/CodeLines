#ifndef __TIME_HPP__
#define __TIME_HPP__

namespace winux
{
/** 本地日期时间,L意思'local',不包含时区信息 */
class WINUX_DLL DateTimeL
{
public:
    DateTimeL();
    DateTimeL( short year, short month, short day, short hour, short minute, short second, short millisec = 0 );
    /** 秒数 */
    DateTimeL( ulong utcSecond );
    /** 毫秒数 */
    DateTimeL( uint64 utcMillisec );
    /** 格式 xxxx-xx-xxTyy:yy:yy.zzz */
    DateTimeL( String const & dateTimeStr );

    short getYear() const { return _year; }
    short getMonth() const { return _month; }
    short getDay() const { return _day; }
    short getHour() const { return _hour; }
    short getMinute() const { return _minute; }
    short getSecond() const { return _second; }
    short getMillisec() const { return _millisec; }
    /** 星期几[0~6, 0=Sunday] */
    short getDayOfWeek() const { return _wday; }
    /** 年第几日[1~366, 1=01-01] */
    short getDayOfYear() const { return _yday; }

    void setYear( short year ) { _year = year; }
    void setMonth( short month ) { _month = month; }
    void setDay( short day ) { _day = day; }
    void setHour( short hour ) { _hour = hour; }
    void setMinute( short minute ) { _minute = minute; }
    void setSecond( short second ) { _second = second; }
    void setMillisec( short millisec ) { _millisec = millisec; }

    ulong toUtcTime() const;
    uint64 toUtcTimeMs() const;

    String toString() const;
    /** 从当前时间构建DateTimeL */
    DateTimeL & fromCurrent();
    /** 从struct tm结构构建DateTimeL */
    DateTimeL & fromTm( struct tm const * t );

    static ulong GetSecondsFromWeeks( int weeks ) { return weeks * 7UL * 86400UL; }
    static ulong GetSecondsFromDays( int days ) { return days * 86400UL; }
    static ulong GetSecondsFromHours( int hours ) { return hours * 3600UL; }
    static ulong GetSecondsFromMinutes( int minutes ) { return minutes * 60; }
private:
    short _millisec;//[0~999]
    short _second; // [0~59]
    short _minute; // [0~59]
    short _hour;   // [0~23]
    short _day;    // [1~31]
    short _month;  // [1~12]
    short _year;   // [1970~2038]
    short _wday; // 星期几[0~6,0=Sun] 
    short _yday; // 年第几日[1~366,1=January 1]
};

WINUX_FUNC_DECL(std::ostream &) operator << ( std::ostream & o, DateTimeL const & dt );

/** \brief 获取UTC时间毫秒数,UTC秒数可以直接除以1000,或者调用CRT的time(NULL) */
WINUX_FUNC_DECL(uint64) GetUtcTimeMs();
/** \brief 获取UTC时间微秒数,UTC秒数可以直接除以1000000,或者调用CRT的time(NULL) */
WINUX_FUNC_DECL(uint64) GetUtcTimeUs();
/** \brief 获取UTC时间秒数,或者调用CRT的time(NULL) */
WINUX_FUNC_DECL(ulong) GetUtcTime();



} // namespace winux

#endif // __TIME_HPP__
