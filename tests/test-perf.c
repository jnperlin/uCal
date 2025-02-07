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
// This module contains a round-robin & performance test using the 'unity' UT framework.
// ----------------------------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/random.h>
#include <time.h>
#include <unity.h>

#include "ucal/common.h"
#include "ucal/gpsdate.h"
#include "ucal/gregorian.h"
#include "ucal/julian.h"
#include "ucal/ntpdate.h"

#if defined(CLOCK_THREAD_CPUTIME_ID)
# define MYCLCOCK CLOCK_THREAD_CPUTIME_ID
#elif defined(CLOCK_PROCESS_CPUTIME_ID)
# define MYCLCOCK CLOCK_PROCESS_CPUTIME_ID
#elif defined(CLOCK_MONOTONIC_RAW)
# define MYCLCOCK CLOCK_MONOTONIC_RAW
#elif defined(CLOCK_MONOTONIC)
# define MYCLCOCK CLOCK_MONOTONIC
#else
# define MYCLCOCK CLOCK_MONOTONIC_ID
#endif

void
setUp(void)
{
  // NOP
}

void
tearDown(void)
{
  // NOP
}

static void test_ucalPerf(void) {
    ucal_CivilTimeT ct;
    ucal_CivilDateT cd;
    struct timespec tbeg, tend;
    clock_gettime(MYCLCOCK, &tbeg);
    for (int loops = 10; loops; --loops) {
        for (int32_t day = -24855; day <= 24855; ++day) {
            time_t tt = (time_t)day * 86400 + 43200;

            ucal_TimeDivT dt = ucal_TimeToRdn(tt);
            dt.q += ucal_DayTimeSplit(&ct, dt.r, 0);
            ucal_RdnToDateGD(&cd, (int32_t)dt.q);

            time_t tx = (time_t)(ucal_DateToRdnGD(cd.dYear, cd.dMonth, cd.dMDay) - UCAL_rdnUNIX);
            tx = tx * 86400 + ucal_DayTimeMerge(ct.tHour, ct.tMin, ct.tSec);

            //printf("day=%d  tt=%lld\n", day, (long long)tt);
            TEST_ASSERT_EQUAL(tt, tx);
        }
    }
    clock_gettime(MYCLCOCK, &tend);
    tend.tv_sec  -= tbeg.tv_sec;
    tend.tv_nsec -= tbeg.tv_nsec;
    if (tend.tv_nsec < 0) {
        tend.tv_nsec += 1000000000L;
        tend.tv_sec  -= 1;
    }
    printf("execution time was %ld.%06ld\n",
           (long)tend.tv_sec,
           (long)(tend.tv_nsec / 1000));
}

static void test_libcPerf(void) {
    struct tm dtb;
    struct timespec tbeg, tend;

    clock_gettime(MYCLCOCK, &tbeg);
    for (int loops = 10; loops; --loops) {
        for (int32_t day = -24855; day <= 24855; ++day) {
            time_t tt = (time_t)day * 86400 + 43200;
            gmtime_r(&tt, &dtb);
            time_t tx = timegm(&dtb);

            // printf("day=%d  tt=%lld\n", day, (long long)tt);
            TEST_ASSERT_EQUAL(tt, tx);
        }
    }
    clock_gettime(MYCLCOCK, &tend);
    tend.tv_sec  -= tbeg.tv_sec;
    tend.tv_nsec -= tbeg.tv_nsec;
    if (tend.tv_nsec < 0) {
        tend.tv_nsec += 1000000000L;
        tend.tv_sec  -= 1;
    }
    printf("execution time was %ld.%06ld\n",
           (long)tend.tv_sec,
           (long)(tend.tv_nsec / 1000));
}


int main(int argc, char **argv)
{
    (void)(argc),(void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_ucalPerf);
    RUN_TEST(test_libcPerf);
    return UNITY_END();
}
// -*- that's allk folks -*-
