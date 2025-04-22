// -*- mode: C; c-file-style: "bsd"; c-basic-offset: 4; fill-column: 98; coding:
// utf-8-unix; -*-
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
// unit tests for POSIX-like time zone info
// ----------------------------------------------------------------------------------------------

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "ucal/common.h"
#include "ucal/tzposix.h"

#include "ucal/gregorian.h"
#include <unity.h>

static const tziPosixZoneT Berlin2 = {
    .stdName = "CET",
    .dstName = "CEST",
    .stdOffs = -60,
    .dstOffs = -120,
    .dstRule = { .rt_month =  3, .rt_mdmw  = 5, .rt_wday  = 7, .rt_ttloc = 120 },
    .stdRule = { .rt_month = 10, .rt_mdmw  = 5, .rt_wday  = 7, .rt_ttloc = 180 },
};

static tziPosixZoneT Berlin, Dublin, Auckland;


void setUp(void)
{
    // NOP
}

void tearDown(void)
{
  // NOP
}

// -------------------------------------------------------------------------------------
// see if we can successfully parse all the unique zone descriptions extracted from
// 'https://ftp.fau.de/aminet/util/time/tzinfo.txt'
static void
test_ParseZones(void)
{
    static const char * const zoneTab[] = {
        "ACST-9", "AEST-10", "AEST-10AEDT,M10.1.0,M4.1.0/3", "AKST9AKDT,M3.2.0,M11.1.0",
        "AST4", "AST4ADT,M3.2.0,M11.1.0", "AWST-8", "CAT-2", "CET-1", "CET-1CEST,M3.5.0,M10.5.0/3",
        "CST5CDT,M3.2.0/0,M11.1.0/1", "CST6", "CST6CDT,M3.2.0,M11.1.0", "CST6CDT,M4.1.0,M10.5.0",
        "CST-8", "EAT-3", "EET-2", "EET-2EEST,M3.5.0/0,M10.5.0/0", "EET-2EEST,M3.5.0/3,M10.5.0/4",
        "EET-2EEST,M3.5.0,M10.5.0/3", "EET-2EEST,M3.5.4/24,M10.5.5/1", "EET-2EEST,M3.5.5/0,M10.5.5/0",
        "EET-2EEST,M3.5.5/0,M10.5.6/1", "EST5", "EST5EDT,M3.2.0,M11.1.0", "GMT0", "GMT0BST,M3.5.0/1,M10.5.0",
        "<GMT+10>-10", "<GMT-10>+10", "<GMT+1>-1", "<GMT-1>+1", "<GMT+11>-11", "<GMT-11>+11", "<GMT+12>-12",
        "<GMT+13>-13",  "<GMT+14>-14", "<GMT-2>+2", "<GMT+3>-3", "<GMT-3>+3", "<GMT+4>-4", "<GMT-4>+4",
        "<GMT+5>-5", "<GMT-5>+5", "<GMT+6>-6", "<GMT-6>+6", "<GMT+7>-7", "<GMT+8>-8", "<GMT-8>+8",
        "<GMT+9>-9", "<GMT-9>+9", "HKT-8", "HST10", "HST10HDT,M3.2.0,M11.1.0", "IST-1GMT0,M10.5.0,M3.5.0/1",
        "IST-5", "JST-9", "KST-9", "MSK-3", "MST7", "MST7MDT,M3.2.0,M11.1.0", "MST7MDT,M4.1.0,M10.5.0",
        "NST3", "NZST-12NZDT,M9.5.0,M4.1.0/3", "PKT-5", "PST-8", "PST8PDT,M3.2.0,M11.1.0", "SAST-2",
        "SST11", "WAT-1", "WET0WEST,M3.5.0/1,M10.5.0", "WIB-7", "WIT-9", "WITA-8",
        NULL
    };

    tziPosixZoneT zone;
    const char *item, *pret, *const *tptr;

    for (tptr = zoneTab; NULL != (item = *tptr); ++tptr) {
        pret = tziFromPosixSpec(&zone, item, NULL);
        TEST_ASSERT_MESSAGE((pret && !*pret), item);
    }
}

