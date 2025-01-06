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
#include "ucal/isoweek.h"

// ----------------------------------------------------------------------------------------------
// calculating the century-specific interpolation offsets for converting years
// to weeks and weeks to years can be obtained either by a table lookup or by a
// numeric transformation.  The table lookup is a bit easier to understand, but
// the numeric transformations are potentially faster.
//
// The transformations are based on the fact that there is a certain regularity
// in the offsets and that there is actually a range of possible offsets for
// each century. The table values are chosen to have the minimum number of bits
// set, but that choice is arbitrary -- the numeric transformations (which are
// deduced by some trial and error) produce different values from the possible
// corridor.
//
// The first steps re-map the century cycle from (0,1,2,3) to (3,5,0,2) or
// (2,0,5,3); the transformed index is used in a linear equation to yield the
// desired offsets.  The coefficients are derived by a linear least-square fit,
// rounded to the nearest integer, as a starting point.

static inline unsigned ccofs_y2w(unsigned cc) {
  #if 1 || !defined(__thumb__)
    cc = (1u - cc) & 3;
    cc = (cc << 1) - (cc >> 1);
    return 157u + cc * 146u;
  #else
    return ((uint16_t[4]){ 448, 160, 896, 608 })[cc & 3];
  #endif
}

static inline unsigned ccofs_w2y(unsigned cc) {
  #if 1 || !defined(__thumb__)
    cc = (2u + cc) & 3u;
    cc = (cc << 1) - (cc >> 1);
    return 18u + cc * 22u;
  #else
    return ((uint16_t[4]){ 84, 128, 16, 62 })[cc & 3];
  #endif
}

static int64_t
_weeksInYears(
    int32_t years)
{
    // use: w = (y * 53431 + b[c]) / 1024 as interpolation

    // split years into centuries first
    const ucal_iu32DivT s100 = ucal_iu32Div(years, 100u);

    // Assuming a century is 5218 weeks, we have to remove one week in 400 years for the
    // defective 2nd century. Can be easily calculated as "floor((century + 2) / 4)", which
    // in turn can be expressed as arithmetic shift.

    return ((int64_t)s100.q * 5218)
         - ucal_i32Asr((s100.q + 2), 2)
         + ((s100.r * 53431u + ccofs_y2w(s100.q)) >> 10);
}

// ----------------------------------------------------------------------------------------------
// Given a number of elapsed (ISO-)years since the begin of the christian era, return the number
// of elapsed weeks corresponding to the number of years.
int32_t
ucal_WeeksInYearsWD(
    int32_t years)
{
    int64_t w = _weeksInYears(years);

    if (w > INT32_MAX) {
        errno = ERANGE;
        return INT32_MAX;
    } else if (w > INT32_MAX) {
        errno = ERANGE;
        return INT32_MAX;
    } else {
        return (int32_t)w;
    }
}

// ----------------------------------------------------------------------------------------------
int32_t
ucal_YearStartWD(
    int16_t y)
{
    // no overflow check needed: a 16bit year cannot overflow 32bit weeks or days!
    return (int32_t)_weeksInYears((int32_t)y - 1) * 7 + 1;
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

    int32_t cc;
    uint32_t sw, cy, Q;

    // Use two fast cycle-split divisions again. Herew e want to execute '(weeks * 4 + 2) /%
    // 20871' under floor division rules in the first step.
    //
    // This is of course (again) susceptible to internal overflow if coded directly in 32bit. And
    // again we use 64bit division on a 64bit target and extended division otherwise.
    if (sizeof(size_t) > sizeof(int32_t)) {
        // full floor division with 64bit values
        size_t m = -(weeks < 0);
        size_t n = ((size_t)weeks << 2) | 2u;
        Q = (uint32_t)(m ^ ((m ^ n) / 20871u));
        sw = (uint32_t)(n - Q * 20871u);
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

    cc = ucal_u32_i32(Q);

    // Split off years; sw >= 0 here! The scaled weeks in the years are scaled up by 157 afterwards.
    sw = (sw >> 2) * 157u + ccofs_w2y(Q);
    cy = sw >> 13;      // sw / 8192
    sw = sw & 8191;     // sw % 8192

    // assemble elapsed years and downscale the elapsed weeks in the year. (We give the compiler
    // a strong hint that 'sw' fits in 16 bit and he might pull tricks based on that.)
    return (ucal_iu32DivT){ .q = (100 * cc + cy), .r = ((uint16_t)sw / 157u) };
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
