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
// POSIX time zone string handling
// ----------------------------------------------------------------------------------------------

/// @file
/// POSIX time zone (string) support
///
/// Support of simple POSIX timezone specs (with GNU extension). This involves parsing from a
/// string and evaluation of teh conversion rules.  The API is minimalistic by intention;  using
/// the functions to provide higher-level services on an embedded or otherwise restricted system
/// is the key idea here.

#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <ctype.h>

#include "ucal/common.h"
#include "ucal/calconst.h"
#include "ucal/gregorian.h"
#include "ucal/tzposix.h"

// EOF is defined in stdio.h, but we don't want that one here...
#ifndef EOF
# define EOF (-1)
#endif

// -------------------------------------------------------------------------------------
// parsing a POSIX time zone string
// -------------------------------------------------------------------------------------

// local parse context -- simply a pair of pointers
typedef struct {
    const char* spHead; //!< current head
    const char* spTail; //!< tail / end of string
} tziParseCtxT;

static inline int
c2i_(const char *cp) {
    return (unsigned char)*cp;
}

static int
tzi_PeekChar(tziParseCtxT const *ctx)
{
    return (ctx->spHead != ctx->spTail) ? c2i_(ctx->spHead) : EOF;
}

static bool
tzi_ParseChar(tziParseCtxT *ctx, int xch)
{
    bool retv = false;
    if (ctx->spHead == ctx->spTail) {
        retv = (EOF == xch);
    } else if (c2i_(ctx->spHead) == xch) {
        retv = true;
        ++ctx->spHead;
    }
    return retv;
}

// Parse away a zone name, either in the CAPS-ONLY or the <quoted-string> format.
// We store the zone name by value, which can be silently truncated.
// !Note! The destination must be able to hold at least a terminating NUL char.
static bool
tzi_ParseName(tziParseCtxT *ctx, char *into, unsigned size)
{
    int         xch;
    bool        retv;
    unsigned    ccnt = 0;
    const char *head = ctx->spHead;

    // The string buffer should be set to zero already. We just make sure that
    // we keep a trailing NUL byte...
    --size;

    if (head == ctx->spTail) {
        retv = false;
    } else  if ('<' == (xch = c2i_(head))) {
        while ((++head != ctx->spTail) && ('>' != (xch = c2i_(head)))) {
            if ('<' == xch) {
                break;              // ...bummer...
            } else if (ccnt < size) {
                into[ccnt++ ]= (char)xch;// store one more byte ...
            }
        }
        retv  = (head != ctx->spTail) && ('>' == c2i_(head));
        head += (int)retv;
    } else if (isupper(xch)) {
        // consume a sequence of 7-bit ASCII uppercase characters. Success if at least
        // 3 of them have been stored.
        do {
            if (ccnt < size) {
                into[ccnt++] = (char)xch;
            }
        } while ((++head != ctx->spTail) && isupper((xch = c2i_(head))));
        retv = (3 <= ccnt);
    } else {
        retv = false;
    }

    if (retv) {
        ctx->spHead = head;
    }
    return retv;
}

// Parse an optional number sign (+/-)
static bool
tzi_ParseOptSign(tziParseCtxT* ctx, bool* into)
{
    bool nsig = false;
    switch (tzi_PeekChar(ctx)) {
        case '-':   nsig = true;
        case '+':   ++ctx->spHead;
        default:    break;
    }
    *into = nsig;
    return true;
}

// Parse a pure unsigned number. Will stop once the current accumulation exceeds a value
// of 100 (so 999 is the max number we will parse) or (of course) when the cursor points
// to a non-digit symbol. Fails if no digits have been converted at all.
static bool
tzi_ParseNum(tziParseCtxT *ctx, int* into)
{
    bool ret = false;
    int  tmp = 0, xch;
    while ((100 > tmp) && isdigit((xch = tzi_PeekChar(ctx)))) {
        tmp = 10 * tmp + (xch - '0');
        ++ctx->spHead;
        ret = true;
    }
    *into = tmp;
    return ret;
}

