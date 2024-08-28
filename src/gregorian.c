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
// This module contains support for the Gregorian civil calendar.
// ----------------------------------------------------------------------------------------------

/// @file
/// Gregorian Civil Calendar

#include <errno.h>

#include "ucal/common.h"
#include "ucal/gregorian.h"

// ----------------------------------------------------------------------------------------------
int32_t
ucal_LeapDaysInYearsGD(
    int32_t ey)
{
    // This is essentially the well-known in-out-in calculation for the sum of leap years.  It's
    // slightly compounded by the fact that the calculation is done on the one's(!!)  complement
    // for negative numbers.  Luckily the in-out-in is so well-balanced that we have to do the bit
    // flips only before and after the cascade!
    uint32_t uy, ud, m;
    m   = -(ey < 0);
    uy  = m ^ (uint32_t)ey;
    ud  = (uy >>= 2);
    ud -= (uy /= 25u);
    ud += (uy >>= 2);
    return ucal_u32_i32(ud ^ m);
}

// ----------------------------------------------------------------------------------------------
ucal_iu32DivT
ucal_DaysToYearsGD(
    int32_t rdn,
    bool   *pLY)
{
    uint32_t sday, qy;
    int32_t qc;
    // We start with splitting the RDN into elapsed centuries, using scaled days.  We want to
    // evaluate ((rdn - 1) * 4 + 3) / 146097, which is a fractional fix-point division.
    // First, we observe that (rdn - 1) * 4 + 3 == rdn * 4 - 1, and this term will be negative
    // if and only if rdn <= 0.
    if (sizeof(size_t) > sizeof(int32_t)) {
        // 'size_t' is a wider type tahn 'int32_t', and it seems to fit into a single
        // register. We use direct floor division.
        size_t m = -(rdn <= 0);
        size_t n = ((size_t)rdn << 2) - 1;
        size_t q = m ^ ((m ^ n) / 146097u);
        sday = (uint32_t)n - (uint32_t)q * 146097u;
        qc = ucal_u32_i32((uint32_t)q);
    } else {
        // No way with single registers! We need two extra bits to do this properly, so we use a
        // Granlund/Möller division step instead.  To normalize the divider, we have to shift 14
        // bits to the left; we fuse this with the shift op from the multiplication by 4.
        uint32_t m = -(rdn <= 0);
        uint64_t D = ((uint64_t)rdn << (14 + 2)) - (UINT32_C(1) << 14);
        // now we can divide...
        ucal_u32DivT qr = ucal_u32DivGM(
                            (ucal_u64hi(D) ^ m), (ucal_u64lo(D) ^ m),
                            UINT32_C(0x8eac4000), UINT32_C(0xcb5835e6));
        qc = ucal_u32_i32(qr.q ^ m);
        sday = ((qr.r >> 14) ^ m) + (UINT32_C(146097) & m);
    }
    // The elapsed year cycles come next. This is again a fractional fixpoint division, but since
    // 'sday' is now in range [0,146097] and unsigned, we do a simple unsigned division op for
    // that.
    sday |= 3;
    qy    = sday / 1461u;
    sday -= qy * 1461u;

    // If needed, provide leap year flag. And while it looks funny, it really does the job.
    if (pLY) {
        *pLY = ((qy & 3u) == 3u) && (qy <= (96 + (qc & 3u)));
    }
    return (ucal_iu32DivT){ .q = qc * 100 + qy, .r = sday >> 2 };
}

