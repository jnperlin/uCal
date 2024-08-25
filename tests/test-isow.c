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
#include <string.h>
#include <errno.h>
#include <unity.h>

#include "ucal/common.h"
#include "ucal/gregorian.h"
#include "ucal/isoweek.h"

void
setUp(void)
{
  //NOP
}

void tearDown(void)
{
  //NOP
}

static int32_t wcYearsStart(int32_t ey)
{
    int32_t rdn;
    rdn = (ey * 365) + ucal_LeapDaysInYearsGD(ey) + 1;
    rdn = ucal_WdNear(rdn, ucal_wdMON);
    return rdn;
}

static bool tref_RdnToDateWD(ucal_WeekDateT *into, int32_t rdn)
{
    bool retv = false;

    int32_t y0  = ucal_DaysToYearsGD(rdn, NULL).q;
    int32_t dlo = wcYearsStart(y0 + 0);
    int32_t dhi = wcYearsStart(y0 + 1);

    if (rdn >= dhi) {
        int loops = 10;
        do {
            if (!--loops)
              return false;
            ++y0;
            dlo = dhi;
            dhi = wcYearsStart(y0 + 1);
        } while (rdn >= dhi);
    } else if (rdn < dlo) {
      int loops = 10;
      do {
        if (!--loops)
          return false;
        --y0;
        dhi = dlo;
        dlo = wcYearsStart(y0 + 0);
      } while (rdn < dlo);
    }

    ++y0;
    if (y0 > INT16_MAX) {
      into->dYear = INT16_MAX;
      errno = ERANGE;
    } else if (y0 < INT16_MIN) {
      into->dYear = INT16_MIN;
      errno = ERANGE;
    } else {
      into->dYear = (int16_t)y0;
      retv = true;
    }
    rdn -= dlo;
    into->dWDay = (rdn % 7u) + 1;
    into->dWeek = (rdn / 7u) + 1;
    return retv;
    }

static void
test_ystart(void)
{
    for (int32_t y = INT16_MIN; y <= INT16_MAX; ++y) {
        int32_t exp, act;
        exp = ucal_WdNear(ucal_YearStartGD((int16_t)y), ucal_wdMON);
        act = ucal_YearStartWD((int16_t)y);
        TEST_ASSERT_EQUAL(exp, act);
    }
}

static void test_ysplit(void) {
    const int32_t dLo = ucal_YearStartWD(INT16_MIN);
    const int32_t dHi = ucal_YearStartWD(INT16_MAX) + 52*7;
    for (int32_t rdn = dLo; rdn < dHi; ++rdn) {
        ucal_WeekDateT wdAct, wdExp;
        memset(&wdExp, 0, sizeof(wdExp));
        memset(&wdAct, 0, sizeof(wdAct));
        TEST_ASSERT_TRUE(ucal_RdnToDateWD(&wdAct, rdn));
        TEST_ASSERT_TRUE(tref_RdnToDateWD(&wdExp, rdn));
        TEST_ASSERT_FALSE(memcmp(&wdAct, &wdExp, sizeof(ucal_WeekDateT)));
    }
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_ystart);
    RUN_TEST(test_ysplit);
    return UNITY_END();
}
// -*- that's all folks -*-