// Parse time, either in a zone offset or a rule transition, with optional sign on hours.
// Also, offsets must be in +/- 1day, while transition times cover +/- 1 week range.
// That's to faciliate funny things like the Greenland rules, which switches  together
// with Denmark for the time being.
//
// Note that while w e _read_ h[:m[:s]], seconds must evaluate to zero. Every time zone
// in the computer age should be at least be on a minutes boundary (hours preferred),
// and the zone offset should also be expressible in minutes.  Ideally, it should be
// full hours, but there's always someone spoiling the game.  At least the 15 minute
// interval boundary seems to be an accepted granularity these days.
// Keep fingers crossed!
static bool
tzi_ParseTime(tziParseCtxT *ctx, int16_t* into, bool isRuleTime)
{
    bool retv, nsig;
    int idx = 0, tmpHMS[3] = { 0, 0, 0 };

    retv = tzi_ParseOptSign(ctx, &nsig);
    if (retv) do {
        retv = tzi_ParseNum(ctx, &tmpHMS[idx]);
    } while (retv && (++idx < 3) && tzi_ParseChar(ctx, ':'));
    if (retv) {
        retv = ((isRuleTime ? 168 : 24) > tmpHMS[0]) && (60 > tmpHMS[1]) && (0 == tmpHMS[2]);
    }
    tmpHMS[2] = retv ? (60 * tmpHMS[0] + tmpHMS[1]) : 0;
    *into = nsig ? -tmpHMS[2] : tmpHMS[2];
    return retv;
}

// Parse a single transition rule, any of the 3 types described by POSIX.
static bool
tzi_ParseRule(tziParseCtxT* ctx, tziPosixRuleT* into)
{
    bool ret = false;
    int tmpMDW[3] = { 0, 0, 0 };
    int xch = tzi_PeekChar(ctx);
    if ('M' == xch) {
        ++ctx->spHead;
        ret = tzi_ParseNum(ctx, &tmpMDW[0]) && tzi_ParseChar(ctx, '.') &&
              tzi_ParseNum(ctx, &tmpMDW[1]) && tzi_ParseChar(ctx, '.') &&
              tzi_ParseNum(ctx, &tmpMDW[2]);
        ret = ret
           && (1 <= tmpMDW[0]) && (12 >= tmpMDW[0])
           && (1 <= tmpMDW[1]) && ( 5 >= tmpMDW[1])
           && (7 >= tmpMDW[2]);
        if (ret) {
            into->rt_month = tmpMDW[0];
            into->rt_mdmw  = tmpMDW[1];
            into->rt_wday  = ((tmpMDW[2] + 6u) % 7u) + 1;
        }
    } else if ('J' == xch) {
        ++ctx->spHead;
        if (isdigit(tzi_PeekChar(ctx)) && tzi_ParseNum(ctx, &tmpMDW[1]) && (1 <= tmpMDW[1]) && (365 >= tmpMDW[1])) {
            ucal_iu32DivT yd = ucal_DaysToMonth(tmpMDW[1] - 1, false);
            into->rt_month = yd.q + 1;
            into->rt_mdmw  = yd.r + 1;
            into->rt_wday  = 0;
            ret = true;
        }
    } else if (isdigit(xch)) {
        if (isdigit(tzi_PeekChar(ctx)) && tzi_ParseNum(ctx, &tmpMDW[1]) && (365 >= tmpMDW[1])) {
            into->rt_month = 1;
            into->rt_mdmw  = tmpMDW[1] + 1;
            into->rt_wday  = 0;
            ret = true;
        }
    }
    if (ret && tzi_ParseChar(ctx, '/')) {
        int16_t tmp;
        ret = tzi_ParseTime(ctx, &tmp, true);
        into->rt_ttloc = tmp;
    } else {
        into->rt_ttloc = 120;
    }
    return ret;
}

