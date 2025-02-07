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
// Written anno 2025 by J.Perlinger (perlinger@nwtime.org)
//
// This module contains some special parsing/decoding functions
// ----------------------------------------------------------------------------------------------

/// @file
/// decoding special time stamp formats
///
/// Parsing ASN.1 time stamps with @c strptime() can be astonishing tricky, not to mention that
/// it can become a performance issue.  Likwise is parsing a decimal fraction to a binary fraction
/// and (to a lesser degree) parsing nanoseconds from a fractional string.
///
/// As this is a frequent topic in some domains, here are some helpers and building blocks.

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "ucal/tsdecode.h"

#include "ucal/common.h"
#include "ucal/gregorian.h"
#include "ucal/calconst.h"

static const uint32_t pow10tab[9] = {
    100000000ul, 10000000ul,  1000000ul,
       100000ul,    10000ul,     1000ul,
          100ul,       10ul,        1ul
};

static const uint32_t pow10_9 = UINT32_C(1000000000);

static uint32_t
_ucal_pnum(
    const char *head,
    unsigned    nch )
{
    uint32_t accu = 0u;
    while (nch--) {
        accu = (accu * 10) + (*head++ - '0');
    }
    return accu;
}

static bool
_ucal_pdot(
    const char **pstr,
    const char  *end )
{
    bool retv = (*pstr != end) && (**pstr == '.');
    *pstr += (size_t)retv;
    return retv;
}

// ----------------------------------------------------------------------------------------------
// parse nanoseconds fractional digits
// ----------------------------------------------------------------------------------------------

uint32_t
ucal_decNano_raw(
    const char **pstr,
    const char  *end )
{
    uint32_t   nsec = 0;
    const char *str = *pstr;
    int         rnd = 0, nch = 0, xch;

    if (NULL == end) {
        end = str + strnlen(str, 128);
    }

    while ((str != end) && isdigit(xch = (uint8_t)*str)) {
        ++str;
        ++nch;
        xch -= '0';
        if (nch < 10) {
            nsec = nsec * 10 + xch;
        } else if (nch == 10) {
            rnd = xch;
        } else if (rnd == 5) {
            rnd += (xch != 0);
        }
    }

    if (0 < nch && 9 > nch) {
        // scale up to nsec resolution if less than 9 digits
        nsec *= pow10tab[nch - 1];
    } else {
        // round; tiebreak is round-to-even
        if (5 == rnd) {
            rnd += (nsec & 1);
        }
        if (5 < rnd) {
            ++nsec;
        }
    }

    *pstr = str;
    return nsec;
}

uint32_t
ucal_decNano(
    const char **pstr,
    const char  *end )
{
    return _ucal_pdot(pstr, end)
            ? ucal_decNano_raw(pstr, end)
            : 0u;
}

// ----------------------------------------------------------------------------------------------
// parse decimal fraction as Q0.32 binary fraction
// ----------------------------------------------------------------------------------------------

