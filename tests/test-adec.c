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

#include "ucal/common.h"
#include "ucal/tsdecode.h"

#include <unity.h>

void
setUp(void)
{
    //NOP
}

void tearDown(void)
{
    //NOP
}

static void test_pfrac(void) {
  ucal_u32DivT f;
  const char* str;

  str = ".5";
  f = ucal_decFrac(&str, NULL);
  TEST_ASSERT_EQUAL(UINT32_C(0x80000000), f.r);

  str = ".0625";
  f = ucal_decFrac(&str, NULL);
  TEST_ASSERT_EQUAL(UINT32_C(0x10000000), f.r);

  str = ".999999999999999999999999999999999999999999999999";
  f = ucal_decFrac(&str, NULL);
  TEST_ASSERT_EQUAL(UINT32_C(0x00000000), f.r);
  TEST_ASSERT_EQUAL(1u, f.q);

  str = ".50000000023283064365386962890624";
  f = ucal_decFrac(&str, NULL);
  TEST_ASSERT_EQUAL(UINT32_C(0x80000001), f.r);

  str = ".500000000116415321826934814453125";
  f = ucal_decFrac(&str, NULL);
  TEST_ASSERT_EQUAL(UINT32_C(0x80000000), f.r);

}




static void test_UtcTm(void) {

}

static void test_GenTm(void) {
    struct timespec ts;
    const char* str;
    str = "19700101000000.0-0100";
    TEST_ASSERT_TRUE(ucal_decASN1GenTime24(&ts, &str, NULL));
    TEST_ASSERT_EQUAL(+3600, ts.tv_sec);
    TEST_ASSERT_EQUAL(0, ts.tv_nsec);

    str = "19700101000000.010";
    TEST_ASSERT_TRUE(ucal_decASN1GenTime24(&ts, &str, NULL));
}

static unsigned
double_up(
    uint8_t *dbuf,
    unsigned ndig,
    uint8_t  base,
    bool     cf  )
{
    uint_fast16_t accu, flag;

    flag = -1 + !cf;
    while (ndig--) {
        accu = (*dbuf << 1) - flag;
        flag = -(accu >= base);
        *dbuf++ = (uint8_t)(accu - (flag & base));
    }
    return (flag & 1u);
}

static unsigned
double_dn(
    uint8_t *dbuf,
    unsigned ndig,
    uint8_t  base,
    bool     cf  )
{
    uint_fast16_t accu, flag;

    dbuf += ndig;
    flag = -1 + !cf;
    while (ndig--) {
        accu = (*--dbuf << 1) - flag;
        flag = -(accu >= base);
        *dbuf = (uint8_t)(accu - (flag & base));
    }
    return (flag & 1u);
}

static unsigned
rabble_up(
    uint8_t *dbuf,
    unsigned ndig,
    uint8_t  base,
    bool     cf  )
{
    uint_fast16_t accu, flag;

    flag = -1 + !cf;
    while (ndig--) {
        accu = (flag & base) + *dbuf;
        flag = 0u - (accu & 1u);
        *dbuf++ = (uint8_t)(accu >> 1);
    }
    return (flag & 1u);
}

static unsigned
rabble_dn(
    uint8_t *dbuf,
    unsigned ndig,
    uint8_t  base,
    bool     cf  )
{
    uint_fast16_t accu, flag;

    dbuf += ndig;
    flag = -1 + !cf;
    while (ndig--) {
        accu = (flag & base) + *--dbuf;
        flag = 0u - (accu & 1u);
        *dbuf = (uint8_t)(accu >> 1);
    }
    return (flag & 1u);
}



int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    (void)rabble_dn;
    (void)rabble_up;
    (void)double_dn;
    (void)double_up;

    UNITY_BEGIN();
    RUN_TEST(test_pfrac);
    RUN_TEST(test_UtcTm);
    RUN_TEST(test_GenTm);
    return UNITY_END();
}