const char*
tziFromPosixSpec(
    tziPosixZoneT *into,
    const char    *head,
    const char    *tail)
{
    // We keep the POSIX (U.S.) default rules as a verbatim copy here.
    static tziPosixRuleT defRules[2] = {
        // 2nd Sunday in March, 2:00am local is STD --> DST
        { .rt_month =  3, .rt_mdmw = 2, .rt_wday = 7, .rt_ttloc = 120 },
        // 1st Sunday in November, 2:00am local is DST --> STD
        { .rt_month = 11, .rt_mdmw = 1, .rt_wday = 7, .rt_ttloc = 120 }
    };

    bool        retv = false;
    tziParseCtxT ctx = { NULL, NULL };

    if ((NULL != into) && (NULL != head)) {
        ctx.spHead = head;
        ctx.spTail = tail ? tail : (head + strlen(head));

        memset(into, 0, sizeof(tziPosixZoneT));

        retv = tzi_ParseName(&ctx, into->stdName, sizeof(into->stdName)) &&
               tzi_ParseTime(&ctx, &into->stdOffs, false);
        if (retv && tzi_ParseName(&ctx, into->dstName, sizeof(into->dstName))) {
            // We have two zones here. We initialize the transition rules to the
            // POSIX / US default -- it might be fully or partially overwritten
            // quite soon below. Since using the zone info after a parsing failure
            // is calling UB anyway, it doesn't matter if the if we preset before
            // parsing or trying to fit in missing parts later.
            into->dstRule = defRules[0];
            into->stdRule = defRules[1];

            // The offset for the DST zone is optional: If not given, DST is 1h ahead of
            // the zones standard time.  We save the parse position and try to parse an
            // offset; if this fails, we use the default and restore the scan position.
            head = ctx.spHead;
            if (!tzi_ParseTime(&ctx, &into->dstOffs, false)) {
                ctx.spHead = head;
                into->dstOffs = into->stdOffs - 60;
            }

            // For the transition rules, either none or two must be given.
            if (retv && ',' == tzi_PeekChar(&ctx)) {
                retv = tzi_ParseChar(&ctx, ',')
                    && tzi_ParseRule(&ctx, &into->dstRule)
                    && tzi_ParseChar(&ctx, ',')
                    && tzi_ParseRule(&ctx, &into->stdRule);
            }

            // There's a ...special... thing like a all-year DST zone.  Which is something
            // only politicans and other dorks can come up with.  Oh well, we deal with it...
            if (  retv
               && (1 == into->dstRule.rt_month)
               && (1 == into->dstRule.rt_mdmw)
               && (0 == into->dstRule.rt_wday)
               && (0 == into->dstRule.rt_ttloc)) {
                memset(&into->stdRule, 0, sizeof(into->stdRule));
            }
        }
    }
    return retv ? ctx.spHead : NULL;
}

#define EPOCH_YEAR 1970

static inline int int_min(int a, int b) {
    return (a <= b) ? a : b;
}
static inline int int_max(int a, int b) {
    return (a <= b) ? b : a;
}

static int64_t
tzi_dm2s(int32_t days, int mins) {
    return 60 * ((int64_t)days * 1440 + mins);
}

// Evaluate a POSIX rule for a given year.
static int32_t
tzi_EvalRule(tziPosixRuleT rule, int16_t year)
{
    int32_t rdn;
    if (rule.rt_wday) {
        if (5 == rule.rt_mdmw) {
            rdn = ucal_DateToRdnGD(year, rule.rt_month + 1, 0);
            rdn = ucal_WdLE(rdn, rule.rt_wday);
        } else {
            rdn = ucal_DateToRdnGD(year, rule.rt_month, 1);
            rdn = ucal_WdGE(rdn, rule.rt_wday);
            rdn += (rule.rt_mdmw - 1) * 7;
        }
    } else {
        rdn = ucal_DateToRdnGD(year, rule.rt_month, rule.rt_mdmw);
    }
    return rdn;
}

// For a given time stamp in seconds since UNIX epoch, establish the frame for
// the corresponding year.  Assumes that two valid transition rules are present,
// or "Evil Things"(tm) might happen.
static bool
tzi_CtxUpdate(tziConvCtxT* ctx, int64_t tsfrom)
{
    tziPosixZoneT const * const tzi = ctx->pTZI;

    if ((tsfrom < ctx->trLoBound - 86400) || (tsfrom >= ctx->trHiBound + 86400)) {
        int year = tsfrom / 31556952;
        year += EPOCH_YEAR - (tsfrom < year * INT64_C(31556952));

        int32_t ystart = ucal_YearStartGD(year)           - UCAL_rdnUNIX;
        int32_t ysnext = ucal_YearStartGD(year + 1)       - UCAL_rdnUNIX;
        int32_t dayDST = tzi_EvalRule(tzi->dstRule, year) - UCAL_rdnUNIX;
        int32_t daySTD = tzi_EvalRule(tzi->stdRule, year) - UCAL_rdnUNIX;

        ctx->trLoBound = tzi_dm2s(ystart, int_min(tzi->stdOffs, tzi->dstOffs));
        ctx->trHiBound = tzi_dm2s(ysnext, int_max(tzi->stdOffs, tzi->dstOffs));
        ctx->ttDST     = tzi_dm2s(dayDST, tzi->dstRule.rt_ttloc + tzi->stdOffs);
        ctx->ttSTD     = tzi_dm2s(daySTD, tzi->stdRule.rt_ttloc + tzi->dstOffs);
    }
    return true;
}