ucal_u32DivT
ucal_decFrac_raw(
    const char **pstr,
    const char  *end )
{
    // normalized divider and inverse for Granlund-Moeller division step by 10⁸
    static const uint32_t D = UINT32_C(0xbebc2000); // 3200000000;
    static const uint32_t V = UINT32_C(0x5798ee23);

    const char   *str, *lnz;
    ucal_u32DivT accu = { 0, 0 };
    uint32_t     xrem = 0;
    bool         drop = false;
    int          xch;

    // Initial setup & check if there's work to do at all...
    lnz = str = *pstr;
    if (NULL == end) {
        end = str + strnlen(str, 128);
    }

    // Now find the end of the digit string, remembering the last non-zero digit
    // position as we go.
    while ((str != end) && isdigit((xch = (uint8_t)*str))) {
        ++str;
        if (xch != '0') {
            lnz = str;
        }
    }
    end = str;
    str = *pstr;
    *pstr = end;

    // Clamp the length of digits to process, but remember the fact that we dropped
    // some non-zero digit if we indeed do so.
    if ((drop = (lnz - str > 24))) {
        end = str + 24;
    } else {
        end = lnz;
    }

    // Process digit groups from back to front. We deal with 8 digits per round to
    // avoid repeated division by 10 -- we deal with super-digits of base 10⁸ instead.
    while (end != str) {
        unsigned nch = ((end - str - 1) & 7 ) + 1;
        accu.r = _ucal_pnum((end -= nch), nch) * pow10tab[nch];
        accu.r = (accu.r << 5) | (accu.q >> 27);    // shift up high word, extend from fraction
        accu.q = (accu.q << 5) | ( xrem  >> 27);    // shift up fraction, extend from remainder
        drop   = drop || (0 != (xrem << 5));        // remember if we dropped none-zero bits...
        accu = ucal_u32DivGM(accu.r, accu.q, D, V); // do the division
        xrem = accu.r;                              // save remainder for next round
    }

    // Now we do the final rounding dance:
    accu.r = accu.q;                    // move fraction to remainder part!
    if (xrem > (D >> 1)) {              // > 0.5, round up
        accu.q = (0 == ++accu.r);
    } else if (xrem < (D >> 1)) {       // < 0.5, round down
        accu.q = 0;
    } else if ((accu.r & 1u) || drop) { // 0.5 tiebreak -- round up
        accu.q = (0 == ++accu.r);
    } else {                            // 0.5 tiebreak -- no runding
        accu.q = 0;
    }
    return accu;
}

ucal_u32DivT
ucal_decFrac(
    const char **pstr,
    const char  *end )
{
    return _ucal_pdot(pstr, end)
            ? ucal_decFrac_raw(pstr, end)
            : (ucal_u32DivT){ 0u, 0u };
}

// ----------------------------------------------------------------------------------------------
// deocde some special ASN.1 timestamp formats
// ----------------------------------------------------------------------------------------------

// parse digit groups
static unsigned
_ucal_pdgroups(
    uint8_t     adg[],
    unsigned     ndig,
    const char **pstr,
    const char  *end )
{
    unsigned    cdi = 0;
    int         xch;
    const char *str = *pstr;

    if ((end - str) > ndig) {
        end = str + ndig;
    }
    while ((str != end) && isdigit(xch = (uint8_t)*str)) {
        ++str;
        xch -= '0';
        if (cdi & 1) {
            adg[cdi >> 1] = adg[cdi >> 1] * 10 + xch;
        } else {
            adg[cdi >> 1] = xch;
        }
        ++cdi;
    }
    *pstr = str;
    return cdi;
}

// parse a zone offset spec
static bool
_ucal_ptzo(
    int         *into,
    const char **pstr,
    const char  *end )
{
    bool        tzs = false;
    uint8_t     tzo[2];

    if (*pstr != end) {
        switch (*(*pstr)++) {
        case '-':   tzs = true;
	            //FALLTHROUGH
        case '+':   if ((4 != _ucal_pdgroups(tzo, 4, pstr, end)) || (tzo[0] > 23) || (tzo[1] > 59)) {
                        return false;
                    }
                    if (tzs) {
                        *into = -(int)tzo[0] * 60 - tzo[1];
                    } else {
                        *into = +(int)tzo[0] * 60 + tzo[1];
                    }
                    return true;

        case 'Z':   *into = 0;
                    return true;

        default:    --(*pstr);
                    return false;
        }
    }
    return false;
}

// check if a date/time is well-formed
static bool
_ucal_validate(
    int             year,
    const uint8_t adg[5])
{
    unsigned mon, day;

    mon = adg[0];
    day = adg[1];
    if ((year < INT16_MIN) || (year > INT16_MAX) || (mon < 1) || (mon > 12)) {
        return false;
    }
    if ((day < 1) || (day > _ucal_mdtab[ucal_IsLeapYearGD(year)][mon-1])) {
        return false;
    }

    if ((adg[2] > 23) || (adg[3] > 59) || (adg[4] > 60)) {
      return false;
    }

    return true;
}

