// -*- mode: C; c-file-style: "bsd"; c-basic-offset: 4; fill-column: 98; coding: utf-8-unix; -*-
// ----------------------------------------------------------------------------------------------
// µCal by J.Perlinger (perlinger@nwtime.org)
//
// To the extent possible under law, the person who associated CC0 with
// µCal has waived all copyright and related or neighboring rights
// to µCal.
//
// You should have received a copy of the CC0 legalcode along with this
// work.  If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
// ----------------------------------------------------------------------------------------------
// µCal -- a small calendar component in C99
// Written anno 2024 by J.Perlinger (perlinger@nwtime.org)
//
// This module contains the interface for Gregorian civil calendar support.
// ----------------------------------------------------------------------------------------------
#ifndef GREGORIAN_H_D2078C60_0B6B_439F_B110_087913F54042
#define GREGORIAN_H_D2078C60_0B6B_439F_B110_087913F54042

#include "common.h"

CDECL_BEG

/// @brief check if a year number denotes a leap year in Gregorian calendar
///
/// Not quite the usual implementation, but it needs only one true division.
/// @param y    calendar year in Gregorian calendar
/// @return     @c true if year is a leap year
static inline bool ucal_IsLeapYearGD(int32_t y) {
    return !(y & 3u) && (!(y & 15u) || (y % 25));
}

/// @brief calculate number of leap days in years
///
/// For a number of years elapsed since epoch, establish the number of leap years (which is also
/// the number of leap days) that happened in that period.  There are several ways to calculate
/// this; this implementation should be reasonably fast.
///
/// While it is quite obvious for years _after_ the epoch, the meaning and handling of negative
/// numbers needs clarification. Conceptually the algorithm finds the last Gregorian Calendar
/// cycle start on or before the given year and works up from there. So, for -1 you will find that
/// there is -1 leap days, as the last year before epoch must be the last year of a (proleptic)
/// cycle and hence is a leap year.
/// @param ey   number of full elapsed years since epoch (negative for proleptic years!)
/// @return     number of leap days in that period
extern int32_t ucal_LeapDaysInYearsGD(int32_t ey);

/// @brief split a RataDie number into elapsed years and days in year
///
/// Basic workhorse to split a number of days to days and years, according to the Gregorian
/// Calendar rules.
/// @param rdn  RDN to split
/// @param pLY  optional pointer to storage of leap year indicator
/// @return     tuple with years as quotient and days as remainder
extern ucal_iu32DivT ucal_DaysToYearsGD(int32_t rdn, bool* pLY);

/// @brief convert RDN to civil date in Gregorian calendar
/// @param into where to store the result
/// @param rdn  RDN (days since 0000-12-31 Gregorian)
/// @return     @c true on success, false on overflow error
extern bool ucal_RdnToDateGD(ucal_CivilDateT* into, int32_t rdn);

/// @brief convert a Gregorian Date to the corresponding Rata Die Number
///
/// The values of month and day can be out of their nominal range; the algorithm treats
/// out-of-range values gracefully. Months will be normalized first, and if only a year and
/// day-of-year are available, act as if it is just an extended January!
/// @param y    calendar year
/// @param m    calender month (can be off-scale)
/// @param d    day-of-month (can be off-scale)
/// @return     RataDie number of the date
extern int32_t ucal_DateToRdnGD(int16_t y, int16_t m, int16_t d);

/// @brief calculate RDN of 1st day of calendar year
///
/// Equivalent to ucal_DateToRdnGD(year,1,1) but faster since there is no need to deal with
/// vagaries of months.  Just the leap year corrections apply here.
/// @param y    calendar year
/// @return     RDN of Jan,1 of year
extern int32_t ucal_YearStartGD(int16_t y);

/// @brief Expand a 2-digit year to a 400 year period (Gregorian calendar)
///
/// This function expands a two-digit year (in the range [0,99]) to a full 400 year period
/// starting at @c ybase.  It basically uses Zeller's congruence backwards, hence the strange
/// name.  (Zeller's congruence is a method to calculate the day-of-week for a date -- we use it
/// to do the reverse: find which date actually has the given day-of-week!)
///
/// For the Gregorian calendar, every valid date with a two-digit year can only have four
/// different weekdays, as the whole calendar has a period of 400 years.  (The only exception is
/// Feb.29, 00. This is the leap day of a quadricentennial year and _must_ be a Tuesday...)
///
/// @note @c errno is set to @c ERANGE if the unfolding would create an overflow
/// and to @c EINVAL if there is no solution for the given date/weekday combination!
///
/// @param y        year, will be taken (mod 100)
/// @param m        month in year, must be in [1,12]
/// @param d        day in month, must be in range
/// @param wd       day of week, [1..7]
/// @param ybase    base year for expansion
/// @return         expanded year or @c INT16_MIN on error
extern int16_t ucal_RellezGD(uint16_t y, uint16_t m, uint16_t d, uint16_t wd, int16_t ybase);

CDECL_END
#endif /*GREGORIAN_H_D2078C60_0B6B_439F_B110_087913F54042*/
