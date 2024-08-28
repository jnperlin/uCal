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
// This module contains support for the ISO8601 week calendar.
// ----------------------------------------------------------------------------------------------

/// @file
/// ISO8601 Week Calendar

#include <errno.h>

#include "ucal/common.h"
#include "ucal/julian.h"
#include "ucal/isoweek.h"

// ----------------------------------------------------------------------------------------------
// Given a number of elapsed (ISO-)years since the begin of the christian era, return the number
// of elapsed weeks corresponding to the number of years.
int32_t
ucal_WeeksInYearsWD(
    int32_t years)
{
    // use: w = (y * 53431 + b[c]) / 1024 as interpolation
    static const uint16_t bctab[4] = { 157, 449, 597, 889 };

    int32_t  cs, cw, ci;

    // split years into centuries first
    ucal_iu32DivT s100 = ucal_iu32Div(years, 100u);
    // calculate century cycles shift and cycle index:    
    // Assuming a century is 5217 weeks, we have to add a cycle shift that is 3 for every 4
    // centuries, because 3 of the four centuries have 5218 weeks. So '(cc*3 + 1) / 4' is the
    // actual correction, and the second century is the defective one.
    //
    // Needs floor division by 4, which is done with masking and shifting.
    ci = s100.q * 3 + 1;
    cs = ucal_i32Asr(ci, 2);
    ci = ci & 3u;

    // Get weeks in century. Can use plain division here as all ops are >= 0, and let the
    // compiler sort out the possible optimizations.
    cw = (s100.r * 53431u + bctab[ci]) / 1024u;

    return s100.q * 5217 + cs + cw;
}

// ----------------------------------------------------------------------------------------------
int32_t
ucal_YearStartWD(
    int16_t y)
{
    return ucal_WeeksInYearsWD((int32_t)y - 1) * 7 + 1;
}

// ----------------------------------------------------------------------------------------------
// Given a number of elapsed weeks since the begin of the christian era, split this number into
// the number of elapsed years in res.hi and the excessive number of weeks in res.lo. (That is,
// res.lo is the number of elapsed weeks in the remaining partial year.)
ucal_iu32DivT
ucal_SplitEraWeeksWD(
    int32_t weeks)
{
    // use: y = (w * 157 + b[c]) / 8192 as interpolation
    static const uint8_t bctab[4] = { 85, 130, 17, 62 };

    int32_t cc, ci;
    uint32_t sw, cy, Q;

    // Use two fast cycle-split divisions again. Herew e want to execute '(weeks * 4 + 2) /%
    // 20871' under floor division rules in the first step.
    //
    // This is of course (again) susceptible to internal overflow if coded directly in 32bit. And
    // again we use 64bit division on a 64bit target and extended division otherwise.
    if (sizeof(size_t) > sizeof(int32_t)) {
        /* Full floor division with 64bit values. */
        size_t m = -(weeks < 0);
        size_t n = ((size_t)weeks << 2) | 2u;
        Q = (uint32_t)(m ^ ((m ^ n) / 20871));
        sw = (uint32_t)(n - Q * 20871);
    } else {
        // we use a single Granlund-Möller step again
        uint32_t m = -(weeks < 0);
        uint64_t D = ((uint64_t)weeks << (17 + 2)) + (UINT32_C(2) << 17);
        // now we can divide...
        ucal_u32DivT qr = ucal_u32DivGM(
                            (ucal_u64hi(D) ^ m), (ucal_u64lo(D) ^ m),
                            UINT32_C(0xa30e0000), UINT32_C(0x91ed2f29));
        Q = qr.q ^ m;
        sw = ((qr.r >> 17) ^ m) + (UINT32_C(20871) & m);
    }

  ci = Q & 3u;
  cc = ucal_u32_i32(Q);

  // Split off years; sw >= 0 here! The scaled weeks in the years are scaled up by 157 afterwards.
  sw = (sw / 4u) * 157u + bctab[ci];
  cy = sw / 8192u; /* sw >> 13 , let the compiler sort it out */
  sw = sw % 8192u; /* sw & 8191, let the compiler sort it out */

  // assemble elapsed years and downscale the elapsed weeks in the year.
  return (ucal_iu32DivT) { .q = 100 * cc + cy, .r = sw / 157u };
}

// ----------------------------------------------------------------------------------------------
int32_t
ucal_DateToRdnWD(
    int16_t y,
    int16_t w,
    int16_t d)
{
    return (ucal_WeeksInYearsWD((int32_t)y - 1) + w - 1) * 7 + d;
}

// ----------------------------------------------------------------------------------------------
bool
ucal_RdnToDateWD(
    ucal_WeekDateT *into,
    int32_t         rdn )
{
    bool retv = false;
    ucal_iu32DivT qr;
   
    qr = ucal_iu32SubDiv(rdn, 1, 7u);
    into->dWDay = qr.r + 1;

    qr = ucal_SplitEraWeeksWD(qr.q);
    into->dWeek = qr.r + 1;

    if (qr.q >= INT16_MAX) {
        into->dYear = INT16_MAX;
        errno = ERANGE;
    } else if (qr.q < INT16_MIN - 1) {
        into->dYear = INT16_MIN;
        errno = ERANGE;
    } else {
        into->dYear = (int16_t)(qr.q + 1);
        retv = true;
    }
    return retv;
}

// -*- that's all folks -*-
