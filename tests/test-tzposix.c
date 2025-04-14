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

static tziPosixZoneT Berlin;


void setUp(void)
{
    // NOP
}

void tearDown(void)
{
  // NOP
}

static void
test_spring2025(void) {
    tziConvCtxT ctx;
    tziConvInfoT info;
    bool cret;

    memset(&ctx, 0, sizeof(ctx));
    ctx.pTZI = &Berlin;

    cret = tziFromPosixSpec(&Berlin, "CET-1CEST-2,M3.5.0/2,M10.5.0/3", NULL);

    int64_t ts = (ucal_DateToRdnGD(2025, 3, 30) - ucal_YearStartGD(1970)) * 86400 + 7200 + 1800;

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_None);
    TEST_ASSERT_FALSE(cret);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrA);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrB);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -7200 == info.offs);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts - 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts + 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -7200 == info.offs);
}

static void
test_autumn2025(void) {
    tziConvCtxT ctx;
    tziConvInfoT info;
    bool cret;

    memset(&ctx, 0, sizeof(ctx));
    ctx.pTZI = &Berlin;

    cret = tziFromPosixSpec(&Berlin, "CET-1CEST-2,M3.5.0/2,M10.5.0/3", NULL);

    int64_t ts = (ucal_DateToRdnGD(2025, 10, 26) - ucal_YearStartGD(1970)) * 86400 + 7200 + 1800;

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_None);
    TEST_ASSERT_FALSE(cret);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrB);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts, tziCvtHint_HrA);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -7200 == info.offs);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts + 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(0 == info.isDst && -3600 == info.offs);

    cret = tziGetInfoLocal2Utc(&info, &ctx, ts - 3600, tziCvtHint_None);
    TEST_ASSERT_TRUE(cret);
    TEST_ASSERT(1 == info.isDst && -7200 == info.offs);
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
    RUN_TEST(test_spring2025);
    RUN_TEST(test_autumn2025);
    return UNITY_END();
}
