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
// This module contains the interface for Julian civil calendar support.
// ----------------------------------------------------------------------------------------------
#ifndef JULIAN_H_D2078C60_0B6B_439F_B110_087913F54042
#define JULIAN_H_D2078C60_0B6B_439F_B110_087913F54042

#include "common.h"

CDECL_BEG

/// @brief check if a year number denotes a leap year in Julian calendar
/// @param y    calendar year in Gregorian calendar
/// @return     @c true if year is a leap year
static inline bool ucal_IsLeapYearJD(int32_t y) {
    return !(y & 3u);
}

/// @brief get number of leap days for number of elapsed years
/// @param  ey  elapsed years since calendar epoch
/// @return     number of leap days
extern int32_t ucal_LeapDaysInYearsJD(int32_t ey);

/// @brief split a RataDie number into elapsed years and days in year
///
/// Basic workhorse to split a number of days to days and years, according to the Julian Calendar
/// rules.
/// @param rdn  RDN to split
/// @param pLY  optional pointer to storage of leap year indicator
/// @return     tuple with years as quotient and days as remainder
extern ucal_iu32DivT ucal_DaysToYearsJD(int32_t rdn, bool* pLY);

/// @brief convert RDN to civil date in Julian calendar
/// @param into where to store the result
/// @param rdn  RDN (days since 0000-12-31 Gregorian)
/// @return     @c true on success, false on overflow error
extern bool ucal_RdnToDateJD(ucal_CivilDateT* into, int32_t rdn);

/// @brief convert a Julian Date to the corresponding Rata Die Number
///
/// The values of month and day can be out of their nominal range; the algorithm treats
/// out-of-range values gracefully. Months will be normalized first, and if only a year and
/// day-of-year are available, act as if it is just an extended January!
/// @param y    calendar year
/// @param m    calendar month (can be off-scale)
/// @param d    day-of-month (can be off-scale)
/// @return     RataDie number of the date
extern int32_t ucal_DateToRdnJD(int16_t y, int16_t m, int16_t d);

/// @brief calculate RDN of 1st day of Julian calendar year
/// @param y    calendar year
/// @return     RDN of Jan,1 of year
extern int32_t ucal_YearStartJD(int16_t y);

/// @brief Expand a 2-digit year to a 700 year period (Julian calendar)
///
/// This function expands a two-digit year (in the range [0,99]) to a full 700 year period
/// starting at @c ybase.  It basically uses Zeller's congruence backwards, hence the strange
/// name.  (Zeller's congruence is a method to calculate the day-of-week for a date -- we use it
/// to do the reverse: find which date actually has the given day-of-week!)
///
/// @note: @c errno is set to @c ERANGE if the unfolding would create an overflow.
///
/// @param y        year, will be taken (mod 100)
/// @param m        month in year, must be in [1,12]
/// @param d        day in month, must be in range
/// @param wd       day of week, [1..7]
/// @param ybase    base year for expansion
/// @return         expanded year or @c INT16_MIN on error
extern int16_t ucal_RellezJD(uint16_t y, uint16_t m, uint16_t d, uint16_t wd, int16_t ybase);

CDECL_END
#endif /*JULIAN_H_D2078C60_0B6B_439F_B110_087913F54042*/
