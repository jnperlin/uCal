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
// This module contains unit / regression test code for the 'unity' UT framework.
// ----------------------------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/random.h>
#include <time.h>
#include <unity.h>
#include <string.h>

#include "ucal/common.h"
#include "ucal/gregorian.h"
#include "ucal/julian.h"
#include "ucal/ntpdate.h"
#include "ucal/gpsdate.h"

void
setUp(void)
{
  //NOP
}

void tearDown(void)
{
  //NOP
}

#define tabsize 1024


#if INT_MAX >= INT32_MAX
# define DIV_F div
# define DIV_T div_t
#else
# error no matching 'div()' function available!?!
#endif

#ifdef __GNUC__
__attribute__((noinline))
#endif
static int32_t asr_by_div(int32_t x, int s)
{
    DIV_T qr = { .quot = x, .rem = 0 };
    while (s > 0) {
        int s0 = (s > 16) ? 16 : s;
        qr = DIV_F(qr.quot, (1 << s0));
        qr.quot -= (qr.rem < 0);
        s -= s0;
    }
    return qr.quot;
}

static void test_asrONE(void) {
    for (int s = 0; s < 32; ++s) {
        int32_t q1 = ucal_i32Asr(-1, s);
        int32_t q2 = asr_by_div(-1, s);
        TEST_ASSERT_EQUAL(q2, q1);
    }
}

static void test_asrMAX(void) {
    for (int s = 0; s < 32; ++s) {
        int32_t q1 = ucal_i32Asr(INT32_MIN, s);
        int32_t q2 = asr_by_div(INT32_MIN, s);
        TEST_ASSERT_EQUAL(q2, q1);
    }
}

static int ref_mod7(int64_t v64)
{
    int r = (int)(v64 % 7);
    if (r < 0) {
        r += 7;
    }
    return r;
}

static void test_mod7(void) {
    static int32_t table[tabsize];
    int r1, r2;

    TEST_ASSERT_EQUAL(sizeof(table), getrandom(table, sizeof(table), 0));

    for (unsigned i = 0; i < tabsize; ++i) {
      r1 = ucal_i32Mod7(table[i]);
      r2 = ref_mod7(table[i]);
      TEST_ASSERT_EQUAL(r1, r2);

      for (unsigned j = 0; j < tabsize; ++j) {
        r1 = ucal_i32AddMod7(table[i], table[j]);
        r2 = ref_mod7((int64_t)table[i] + (int64_t)table[j]);
        TEST_ASSERT_EQUAL(r1, r2);

        r1 = ucal_i32SubMod7(table[i], table[j]);
        r2 = ref_mod7((int64_t)table[i] - (int64_t)table[j]);
        TEST_ASSERT_EQUAL(r1, r2);
      }
    }
}

static void test_dsplit(void)
{
  ucal_TimeDivT QR;

  QR = ucal_TimeToDays(INT64_MAX);
  TEST_ASSERT_EQUAL(INT64_C(106751991167300), QR.q);
  TEST_ASSERT_EQUAL(UINT32_C(55807), QR.r);

  QR = ucal_TimeToDays(INT64_MIN);
  TEST_ASSERT_EQUAL(INT64_C(-106751991167301), QR.q);
  TEST_ASSERT_EQUAL(UINT32_C(30592), QR.r);
}


static void test_wdshift(void) {
    int32_t base = 5 * 146097 + 1;  // Monday, 2001-01-01
    for (int i = 1; i < 7; ++i) {
        TEST_ASSERT_EQUAL((base + i - 1), ucal_WdGE(base, i));
        TEST_ASSERT_EQUAL((base - i + 1), ucal_WdLE(base, 2 - i));
    }
}

static void test_Date2Rdn(void) {
    TEST_ASSERT_EQUAL(5 * 146097 + 1, ucal_DateToRdnGD(2001, 1, 1));
}

static void test_reform1(void) {
    int32_t rdn1, rdn2;

    rdn1 = ucal_DateToRdnGD(1582, 10, 15);
    rdn2 = ucal_DateToRdnJD(1582, 10, 5);
    TEST_ASSERT_EQUAL(rdn1, rdn2);

    rdn1 = ucal_DateToRdnGD(1582, 10, 14);
    rdn2 = ucal_DateToRdnJD(1582, 10, 4);
    TEST_ASSERT_EQUAL(rdn1, rdn2);

    rdn1 = ucal_DateToRdnGD(1582, 10, 15);
    rdn2 = ucal_DateToRdnJD(1582, 10, 4);
    TEST_ASSERT_EQUAL(rdn1, rdn2 + 1);
}

static void test_reform2(void) {
    ucal_CivilDateT dtb;
    int32_t rdn;

    rdn = ucal_DateToRdnGD(1582, 10, 15);
    TEST_ASSERT_TRUE(ucal_RdnToDateJD(&dtb, rdn - 1));
    TEST_ASSERT_EQUAL(1582, dtb.dYear);
    TEST_ASSERT_EQUAL(10, dtb.dMonth);
    TEST_ASSERT_EQUAL(4, dtb.dMDay);
    TEST_ASSERT_EQUAL(4, dtb.dWDay);

    rdn = ucal_DateToRdnJD(1582, 10, 4);
    TEST_ASSERT_TRUE(ucal_RdnToDateGD(&dtb, rdn + 1));
    TEST_ASSERT_EQUAL(1582, dtb.dYear);
    TEST_ASSERT_EQUAL(10, dtb.dMonth);
    TEST_ASSERT_EQUAL(15, dtb.dMDay);
    TEST_ASSERT_EQUAL(5, dtb.dWDay);
}

