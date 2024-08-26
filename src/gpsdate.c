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
// This module contains support for GPS/GNSS time stamps.
// ----------------------------------------------------------------------------------------------

/// @file
/// GPS date and raw data handling

#include <errno.h>

#include "ucal/common.h"
#include "ucal/gpsdate.h"
#include "ucal/gregorian.h"
#include "ucal/calconst.h"

// ----------------------------------------------------------------------------------------------
ucal_GpsRawTimeT
ucal_GpsMapTime(
    time_t  tt,
    int16_t ls)
{
    // Execute a phase shift (basic shift + leap seconds), then split by seconds in a week.  The
    // algorithm is simple -- the unknown nature of 'time_t' makes this tricky.  But a GPS era
    // fits into 30 bits, so after a primary reduction we're safe to apply cycle shifts and leap
    // second corrections in 32bit ops.

    ucal_iu32DivT qr;
    int32_t       secs;

    if (sizeof(size_t) >= sizeof(time_t)) {
        // Either a 64bit platform, or a 32-bit platform with 32-bit time_t.  We can do it the
        // simple way here...
        secs = (int32_t)(tt % (1024 * 604800));
    } else {
        // same division, just with our own helper
        secs = (int32_t)ucal_i64u32DivGM((int64_t)tt, 0x93a80000, 0xbbd77933, 2).r;
    }
    secs -= UCAL_sysPhiGPS;          // ... no overflow here ...
    secs += ls;                      // ... or here!
    qr = ucal_iu32Div(secs, 604800); // week split is simple now.
    return (ucal_GpsRawTimeT){ .w = (qr.q & 1023), .t = qr.r };
}

// ----------------------------------------------------------------------------------------------
ucal_iu32DivT
ucal_GpsMapRaw1(
    uint16_t w      ,
    uint32_t t      ,
    int16_t  ls     ,
    int32_t  baseRdn)
{
    // Here we try to keep our numbers down by essentially using calculations based on DAYS, since
    // we can easily factor the time-of-day out of the calculation.  Then it's only a matter of
    // aligning the day cycles.
    
    // First we split the time-in-week to days and time, applying the leap second correction
    // on the fly:
    ucal_iu32DivT dt = ucal_iu32SubDiv(t, ls, 86400u);

    // Now accummulate the days, including the day phase shift between the GPS and RDN scales:
    int32_t days = ((int32_t)(w & 1023) * 7) + dt.q + UCAL_phiGPS;

    // check base day: don't go before epoch ;)
    if (baseRdn < UCAL_rdnGPS) {
        baseRdn = UCAL_rdnGPS;
    }

    // reduce days to the difference to the base (mapping days to RDN on the fly)
    days = ucal_iu32SubDiv((days + 1), baseRdn, (7 * 1024)).r;

    // Add days to base, checking for potential overflow
    if (days > (uint32_t)INT32_MAX - (uint32_t)baseRdn) {
        dt.q = INT32_MAX;
        errno = ERANGE;
    } else {
        dt.q = baseRdn + days;
    }
    return dt;
}

// ----------------------------------------------------------------------------------------------
time_t
ucal_GpsMapRaw2(
    uint16_t     w    ,
    uint32_t     t    ,
    int16_t      ls   ,
    const time_t *base)
{
    // Here we directly align the GPS cycle in seconds to the UNIX epoch and add the cycle
    // difference To the base time.  While this is conceptually somewhat easier than the day-based
    // function, implementing this _really_ efficient is a bit of a chore, as it requires divisons
    // on 'time_t'...
    //
    // We avoid that issue by calculating the cycle difference in int64_t and do a proper floor
    // division on it, either open-coded on 64bit targets and via library function otherwise.
    
    static const int32_t wcycle = INT32_C(604800);		// seconds in a week cycle
    static const int32_t fcycle = INT32_C(604800) * 1024;	// seconds in a full cycle

    time_t  tbase;
    int32_t secs = ((w & 1023) * wcycle) + t - ls + UCAL_sysPhiGPS;

    // Get & trim the expansion / unfolding base:
    if (!base) {
        time(&tbase);
        tbase -= (fcycle >> 1);
    } else {
        tbase = *base;
    }
    if (tbase < UCAL_sysPhiGPS) {
        tbase = UCAL_sysPhiGPS;
    }

    // Calculate cycle difference MOD full cycle length in 64 bit:
    int64_t r = (int64_t)secs - tbase;
    if (sizeof(size_t) >= sizeof(int64_t)) {
        // we can do the division directly
        size_t m = -(r < 0);
        size_t q = m ^ ((m ^ r) / fcycle);
        secs = (int32_t)(r - q * fcycle);
    } else {
        // We do an extended floor division step
        secs = ucal_i64u32DivGM(
            r, UINT32_C(0x93a80000), UINT32_C(0xbbd77933), 2).r;
    }

    // Just glue the parts together:
    return tbase + secs;
}

// ----------------------------------------------------------------------------------------------
int32_t
ucal_GpsRemapRdn(
    int32_t rdn    ,
    int32_t baseRdn)
{
    // This boils down to floor division with 1024*7 on the cycle difference.
    ucal_iu32DivT qr = ucal_iu32SubDiv(rdn, baseRdn, (1024u * 7u));
    if ((uint32_t)INT32_MAX - (uint32_t)baseRdn < qr.r) {
        errno = ERANGE;
        rdn = INT32_MAX;
    } else {
        rdn = baseRdn + qr.r;
    }
    return rdn;
}

// ----------------------------------------------------------------------------------------------
int16_t
ucal_GpsFullYear(
    int16_t y,
    int8_t  m,
    int8_t  d,
    int8_t wd)
{
    int16_t z;
    if (y < 1980) {
	    // If we have a valid day-of-week, try inverting Zeller's congruence to get the year.
	    // Otherwise just do a fixed mapping to 1980..2079.
        y = (int16_t)ucal_iu32Div(y, 100u).r;
        if ((wd >= 0) && (1980 <= (z = ucal_RellezGD(y, m, d, wd, 1980)))) {	    
            y = z;
        } else if (y >= 80) {
            y += 1900;
        } else {
            y += 2000;
        }
    }
    return y;
}

// ----------------------------------------------------------------------------------------------
int32_t
ucal_GpsDateUnfold(
    int16_t y      ,
    int8_t  m      ,
    int8_t  d      ,
    int8_t  wd     ,
    int32_t baseday)
{
    return ucal_GpsRemapRdn(
      ucal_DateToRdnGD(ucal_GpsFullYear(y, m, d, wd), m, d), baseday);
}

// -*- that's all folks -*-
