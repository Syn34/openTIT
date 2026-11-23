// openTIT - C runtime <time.h> routines.
//
// The console library does its own calendar arithmetic (Dinkumware's
// _Daysto/_Ttotm, counting from 1 January 1900) and asks the kernel for the
// time zone. gmtime keeps that arithmetic; local time zone handling comes
// from the host.
#include "crt/libc/Libc.h"

#include <ctime>

namespace crt {

namespace {

constexpr int64_t kSecondsPerDay = 86400;
// Seconds from 1900-01-01 to the 1970-01-01 epoch.
constexpr int64_t kSeconds1900To1970 = 2208988800LL;
// NOTE: the console asks the kernel for the daylight-saving shift; one hour
// is assumed here.
constexpr int64_t kDaylightSavingSeconds = 3600;

// Day of the year on which each month starts: [0] ordinary years, [1] leap years.
constexpr short kMonthStart[2][12] = {
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335},
};

// Leap rule of the original: every fourth year counted from 1900, except
// 1900 itself (correct for 1901-2099).
bool IsLeapYear(int yearsSince1900)
{
    return yearsSince1900 % 4 == 0 && yearsSince1900 != 0;
}

int64_t FloorDiv(int64_t value, int64_t divisor)
{
    int64_t quotient = value / divisor;
    if ((value % divisor) != 0 && ((value < 0) != (divisor < 0)))
        --quotient;
    return quotient;
}

} // namespace

// @func 0x00722EBC DaysToMonth
// _Daysto(): the leap days accumulated between 1900 and `year` (years since
// 1900) plus the day of the year on which `month` starts. Together with
// 365 * year this gives the day number of the first of that month.
int DaysToMonth(int year, int month)
{
    int leapDays;
    if (year > 0)
        leapDays = (year - 1) / 4;
    else if (year <= -4)
        leapDays = 1 + (4 - year) / 4;
    else
        leapDays = 0;
    return leapDays + kMonthStart[IsLeapYear(year) ? 1 : 0][month];
}

// @func 0x00722F4C SecondsToTm
// _Ttotm(): splits a time in seconds since 1970 into calendar fields. With a
// null `out` the calling thread's static tm is used (as gmtime/localtime
// return). `isDst` > 0 shifts the time by the daylight-saving offset first.
std::tm* SecondsToTm(std::tm* out, int64_t secondsSince1970, int isDst)
{
    static thread_local std::tm s_tm;
    if (!out)
        out = &s_tm;

    int64_t seconds = secondsSince1970 + kSeconds1900To1970;
    out->tm_isdst = isDst;
    if (isDst > 0)
        seconds += kDaylightSavingSeconds;

    const int64_t days = FloorDiv(seconds, kSecondsPerDay);
    const int64_t secondOfDay = seconds - days * kSecondsPerDay;

    // 1 January 1900 was a Monday.
    out->tm_wday = (int)((days % 7 + 8) % 7);

    int year = (int)(days / 365);
    int64_t yearStart;
    while ((yearStart = DaysToMonth(year, 0) + 365LL * year) > days)
        --year;
    const int dayOfYear = (int)(days - yearStart);
    out->tm_year = year;
    out->tm_yday = dayOfYear;

    const short* monthStart = kMonthStart[IsLeapYear(year) ? 1 : 0];
    int month = 11;
    while (dayOfYear < monthStart[month])
        --month;
    out->tm_mon = month;
    out->tm_mday = dayOfYear - monthStart[month] + 1;

    out->tm_hour = (int)(secondOfDay / 3600);
    out->tm_min = (int)(secondOfDay % 3600 / 60);
    out->tm_sec = (int)(secondOfDay % 60);
    return out;
}

// @func 0x00725E14 Time
// time(): seconds since 1970; also stored through `out` when given.
std::time_t Time(std::time_t* out)
{
    return std::time(out);
}

// @func 0x0071C1D0 GmTime
// gmtime(): UTC calendar time in a per-thread static tm.
std::tm* GmTime(const std::time_t* t)
{
    return SecondsToTm(nullptr, (int64_t)*t, 0);
}

// @func 0x0071C220 LocalTime
// localtime(): calendar time in the local time zone, in a per-thread static
// tm. The console applies the system time zone and daylight-saving setting;
// here the host's time zone database does that.
std::tm* LocalTime(const std::time_t* t)
{
    static thread_local std::tm s_local;
#if defined(_WIN32)
    if (localtime_s(&s_local, t) != 0)
        return nullptr;
#else
    if (!localtime_r(t, &s_local))
        return nullptr;
#endif
    return &s_local;
}

// @func 0x0071DC88 MkTime
// mktime(): converts (and normalises) a local calendar time into seconds
// since 1970, or -1 if it cannot be represented.
std::time_t MkTime(std::tm* t)
{
    return std::mktime(t);
}

} // namespace crt