static void test_rellez(void) {
    int16_t y;

    y = ucal_RellezGD(82, 10, 15, ucal_wdFRI, 1500);
    TEST_ASSERT_EQUAL(1582, y);

    y = ucal_RellezJD(82, 10, 4, ucal_wdTHU, 1500);
    TEST_ASSERT_EQUAL(1582, y);
}

static void test_ysplitGD(void) {
    for (int i = -100; i <= 100; ++i) {
        bool lyf;
        ucal_iu32DivT act = ucal_DaysToYearsGD((i * 146097 + 60), &lyf);
        int32_t exp = i * 400;
        TEST_ASSERT_EQUAL(exp, act.q);
        TEST_ASSERT_EQUAL(59, act.r);
        TEST_ASSERT_FALSE(lyf);
    }
    //for (int i = -100; i <= 100; ++i) {
    for (int i = 0; i <= 100; ++i) {
        bool lyf;
        ucal_iu32DivT act = ucal_DaysToYearsGD((i * 146097 - 305), &lyf);
        int32_t exp = i * 400 - 1;
        TEST_ASSERT_EQUAL(exp, act.q);
        TEST_ASSERT_EQUAL(60, act.r);
        TEST_ASSERT_TRUE(lyf);
      }
}

static void test_BuildDate(void) {
    TEST_ASSERT_GREATER_OR_EQUAL(0, ucal_BuildDateRdn());
}

#define RDN(y, m, d) ucal_DateToRdnGD((y), (m), (d))

#define DDD(a, b) (ucal_DateToRdnGD a - ucal_DateToRdnGD b)
#define DDT(a, b) ((ucal_DateToRdnGD a - ucal_DateToRdnGD b) * (time_t)86400)

static void test_ntpDate(void) {
    time_t act, exp, base;
    uint32_t ntpSec;

    // test origin of UNIX epoch
    base = 0;
    ntpSec = DDT((1970, 1, 1), (1900, 1, 1));    
    exp = 0;

    act = ucal_NtpToTime(ntpSec, &base);
    printf("exp = %s", ctime(&exp));
    printf("act = %s", ctime(&act));
    TEST_ASSERT_EQUAL(exp, act);

    base = DDT((2024,8,18), (1970,1,1));
    act = ucal_NtpToTime(0, &base);
    exp = DDT((1900, 1, 1), (1970, 1, 1)) + INT64_C(0x100000000);
    printf("exp = %s", ctime(&exp));
    printf("act = %s", ctime(&act));
    TEST_ASSERT_EQUAL(exp, act);

    ntpSec = ucal_TimeToNtp(exp);
    TEST_ASSERT_EQUAL(0, ntpSec);
}

static void test_gpsDate1(void) {
    int32_t       base;
    ucal_iu32DivT exp, act;

    // era zero:
    base = RDN(1980, 1, 6);
    exp = (ucal_iu32DivT){ .q = base, .r = 0 };
    act = ucal_GpsMapRaw1(0, 0, 0, base);
    TEST_ASSERT_EQUAL(0, memcmp(&exp, &act, sizeof(ucal_iu32DivT)));

    // era one:
    base += 1024 * 7;
    exp = (ucal_iu32DivT){ .q = base, .r = 0 };
    act = ucal_GpsMapRaw1(0, 0, 0, base);
    TEST_ASSERT_EQUAL(0, memcmp(&exp, &act, sizeof(ucal_iu32DivT)));

    base -= 100 * 7;
    act = ucal_GpsMapRaw1(0, 0, 0, base);
    TEST_ASSERT_EQUAL(0, memcmp(&exp, &act, sizeof(ucal_iu32DivT)));
}

static void test_gpsDate2(void) {
    static const int32_t wcycle = INT32_C(604800);

    time_t base, exp, act;

    // era zero:
    base = DDT((1980, 1, 6), (1970, 1, 1));
    exp = base;
    act = ucal_GpsMapRaw2(0, 0, 0, &base);
    TEST_ASSERT_EQUAL(exp, act);

    // era one:
    base += 1024 * wcycle;
    exp = base;
    act = ucal_GpsMapRaw2(0, 0, 0, &base);
    TEST_ASSERT_EQUAL(exp, act);

    base -= 100 * wcycle;
    act = ucal_GpsMapRaw2(0, 0, 0, &base);
    TEST_ASSERT_EQUAL(exp, act);
}

int  main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_asrONE);
    RUN_TEST(test_asrMAX);
    RUN_TEST(test_BuildDate);
    RUN_TEST(test_mod7);
    RUN_TEST(test_dsplit);
    RUN_TEST(test_wdshift);
    RUN_TEST(test_ysplitGD);
    RUN_TEST(test_Date2Rdn);
    RUN_TEST(test_reform1);
    RUN_TEST(test_reform2);
    RUN_TEST(test_rellez);
    RUN_TEST(test_ntpDate);
    RUN_TEST(test_gpsDate1);
    RUN_TEST(test_gpsDate2);
    return UNITY_END();
}