// -------------------------------------------------------------------------------------
// Berin (CET-1CEST,M3.5.0,M10.5.0/3) comes natural for a German
static void
test_BerlinSpring2025(void) {
    tziConvCtxT ctx;
    tziConvInfoT info;
    bool cret;
    const char* pret;

    memset(&ctx, 0, sizeof(ctx));
    ctx.pTZI = &Berlin;

    pret = tziFromPosixSpec(&Berlin, "CET-1<CEST>-2,M3.5.0/2,M10.5.0/3", NULL);
    TEST_ASSERT_TRUE(pret && !*pret);
    TEST_ASSERT_EQUAL(0, memcmp(&Berlin, &Berlin2, sizeof(tziPosixZoneT)));

    int64_t ts = (ucal_DateToRdnGD(2025, 3, 30) - ucal_YearStartGD(1970)) * 86400 + 7200 + 1800;

    // Plunging into the gap must fail without a full hint:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_None);
    TEST_ASSERT_FALSE(cret);

    // hint to yield STD time:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrA);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_STD);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    // hint to yield DST time:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrB);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -7200 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_DST);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -7200 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    // outside the critical range must succeed without hint:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts - 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts + 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -7200 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);
}

static void
test_BerlinAutumn2025(void) {
    tziConvCtxT ctx;
    tziConvInfoT info;
    bool cret;
    const char *pret;

    memset(&ctx, 0, sizeof(ctx));
    ctx.pTZI = &Berlin;

    pret = tziFromPosixSpec(&Berlin, "CET-1CEST-2,M3.5.0/2,M10.5.0/3", NULL);
    TEST_ASSERT_TRUE(pret && !*pret);

    int64_t ts = (ucal_DateToRdnGD(2025, 10, 26) - ucal_YearStartGD(1970)) * 86400 + 7200 + 1800;

    // conversion in the repeated hour fails without explicit hint:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_None);
    TEST_ASSERT_FALSE(cret);

    // hint to yield STD time:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrB);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 1 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_STD);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 1 == info.isHrB);

    // hint to yield DST time:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrA);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -7200 == info.offs);
    TEST_ASSERT(1 == info.isHrA && 0 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_DST);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -7200 == info.offs);
    TEST_ASSERT(1 == info.isHrA && 0 == info.isHrB);

    // times not in the critical hour must resolve without hint:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts + 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts - 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -7200 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);
}

// -------------------------------------------------------------------------------------
// test New Zealand (NZST-12NZDT,M9.5.0,M4.1.0/3) for southern hemisphere:

static void
test_AucklandSpring2025(void) {
    tziConvCtxT ctx;
    tziConvInfoT info;
    bool cret;
    const char* pret;

    memset(&ctx, 0, sizeof(ctx));
    ctx.pTZI = &Auckland;

    pret = tziFromPosixSpec(&Auckland, "NZST-12NZDT,M9.5.0,M4.1.0/3", NULL);
    TEST_ASSERT_TRUE(pret && !*pret);

    int64_t ts = (ucal_DateToRdnGD(2025, 9, 28) - ucal_YearStartGD(1970)) * 86400 + 7200 + 1800;

    // Plunging into the gap must fail without a full hint:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_None);
    TEST_ASSERT_FALSE(cret);

    // hint to yield STD time:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrA);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -43200 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_STD);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -43200 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    // hint to yield DST time:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrB);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -46800 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_DST);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -46800 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    // outside the critical range must succeed without hint:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts - 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -43200 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts + 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -46800 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);
}

