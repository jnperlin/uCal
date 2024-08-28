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
// This module contains support for the Julian civil calendar.
// ----------------------------------------------------------------------------------------------

/// @file
/// Julian Civil Calendar

#include <errno.h>

#include "ucal/common.h"
#include "ucal/julian.h"

// ----------------------------------------------------------------------------------------------
int32_t
ucal_LeapDaysInYearsJD(
    int32_t ey)
{
    // This is so dead pan easy... just a floor division by 4, which is simply an arithmetic shift
    // by two!
    return ucal_i32Asr(ey, 2);
}

// ----------------------------------------------------------------------------------------------
ucal_iu32DivT
ucal_DaysToYearsJD(
    int32_t rdn,
    bool   *pLY)
{
    uint32_t sday;
    int32_t qy;

    // We start with splitting the RDN into elapsed days, using scaled days.  This works very
    // similar to the Gregorian calendar split, with two major differences: We have only one
    // bi-phase split with 1461 (the days of a full leap cycle), and we have to add 2 to days to
    // compensate the origin difference of the Julian calendar.
    //
    // So, we want to calculate ((rdn - 1 + 2) * 4 + 3) / 1461, which boils down to
    // (rdn * 4 + 7) / 1461.  The dividend will be negative if and only if rdn < -1.
    if (sizeof(size_t) > sizeof(int32_t)) {
        // Direct scaled division floor via 'size_t' variables 
        const size_t m = -(rdn < -1);
        const size_t n = ((size_t)rdn << 2) + 7;
        const size_t q = m ^ ((m ^ n) / 1461u);
        sday = (uint32_t)n - (uint32_t)q * 1461u;
        qy   = ucal_u32_i32((uint32_t)q);
    } else {
        // Granlund/Möller style division as in the Gregorian calendar, just with other detail
        // parameters, like a pre-division shift of 21 (instead of 14)!
        uint32_t m = -(rdn < -1);
        uint64_t D = ((uint64_t)rdn << (21 + 2)) + (UINT32_C(7) << 21);
        // now we can divide...
        ucal_u32DivT qr = ucal_u32DivGM(
                            (ucal_u64hi(D) ^ m), (ucal_u64lo(D) ^ m),
                            UINT32_C(0xb6a00000), UINT32_C(0x66db072f));
        qy = ucal_u32_i32(qr.q ^ m);
        sday = ((qr.r >> 21) ^ m) + (UINT32_C(1461) & m);
    }

    // if needed, provide leap year flag.
    if (pLY) {
        *pLY = ((qy & 3u) == 3u);
    }
    return (ucal_iu32DivT){ .q = qy, .r = sday >> 2 };
}

// ----------------------------------------------------------------------------------------------
bool
ucal_RdnToDateJD(
    ucal_CivilDateT* into,
    int32_t          rdn )
{
    bool          bLY;
    ucal_iu32DivT yd = ucal_DaysToYearsJD(rdn, &bLY);
    ++yd.q; // from elapsed to calendar year!
    if (yd.q < INT16_MIN || yd.q > INT16_MAX) {
        return false;
    }
    into->dWDay = ucal_i32SubMod7(rdn, 1) + 1;
    into->fLeap = bLY;
    into->dYear = (int16_t)yd.q;
    into->dYDay = yd.r + 1;

    yd = ucal_DaysToMonth(yd.r, bLY);
    into->dMonth = yd.q + 1;
    into->dMDay  = yd.r + 1;
    return true;
}

// ----------------------------------------------------------------------------------------------
int32_t
ucal_DateToRdnJD(
    int16_t y,
    int16_t m,
    int16_t d)
{
    // We use the shifted calendar (starting with March) for this conversion!
    ucal_iu32DivT em = ucal_MonthsToDays(m);
    int32_t       ey = (int32_t)y - 1 + em.q;
    return (ey * 365)
         + ucal_LeapDaysInYearsJD(ey)
         + em.r
         + d
         - INT32_C(308);
}

// ----------------------------------------------------------------------------------------------
int32_t
ucal_YearStartJD(
    int16_t y)
{
    // We use the shifted calendar (starting with March) for this conversion!
    int32_t ey = (int32_t)y - 1;
    return (ey * 365) + ucal_LeapDaysInYearsJD(ey) + 1;
}

// ----------------------------------------------------------------------------------------------
int16_t
ucal_RellezJD(
    uint16_t y,
    uint16_t m,
    uint16_t d,
    uint16_t w,
    int16_t  ybase)
{
    // The rules for inverting Zeller's congruence in the Julian calendar are slightly different:    
    //  - There's no special handling of centennial and quadricentennial years.
    //  - With every century the day-of-week shifts by 6 days (36525 % 7 == 6).
    //  - Also, -6 === 1 (mod 7), so the modular inverse becomes 1.
    //  - The LCM of 100 years in the Julian calendar and 7 days is indeed 700 years!

    uint16_t c;

    // get input data as *elapsed* units (or normalised), bail out on nonsense
    y %= 100;      // don't check, just reduce.
    w %= 7;        // don't check, just reduce.
    --d;
    if ((m < 1) || (m > 12u) || (d > 32u)) {
        goto invalid;   // obviously stupid input
    }

    // Proper shift to year start at March,1st, including propagation/wrap of the year signature
    // at century bounds:
    m += UINT16_C(9);
    if (m >= UINT16_C(12)) {
        m -= UINT16_C(12);
    } else if (--y > 100u) {
        y += UINT16_C(100);
    }

    // Check the day-of-month, assuming every 4th year of a century is a leap year.
    if (d >= _ucal_sdtab[!((y + 1) & 3)][m]) {
        goto invalid;
    }

    // Use Zeller's congruence on the years in century and the months:
    d += y + (y >> 2);
    d += ((m * UINT16_C(83) + UINT16_C(16)) >> 5);

    // Get the century (which advances by 6 week days per century) by doing an exact modular
    // division, using the modular inverse of -6 (mod 7) which is 1, and reducing (mod 7) by plain
    // modulo operation.    
    // Notes: - 'd' < 192 here
    //        - day zero (0000-03-01 JULIAN) was a Monday
    c = (d + 7 + ucal_wdMON - w) % 7;

    // undo calendar base shift now.
    if ((m > UINT16_C(9)) && (++y >= UINT16_C(100))) {
        y -= UINT16_C(100);
        c  = (c + 1u) & UINT16_C(3);
    }
    // Combine year in century with century shift:
    y += (c * UINT16_C(100));

    // Now remap the resulting date to the base year. This is just a basic periodic extension:
    y = ucal_iu32SubDiv(y, ybase, 700u).r;
    if (y > (uint16_t)INT16_MAX - (uint16_t)ybase) {
        goto overflow;
    }
    return ybase + y;

invalid:
    errno = EINVAL;
    return INT16_MIN;

overflow:
    errno = ERANGE;
    return INT16_MIN;
}

// -*- that's all folks -*-
