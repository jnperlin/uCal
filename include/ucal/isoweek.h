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
// This module contains the interface for ISO8601 week calendar support.
// ----------------------------------------------------------------------------------------------
#ifndef ISOWEEK_H_D2078C60_0B6B_439F_B110_087913F54042
#define ISOWEEK_H_D2078C60_0B6B_439F_B110_087913F54042

#include "common.h"

CDECL_BEG

/// @brief convert elapsed years since epoch into elapsed weeks since epoch
/// @param years elapsed years since epoch
/// @return         return number of weeks since epoch
int32_t ucal_WeeksInYearsWD(int32_t years);

/// @brief calculate RDN of 1st day of calendar year
///
/// Equivalent to ucal_DateToRdnWD(year,1,1) but faster..
/// @param y    calendar year
/// @return     RDN of w1,1 of year
extern int32_t ucal_YearStartWD(int16_t y);

/// @brief convert elapsed weeks in the Christian epoch to years and weeks
/// @param weeks    number of weeks to split
/// @return         tuple with years in @c .q and weeks in @c .r
extern ucal_iu32DivT ucal_SplitEraWeeksWD(int32_t weeks);

/// @brief Merge to components of an ISO8601 week date to a RataDie Number
/// @param y calendar year
/// @param w calendar week, [1,53] (can be off-range)
/// @param d calendar week day, [1,7], can be off-range
/// @return  RDN of the date
extern int32_t ucal_DateToRdnWD(int16_t y, int16_t w, int16_t d);

/// @brief convert RataDie Number to date in ISO8601 week calendar
///
/// This function fails if the resulting year is out of the range that can be stored in the date
/// buffer.
/// @param into destination
/// @param rdn  source day number
/// @return     @c true if successful, @c false if truncated
extern bool ucal_RdnToDateWD(ucal_WeekDateT* into, int32_t rdn);

CDECL_END
#endif /*ISOWEEK_H_D2078C60_0B6B_439F_B110_087913F54042*/