// ----------------------------------------------------------------------------------------------
bool
ucal_RdnToDateGD(
    ucal_CivilDateT* into,
    int32_t          rdn )
{
    bool bLY;
    ucal_iu32DivT yd = ucal_DaysToYearsGD(rdn, &bLY);
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
ucal_DateToRdnGD(
    int16_t y,
    int16_t m,
    int16_t d)
{
    // We use the shifted calendar (starting with March) for this conversion!
    ucal_iu32DivT em = ucal_MonthsToDays(m);
    int32_t       ey = (int32_t)y - 1 + em.q;
    return (ey * 365)
         + ucal_LeapDaysInYearsGD(ey)
         + em.r
         + d
         - INT32_C(306);
}

// ----------------------------------------------------------------------------------------------
int32_t
ucal_YearStartGD(
    int16_t y)
{
    // We use the shifted calendar (starting with March) for this conversion!
    int32_t ey = (int32_t)y - 1;
    return (ey * 365) + ucal_LeapDaysInYearsGD(ey) + 1;
}

// ----------------------------------------------------------------------------------------------
int16_t
ucal_RellezGD(
    uint16_t y,
    uint16_t m,
    uint16_t d,
    uint16_t w,
    int16_t  ybase)
{
    // This calculation is pure number magic. It *could* have been written in terms of some of the
    // functions above, partially -- but it would not be faster, nor much more readable.
    //
    // So here some hints to understand what's going on:
    //
    // The day-of-week of the 1st day of a century moves by 5 days for each century of a great
    // cycle, since 36524 % 7 == 5. Since the complete calendar repeats every 400 years, each date
    // in a century can only have 4 possible day-of-week values.
    //
    // So this boils down to getting the day-of-week signature of a truncated date (which is to
    // say, a date in the 1st century of the proleptic Gregorian calendar), and this is where
    // Zeller's congruence comes into play. As we only need to consider dates in the 1st century,
    // and need only the day-of-week, we can:
    //
    //   - shorten factors, so products are congruent (mod 7) but with smaller
    //   numbers
    //   - drop leap year rules for centennials and quad-centennials
    //   - reduce all additive terms (mod 7) to get strictly non-negative small
    //   numbers

    // In the end, the century shift can be calculated by doing an exact modular division by 5
    // (mod 7) over the day-of-week difference between the input and the day-of-week for the date
    // taken in the 1st century of a great cycle.
    //
    // It's even a bit easier to take the negated difference and do an exact division by
    // (-5)(mod 7), which is 4: 4*(-5) == -20 === 1 (mod 7).
    //
    // As for fast 'mod 7' implementations, see "Hacker's Delight". We can even avoid the explicit
    // multiplication by 4, as it can be fused into the 'mod 7' implementation.

    uint16_t c;

    // get input data as *elapsed* units (or normalised), bail on nonsense
    y %= 100;      // don't check, just reduce.
    --d;
    w %= 7;        // don't check, just reduce.
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

    // Check the day-of-month, assuming every 4th year of a century is a leap year. This needs
    // some special care: The last day of a (shifted) great calendar cycle (that is, Feb-29 of a
    // quadricentennial year) *must* be a Tuesday, and all other possibilities are ruled out. Since
    // the congruent inversion algorithm cannot detect this below, a special rule is needed
    // here. It is important that for *valid* input the calculation below produces the proper
    // result, but it fails to reject invalid input for this special case.
    if (y == 99 && m == 11 && d == 28) {
        if (w != ucal_wdTUE % 7) {
            goto invalid;
        }
    } else {
        if (d >= _ucal_sdtab[!((y + 1) & 3)][m]) {
            goto invalid;
        }
    }

    // Use Zeller's congruence on the years in century and the months:
    d += y + (y >> 2);
    d += ((m * UINT16_C(83) + UINT16_C(16)) >> 5);

    // Get the century (which advances by 5 week days per century) by doing an exact modular
    // division, using the modular inverse of -5 (mod 7) which is 4, and reducing (mod 7) by
    // multiplication, shift and mask in one go.
    // Notes: - 'd' < 192 here
    //        - day zero (0000-03-01) was a Wednesday
    c = (((d + 7 + ucal_wdWED - w) * UINT32_C(0x12493)) >> 14) & UINT16_C(7);
    if (c >= 4) {
        goto invalid;   // invalid solution --> not-so-obvious stupid input
    }

    // Valid solution -- undo calendar base shift now.
    if ((m > UINT16_C(9)) && (++y >= UINT16_C(100))) {
        y -= UINT16_C(100);
        c  = (c + 1u) & UINT16_C(3);
    }
    // Combine year in century with century shift:
    y += (c * UINT16_C(100));

    // Now remap the resulting date to the base year. This is just a basic periodic extension:
    y = ucal_iu32SubDiv(y, ybase, 400u).r;
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