// convert broken-down time to UNIX scale
static bool
_ucal_mktime(
    struct timespec *into,
    int              year,
    const uint8_t  adg[5],
    uint32_t         nsec,
    int              tzo )
{
    if (_ucal_validate(year, adg)) {
        uint32_t h = adg[2];
        uint32_t m = adg[3];
        uint32_t s = adg[4];

        into->tv_sec  = ((h * 60) + m) * 60 + s;
        into->tv_sec += ((time_t)ucal_DateToRdnGD(year, adg[0], adg[1]) - UCAL_rdnUNIX) * 86400;

        while (nsec >= pow10_9) {
            ++into->tv_sec;
            nsec -= pow10_9;
        }
        into->tv_nsec = nsec;
        into->tv_sec -= tzo * 60;
        return true;
    }
    return false;
}

static bool
_ucal_mklocal(
    struct timespec *into,
    int              year,
    const uint8_t  adg[5],
    uint32_t         nsec)
{
    if (_ucal_validate(year, adg)) {
        int       tzmode;
        struct tm itm;

        // move items to 'struct tm' format
        memset(&itm, 0, sizeof(itm));
        itm.tm_year = year - 1900;
        itm.tm_mon  = adg[0] - 1;
        itm.tm_mday = adg[1];
        itm.tm_hour = adg[2];
        itm.tm_min  = adg[3];
        itm.tm_sec  = adg[4];

        // try AUTO, STD, DST in that order when converting to time stamp
        for (tzmode = -1; tzmode < 2; ++tzmode) {
            itm.tm_isdst = tzmode;
            errno        = 0;
            into->tv_sec = mktime(&itm);
            if ((-1 != into->tv_sec) || (0 == errno)) {
                break;
            }
        }
        if (tzmode > 1) {
            return false;   // Bummer. All 3 attempts failed.
        }

        // merge with normalised nano-secs, and that's it!
        while (nsec >= pow10_9) {
            ++into->tv_sec;
            nsec -= pow10_9;
        }
        into->tv_nsec = nsec;
        return true;
    }
    return false;
}


// decode ASN.1 UTCTime (tag 23)
bool
ucal_decASN1UtcTime23(
    struct timespec *into,
    const char     **pstr,
    const char      *end ,
    int             ybase)
{
    uint8_t  adg[6];
    int      tzo, y;
    uint32_t frc = 0;

    if (NULL == end) {
        end = *pstr + strnlen(*pstr, 128);
    }

    switch(_ucal_pdgroups(adg, 12, pstr, end)) {
        case 10:
            adg[5] = 0;
            // FALLTHROUGH
        case 12:
            frc = ucal_decNano(pstr, end);
            y   = ybase + ucal_iu32SubDiv(adg[0], ybase, 100).r;
            if (*pstr == end) {
                return _ucal_mklocal(into, y, (adg + 1), frc);
            } else if (_ucal_ptzo(&tzo, pstr, end)) {
                return _ucal_mktime(into, y, (adg + 1), frc, tzo);
            }
            // FALLTHROUGH
        default:
            break;
    }
    return false;
}

// decode ASN.1 GeneralizedTime (tag 24)
bool
ucal_decASN1GenTime24(
    struct timespec *into,
    const char     **pstr,
    const char      *end )
{
    uint8_t  adg[7];
    int      tzo, y;
    uint32_t frc = 0;

    if (NULL == end) {
        end = *pstr + strnlen(*pstr, 128);
    }

    switch(_ucal_pdgroups(adg, 14, pstr, end)) {
        case 10:
            adg[5] = 0;
            // FALLTHROUGH
        case 12:
            adg[6] = 0;
            // FALLTHROUGH
        case 14:
            frc = ucal_decNano(pstr, end);
            y   = (int)adg[0] * 100 + adg[1];
            if (*pstr == end) {
                return _ucal_mklocal(into, y, (adg + 2), frc);
            } else if (_ucal_ptzo(&tzo, pstr, end)) {
                return _ucal_mktime(into, y, (adg + 2), frc, tzo);
            }
            // FALLTHROUGH
        default:
            break;
    }
    return false;
}

// -*- that's all folks -*-