bool
tziGetInfoUtc2Local(
    tziConvInfoT *into  ,
    tziConvCtxT  *ctx   ,
    int64_t const tsfrom)
{
    if ((NULL == into) || (NULL == ctx)) {
        errno = EINVAL;
        return false;
    }
    tziPosixZoneT const * const tzi = ctx->pTZI;

    memset(into, 0, sizeof(*into));

    if (0 == tzi->dstRule.rt_month) {
        // no rule for transition to DST --> all-year STD time
        into->offs = - tzi->stdOffs * 60;
        into->isDst = 0;
    } else if (0 == tzi->stdRule.rt_month) {
        // no rule for transition to STD --> all-year DST time
        into->offs = - tzi->dstOffs * 60;
        into->isDst = 1;
    } else if (tzi_CtxUpdate(ctx, tsfrom)) {
        // zone with real STD<-->DST transitions
        int64_t ttCrit;
        int32_t ttDiff;

        // Getting the DST flag and offset is simple, but don't forget that
        // seasons flip around when crossing the equator!
        if (ctx->ttDST < ctx->ttSTD) {  // northern hemisphere (except Ireland...)
            into->isDst = (tsfrom >= ctx->ttDST) && (tsfrom < ctx->ttSTD);
        } else {                        // southern hemisphere (or Irlenad...)
            into->isDst = (tsfrom >= ctx->ttDST) || (tsfrom < ctx->ttSTD);
        }
        into->offs = -(into->isDst ? tzi->dstOffs : tzi->stdOffs) * 60;

        // Getting the Hour A/B information is a bit trickier, no thanks to
        // Ireland where someone came up with a glorious negative DST.
        if (tzi->stdOffs >= tzi->dstOffs) {
            // normal case: clock moves forward during DST, the overlap is in
            // autumn.
            ttCrit = ctx->ttSTD;
            ttDiff = (tzi->stdOffs - tzi->dstOffs) * 60;
        } else {
            // Three Cheers for Ireland, with a negative DST in Winter...
            ttCrit = ctx->ttDST;
            ttDiff = (tzi->dstOffs - tzi->stdOffs) * 60;
        }
        into->isHrA = (ttCrit - ttDiff <= tsfrom) && (tsfrom < ttCrit);
        into->isHrB = (ttCrit <= tsfrom) && (tsfrom < ttCrit + ttDiff);
    } else {
        return false;
    }

    return true;
}

