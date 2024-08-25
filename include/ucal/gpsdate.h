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
// This module contains the interface for GPS time data support.
// ----------------------------------------------------------------------------------------------
#ifndef GPSDATE_H_D2078C60_0B6B_439F_B110_087913F54042
#define GPSDATE_H_D2078C60_0B6B_439F_B110_087913F54042

#include "common.h"

CDECL_BEG

/// @brief a packed raw GPS time stamp: 10 bit week, 20 bit time-of-week.
typedef struct {
    uint32_t t : 20;    ///< time in week
    uint32_t w : 10;    ///< week number in GPS era
} ucal_GpsRawTimeT;

// Functions dealing raw GPS time stamps

/// @brief conver a @c time_t value to a rew GPS time
/// @param tt       system time as ordinary @c time_t value
/// @param ls       leap second correction to apply
/// @return         time stamp as raw GPS time
extern ucal_GpsRawTimeT ucal_GpsMapTime(time_t tt, int16_t ls);

/// @brief map a raw GPS time stamp intp the RataDie time scale
/// @param w        GPS week, [0..1023]
/// @param t        GPS time in week, [0..604799]
/// @param ls       GPS leap second difference to UTC
/// @param baseRdn  base date as RDN for expansion
/// @return         tuple with the effective RDN and the time-of-day
extern ucal_iu32DivT ucal_GpsMapRaw1(uint16_t w, uint32_t t, int16_t ls, int32_t baseRdn);

/// @brief map a raw GPS time stamp into the UNIX time scale
///
/// Map a raw GPS time stamp into the 1024 week period starting at the given base.  If @c base is
/// NULL, assume a symmetric range around the current system time.
/// @param w        GPS week, [0..1023]
/// @param t        GPS time in week, [0..604799]
/// @param ls       GPS leap second difference to UTC
/// @param base     ptr to base time (can be NULL)
/// @return
extern time_t ucal_GpsMapRaw2(uint16_t w, uint32_t t, int16_t ls, const time_t *base);

/// @brief remap a RataDie number to a base date
///
/// This maps a GNSS day (as RataDie number) to an 1024 week period starting at @c baseRdn.
/// @param rdn      day to remap
/// @param baseRdn  base day
/// @return         RataDie number of remapped input
extern int32_t ucal_GpsRemapRdn(int32_t rdn, int32_t baseRdn);

/// @brief establish full year number from possibly truncated calendar year
///
/// This function tries to make the best out of GNSS date information. First it has to establish
/// what the time stamp really means:
///
/// * If the year is >= 1980, it is taken literally.
/// * Else, if the day-of-week is known, use inverse Zeller's congruence to construct a year in the
///   range [1980,2379]
/// * Else, or if Zeller's congruence failed, map the yaer (mod 100) into the range[1980,279]
///
/// @param y        full or truncated calendar year
/// @param m        calendar month
/// @param d        calendar day of month
/// @param wd       day-of-week (-1 if not available)
/// @return         full year number or @c INT16_MIN on error
extern int16_t ucal_GpsFullYear(int16_t y, int8_t m, int8_t d, int8_t wd);

/// @brief unfold a GPS/GNSS time stamp with unknow origin
///
/// First use @c ucal_GpsFullYear() to get the righ (or most likely) calendar
/// year.
///
/// Now that we have a (hopefully meaningful) absolute year, evaluate the date to yield the RDN of
/// that date.
///
/// Since we do not trust the time stamp (many receivers have rollovers after 1024 weeks) we
/// manually remap the RDN to the era starting at the base date.  Obviously this uses @c
/// ucal_GpsRemapRdn to achieve this.
///
/// Since GPS was invented in the 20th century, the RDN must definitely be positive. We reserve
/// the right to return @c INT32_MIN in the case of an error, which can only happen if the
/// day-of-week is specified but Zeller's congruence tells us this date/day combination cannot
/// happen in the Gregorian calendar.
///
/// @param y        year (2 or 4 digit value)
/// @param m        calendar moth
/// @param d        day of month
/// @param wd       day of week (-1 if unknown)
/// @param baseday  RDN of base day for era unfolding
/// @return         RDN of day in a 1024 week period starting at @c baseday
extern int32_t ucal_GpsDateUnfold(int16_t y, int8_t m, int8_t d, int8_t wd, int32_t baseday);

CDECL_END
#endif /*GPSDATE_H_D2078C60_0B6B_439F_B110_087913F54042*/
