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
// This module contains internal common utilities.
// ----------------------------------------------------------------------------------------------

/// @file
/// Common utilities for the Civil Calendar

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "ucal/common.h"
#include "ucal/calconst.h"

/// @brief hint the compiler which branch to take (if conditional execution is not an option)
#ifndef UNLIKELY
# ifdef __GNUC__
#  define UNLIKELY(x) __builtin_expect((x), 0)
# else
#  define UNLIKELY(x) x
# endif
#endif

// ----------------------------------------------------------------------------------------------
/// @brief month length table, regular 
///
/// Table with days of month, zero-based. Used internally for validation only. Contains 2 vectors,
/// one for regular years at index 0 and one for leap years at index 1.
const uint8_t _ucal_mdtab[2][12] = {
//   JAN FEB MAR APR MAY JUN JUL AUG SEP OCT NOV DEC
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

// ----------------------------------------------------------------------------------------------
/// @brief month length table, shifted 
///
/// Table with days of month, shifted for year starting at March, zero-based. Used internally for
/// validation only. Contains 2 vectors, one for regular years at index 0 and one for leap years
/// at index 1.
const uint8_t _ucal_sdtab[2][12] = {
//   MAR APR MAY JUN JUL AUG SEP OCT NOV DEC JAN FEB
    {31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 28},
    {31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29}
};

// ----------------------------------------------------------------------------------------------
// get buils date (either from compiler or user-supplied) as RDN

static const char s_monthTab[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
static const char s_autoDate[] = __DATE__;
static const char s_userDate[] =
#ifdef BUILD_DATE
  BUILD_DATE
#else
  ""
#endif
  ;


static int32_t
parseDate(
    const char* str)
{
    extern int32_t ucal_DateToRdnGD(int16_t, int16_t, int16_t);

    short y, m, d;
    char ms[4];
    const char* mp;

    if (sscanf(str, "%3s %hu %4hu", ms, &d, &y) != 3) {
        return -1;
    }
    if (d < 1 || d > 31 || y < 1970 || y > 9999) {
        return -1;
    }
    if (NULL == (mp = strstr(s_monthTab, ms))) {
        return -1;
    }
    m = 1 + (mp - s_monthTab) / 3u;
    return ucal_DateToRdnGD(y, m, d);
}

int32_t
ucal_BuildDateRdn(void)
{
    int32_t retv;
    retv = parseDate(s_userDate);
    if (retv < 0) {
        retv = parseDate(s_autoDate);
    }
    return retv;
}

// -------------------------------------------------------------------------------------
// This is a single division core step a la Granlund/Möller:
//    2³¹ <= d < 2³²                (divider is normalised)
//    v == (2⁶⁴ - 1) // d - 2³²     (approximation of the fix-point inverse)
//    0 <= u1 < d                   (quotient must fit in limb)
ucal_u32DivT
ucal_u32DivGM(
    uint32_t u1,
    uint32_t u0,
    uint32_t d ,
    uint32_t v )
{
    uint32_t q1, q0;
    // For the double-width product/sum, a 64bit accu is used.
    {
      uint64_t accu = (uint64_t)u1 * (uint64_t)v + u0;
      q0 = (uint32_t)accu;
      q1 = (uint32_t)(accu >> 32) + u1 + 1;
    }
    u0 -= q1 * d;               // u0 becomes the remainder (value no longer needed)
    if (u0 > q0) {              // The 'unpredictable' condition
        q1 -= 1;
        u0 += d;
    }
    if (UNLIKELY(u0 >= d)) {    // The 'unlikely' condition
        q1 += 1;
        u0 -= d;
    }
    return (ucal_u32DivT){ .q = q1, .r = u0 };
}


ucal_i64u32DivT
ucal_i64u32DivGM(
    int64_t  u,
    uint32_t d,
    uint32_t v,
    unsigned s)
{
    ucal_u32DivT   xdiv;
    const uint32_t m = -(u < 0);

    // get the 3 limbs for the division steps
    uint64_t ut  = (uint64_t)u;
    uint32_t utl = m ^ (uint32_t)(ut << s);
    uint32_t utm = m ^ (uint32_t)(ut >> (32 - s));
    uint32_t uth = s ? (m >> (32 - s)) ^ (uint32_t)(ut >> (64 - s)) : 0;

    // do two chained divisions
    xdiv = ucal_u32DivGM(uth, utm, d, v);
    utm = xdiv.q;
    xdiv = ucal_u32DivGM(xdiv.r, utl, d, v);
    utl = xdiv.q;
    // assemble quotient
    ut   = m ^ utm;
    ut <<= 32;
    ut  |= m ^ utl;
    // assemble result
    return (ucal_i64u32DivT){
        .q = ((m) ? -(int64_t)(~ut) - 1 : (int64_t)ut),
        .r = (((m ^ xdiv.r) + (m & d)) >> s)
    };
}

ucal_TimeDivT
ucal_TimeToDays(
    time_t tt)
{
    ucal_TimeDivT retv;
    
    if (sizeof(time_t) <= sizeof(size_t)) {
        // 'time_t' fits in a register, or so it seems. Do a single signed/unsigned floor
        // division, and that's it.
        size_t m = -(tt < 0);
        size_t q = m ^ ((m ^ (size_t)tt) / 86400u);
        retv.q = m ? -(time_t)(~q) - 1 : (time_t)q;
        retv.r = (uint32_t)tt - (uint32_t)q * 86400u;
    } else {
        // Assume a 64bit time on a 32bit machine. Still a division of int64_t by uint32_t
        // constant... it just doesn't look so nice!
        ucal_i64u32DivT ds = ucal_i64u32DivGM(
            (int64_t)tt, 0xa8c00000, 0x845c8a0c, 15);
        retv.q = (time_t)ds.q;
        retv.r = ds.r;
    }
    return retv;
}

ucal_TimeDivT
ucal_TimeToRdn(
    time_t tt)
{
    ucal_TimeDivT qr = ucal_TimeToDays(tt);
    qr.q += UCAL_rdnUNIX;
    return qr;
}

ucal_iu32DivT
ucal_DaysToMonth(
    uint_fast16_t ed  ,
    bool          isLY)
{
    // Adjust for a February with 30 days, so we don't need to shift around the year start...
    unsigned skipdays = 1 + !isLY;
    if (ed >= 61 - skipdays) {
        ed += skipdays;
    }
    uint_fast16_t m = (ed * 67u + 32) >> 11;
    ed -= (m * 489u + 8) >> 4;
    return (ucal_iu32DivT){ .q = m, .r = ed };
}

ucal_iu32DivT
ucal_MonthsToDays(
    int16_t m)
{
    int32_t  em = m + UINT32_C(9);
    uint32_t mm = -(em < 0);
    uint32_t qm = mm ^ ((mm ^ em) / 12u);
    em -= qm * 12u;
    return (ucal_iu32DivT){ .q = ucal_u32_i32(qm), .r = ((UINT32_C(979) * em + 16) >> 5) };
}

static int32_t
checkedAdd(
    int32_t  rdn  ,
    unsigned shift)
{
    uint32_t avail = (uint32_t)INT32_MAX - (uint32_t)rdn;
    if (shift > avail) {
        rdn = INT32_MAX;
        errno = ERANGE;
    } else {
        rdn += shift;
    }
    return rdn;
}

static int32_t
checkedSub(
    int32_t  rdn  ,
    unsigned shift)
{
    uint32_t avail = (uint32_t)rdn - (uint32_t) INT32_MAX;
    if (shift > avail) {
        rdn = INT32_MIN;
        errno = ERANGE;
    } else {
        rdn -= shift;
    }
    return rdn;
}

int32_t
ucal_WdGT(int32_t rdn, int wd)
{
    unsigned shift = ucal_i32SubMod7(wd - 1, rdn) + 1;
    return checkedAdd(rdn, shift);
}

int32_t
ucal_WdGE(int32_t rdn, int wd)
{
    int shift = ucal_i32SubMod7(wd, rdn);
    return checkedAdd(rdn, shift);
}

int32_t
ucal_WdLE(int32_t rdn, int wd)
{
    int shift = ucal_i32SubMod7(rdn, wd);
    return checkedSub(rdn, shift);
}

int32_t
ucal_WdLT(int32_t rdn, int wd)
{
    int shift = ucal_i32SubMod7(rdn, wd + 1) + 1;
    return checkedSub(rdn, shift);
}

int32_t
ucal_WdNear(int32_t rdn, int wd)
{
    return (rdn < 0)
        ? ucal_WdLE(rdn + 3, wd)
        : ucal_WdGE(rdn - 3, wd);
}

// -------------------------------------------------------------------------------------
int32_t
ucal_DayTimeSplit(
    ucal_CivilTimeT *into,
    int32_t          dt  ,
    int32_t          ofs )
{
  ucal_iu32DivT qr = ucal_iu32SubDiv(dt, -ofs, 86400);

  uint16_t m = qr.r / 60u;
  uint16_t h = m    / 60u;

  into->tSec  = (uint8_t)(qr.r - m * 60u);
  into->tMin  = (uint8_t)(m    - h * 60u);
  into->tHour = (uint8_t)h;

  return qr.q;
}

// -------------------------------------------------------------------------------------
int32_t
ucal_DayTimeMerge(
    int16_t h,
    int16_t m,
    int16_t s)
{
    return ((int32_t)h * 60 + m) * 60 + s;
}

// -*- that's all folks -*-
