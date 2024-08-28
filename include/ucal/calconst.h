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
// This module contains calendar constants.
// ----------------------------------------------------------------------------------------------
#ifndef CALCONST_H_D2078C60_0B6B_439F_B110_087913F54042
#define CALCONST_H_D2078C60_0B6B_439F_B110_087913F54042

// Some important calendar fixpoints
#define UCAL_rdnNTP  693596
#define UCAL_rdnUNIX 719163
#define UCAL_rdnGPS  722820

// The week cycle and the Gregorian calendar cycle are aligned: The 1st day of a quadricentennial
// is always a Monday.  Other cycles need more effort...
//
// A GPS era is 1024 weeks, with an epoch of 1980-01-06.  This produces a cycle shift ('phi', for
// 'phase') that has to be applied when mapping GPS stamps to the RDN scale.
#define UCAL_phiGPS  6019

// Just for completeness, here are two other important shifts:

// The NTP timescale has an epoch of 1900-01-01, with a period of 2³² seconds. We define the phi
// in seconds, with regard to the system time:
//   phi = (1900-01-01 - 1970-01-01) * 86400 (mod 2³²)
#define UCAL_sysPhiNTP 0x7c558180

// We make similar PHI for unwrapping a GPS/GNSS time stamp that consists only of a week number
// [0,1023] and the seconds in a week:
//   phi = (1980-01-06 - 1970-01-01) * 86400 (mod 1024*7*86400)
#define UCAL_sysPhiGPS 0x12d53d80

#endif /*CALCONST_H_D2078C60_0B6B_439F_B110_087913F54042*/