bool
tziGetInfoLocal2Utc(
    tziConvInfoT *into  ,
    tziConvCtxT  *ctx   ,
    int64_t const tsfrom,
    tziCvtHintT   hint  )
{
    if ((NULL == into) || (NULL == ctx)) {
        errno = EINVAL;
        return false;
    }
    tziPosixZoneT const* const tzi = ctx->pTZI;

    memset(into, 0, sizeof(*into));

    if (0 == tzi->dstRule.rt_month) {
        // no rule for transition to DST --> all-year STD time
        into->offs = + tzi->stdOffs * 60;
        into->isDst = 0;
    } else if (0 == tzi->stdRule.rt_month) {
        // no rule for transition to STD --> all-year DST time
        into->offs = + tzi->dstOffs * 60;
        into->isDst = 1;
    } else if (tzi_CtxUpdate(ctx, (tsfrom + tzi->stdOffs * 60))) {
        // zone with real STD<-->DST transitions

        // we need both transition times in both zones to detect the pitfalls!
        int64_t ttDstA = ctx->ttDST - tzi->stdOffs * 60;
        int64_t ttDstB = ctx->ttDST - tzi->dstOffs * 60;
        int64_t ttStdA = ctx->ttSTD - tzi->dstOffs * 60;
        int64_t ttStdB = ctx->ttSTD - tzi->stdOffs * 60;
        // order the critical ranges
        if (ttDstA > ttDstB) {
            int64_t tmp = ttDstA; ttDstA = ttDstB; ttDstB = tmp;
        } else {
            int64_t tmp = ttStdA; ttStdA = ttStdB; ttStdB = tmp;
        }

        // The whole logic becomes slightly less convoluted when checking the special
        // cases first.  The whole procedure has some symmeries, but one detail is
        // asymmetric by design: The hour A/B designator will only be set when the
        // local time steps back on the transition.  One _could_ define these ranges
        // (which are only 1h when the DST<->STD diff is one hour!) as the last/first
        // step-sized interval before/after the discontinuiy.  But that would be rather
        // counter-intuitive, considering the way these flags are normally used, and so
        // we only set it when the zone transition causes a backstep of wallclock time!
        if ((tsfrom >= ttDstA) && (tsfrom < ttDstB)) {
            // plunged into STD --> DST discontinuity
            switch (hint) {
            case tziCvtHint_STD:
            case tziCvtHint_HrA:
                into->isDst = 0;
                into->isHrA = (tzi->dstOffs > tzi->stdOffs);
                break;
            case tziCvtHint_DST:
            case tziCvtHint_HrB:
                into->isDst = 1;
                into->isHrB = (tzi->dstOffs > tzi->stdOffs);
                break;
            default:
                return false;
            }
        } else if ((tsfrom >= ttStdA) && (tsfrom < ttStdB)) {
            // plunged into DST --> STD discontinuity
            switch (hint) {
            case tziCvtHint_STD:
            case tziCvtHint_HrB:
                into->isDst = 0;
                into->isHrB = (tzi->dstOffs < tzi->stdOffs);
                break;
            case tziCvtHint_DST:
            case tziCvtHint_HrA:
                into->isDst = 1;
                into->isHrA = (tzi->dstOffs < tzi->stdOffs);
                break;
            default:
                return false;
            }
        } else if (ctx->ttDST < ctx->ttSTD) {
            // northern hemisphere: spring is in March
            into->isDst = (tsfrom >= ttDstB) && (tsfrom < ttStdA);
        } else {
            // southern hemisphere: spring is in September
            into->isDst = (tsfrom >= ttDstB) || (tsfrom < ttStdA);
        }
        into->offs = (into->isDst ? tzi->dstOffs : tzi->stdOffs) * 60;
    } else {
        return false;
    }

    return true;
}

bool
tziAlignedLocalRange(
    int64_t       tlohi[2],
    tziConvInfoT *cvInfo  ,
    tziConvCtxT  *ctx     ,
    int64_t const tsfrom  ,
    int32_t       period  ,
    int32_t       phi     )
{
    bool retv = (period >      0    )
             && (period <= 7 * 86400)
             && tziGetInfoUtc2Local(cvInfo, ctx, tsfrom);
    if (retv) {
        tziPosixZoneT const * const tzi = ctx->pTZI;

        // calculating the cycle position in LOCAL time
        int32_t csoff = (tsfrom + cvInfo->offs + phi) % period;
        if (csoff < 0) {
            csoff += period;            
        }
        // apply the cycle alignment to UTC time
        tlohi[0] = tsfrom - csoff;
        tlohi[1] = tlohi[0] + period;

        // In 99.9% of all cases we'd be done now, but we have to deal with the possibility
        // of a range spanning a STD/DST transition.  We have to clamp the head and tail so
        // 'tsfrom' is _in_ the bracketed range.
        if ((0 != tzi->dstRule.rt_month) && (0 != tzi->stdRule.rt_month)) {
            if ((tlohi[0] < ctx->ttDST) && tsfrom > ctx->ttDST) {
                tlohi[0] = ctx->ttDST;
            }
            if ((tlohi[0] < ctx->ttSTD) && tsfrom > ctx->ttSTD) {
                tlohi[0] = ctx->ttSTD;
            }
            if ((tlohi[1] > ctx->ttDST) && tsfrom < ctx->ttDST) {
                tlohi[1] = ctx->ttDST;
            }
            if ((tlohi[1] > ctx->ttSTD) && tsfrom < ctx->ttSTD) {
                tlohi[1] = ctx->ttSTD;
            }
        }
    }
    return retv;
}

// -*- that's all folks -*-
