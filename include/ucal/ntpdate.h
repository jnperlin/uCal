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
// This module contains the interface for NTP time value support.
// ----------------------------------------------------------------------------------------------
#ifndef NTPDATE_H_D2078C60_0B6B_439F_B110_087913F54042
#define NTPDATE_H_D2078C60_0B6B_439F_B110_087913F54042

#include "common.h"
#include "calconst.h"

CDECL_BEG

/// @brief map a @c time_t value into the NTP time scale
/// @param tt   input time
/// @return     corresponding value in NTP time scale
static inline uint32_t ucal_TimeToNtp(time_t tt) {
    return (uint32_t)tt - (uint32_t)UCAL_sysPhiNTP;
}

/// @brief map a NTP seconds value into the @c time_t time scale
/// 
/// Map the given NTP-scale seconds value to @c time_t value that is in [pivot-2³¹, pivot+2³¹[.
/// If the pivot is NULL, the current system time will be substituted.
/// @note Will never deliver time stamps before the UNIX epoch (1970-01-01).
/// @param secs  seconds in NTP time scale with undefined era
/// @param pivot center date for expansion
/// @return      seconds mapped to UNIX time scale
extern time_t ucal_NtpToTime(uint32_t secs, const time_t* pivot);

CDECL_END
#endif /*NTPDATE_H_D2078C60_0B6B_439F_B110_087913F54042*/