static void
test_AucklandAutumn2025(void) {
    tziConvCtxT ctx;
    tziConvInfoT info;
    bool cret;
    const char *pret;

    memset(&ctx, 0, sizeof(ctx));
    ctx.pTZI = &Auckland;

    pret = tziFromPosixSpec(&Auckland, "NZST-12NZDT,M9.5.0,M4.1.0/3", NULL);
    TEST_ASSERT_TRUE(pret && !*pret);

    int64_t ts = (ucal_DateToRdnGD(2025, 4, 6) - ucal_YearStartGD(1970)) * 86400 + 7200 + 1800;

    // conversion in the repeated hour fails without explicit hint:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_None);
    TEST_ASSERT_FALSE(cret);

    // hint to yield STD time:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrB);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -43200 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 1 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_STD);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -43200 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 1 == info.isHrB);

    // hint to yield DST time:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrA);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -46800 == info.offs);
    TEST_ASSERT(1 == info.isHrA && 0 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_DST);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -46800 == info.offs);
    TEST_ASSERT(1 == info.isHrA && 0 == info.isHrB);

    // times not in the critical hour must resolve without hint:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts + 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -43200 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts - 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -46800 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);
}

// -------------------------------------------------------------------------------------
// Dublin (IST-1GMT0,M10.5.0,M3.5.0/1)
//
// Testing the Dublin (aka Irish) rules is mind-boggling, as they effectively reverse
// the sense of everything: Their 'summer time' is effectively a special winter time,
// starting in autumn and ending in spring, and of course the winter time is _behind_
// the standard time, which is only applied in summer.
//
// Which unfortunately even makes some sense, as the EU rules have only 5 months of
// standard time and 7 months of DST.  Which, by majority, would make the DST the
// standard...

static void
test_DublinSpring2025(void) {
    tziConvCtxT ctx;
    tziConvInfoT info;
    bool cret;
    const char* pret;

    memset(&ctx, 0, sizeof(ctx));
    ctx.pTZI = &Dublin;

    pret = tziFromPosixSpec(&Dublin, "IST-1GMT0,M10.5.0,M3.5.0/1", NULL);
    TEST_ASSERT_TRUE(pret && !*pret);

    int64_t ts = (ucal_DateToRdnGD(2025, 3, 30) - ucal_YearStartGD(1970)) * 86400 + 3600 + 1800;

    // Plunging into the gap must fail without a full hint:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_None);
    TEST_ASSERT_FALSE(cret);

    // hint to yield STD time:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrB);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_STD);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    // hint to yield DST time:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrA);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && 0 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_DST);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && 0 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    // outside the critical range must succeed without hint:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts + 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts - 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && 0 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);
}

static void
test_DublinAutumn2025(void) {
    tziConvCtxT ctx;
    tziConvInfoT info;
    bool cret;
    const char* pret;

    memset(&ctx, 0, sizeof(ctx));
    ctx.pTZI = &Dublin;

    pret = tziFromPosixSpec(&Dublin, "IST-1GMT0,M10.5.0,M3.5.0/1", NULL);
    TEST_ASSERT_TRUE(pret && !*pret);

    int64_t ts = (ucal_DateToRdnGD(2025, 10, 26) - ucal_YearStartGD(1970)) * 86400 + 3600 + 1800;

    // Plunging into the gap must fail without a full hint:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_None);
    TEST_ASSERT_FALSE(cret);

    // hint to yield STD time:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrA);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);
    TEST_ASSERT(1 == info.isHrA && 0 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_STD);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);
    TEST_ASSERT(1 == info.isHrA && 0 == info.isHrB);

    // hint to yield DST time:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrB);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && 0 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 1 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_DST);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && 0 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 1 == info.isHrB);

    // outside the critical range must succeed without hint:
    cret = tziGetInfoLocal2Utc(&info, &ctx, ts + 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && 0 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts - 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);
    TEST_ASSERT(0 == info.isHrA && 0 == info.isHrB);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    tziConvCtxT ctx;
    tziConvInfoT info;

    memset(&ctx, 0, sizeof(ctx));
    ctx.pTZI = &Berlin2;
    tziGetInfoUtc2Local(&info, &ctx, time(NULL));

    (void)argc, (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_ParseZones);
    RUN_TEST(test_BerlinSpring2025);
    RUN_TEST(test_BerlinAutumn2025);
    RUN_TEST(test_AucklandSpring2025);
    RUN_TEST(test_AucklandAutumn2025);
    RUN_TEST(test_DublinSpring2025);
    RUN_TEST(test_DublinAutumn2025);
    return UNITY_END();
}
