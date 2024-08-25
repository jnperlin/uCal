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
// This module contains definitions for internal common utilities and API types.
// ----------------------------------------------------------------------------------------------
#ifndef COMMON_H_D2078C60_0B6B_439F_B110_087913F54042
#define COMMON_H_D2078C60_0B6B_439F_B110_087913F54042

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
# define CDECL_BEG extern "C" {
# define CDECL_END }
#else
# define CDECL_BEG
# define CDECL_END
#endif

CDECL_BEG

// ----------------------------------------------------------------------------------------------
// Days of month, shifted, zero-based. For validation only.
extern const uint8_t _ucal_sdtab[2][12];

typedef enum {
    ucal_wdSUN0 = 0,
    ucal_wdMON  = 1,
    ucal_wdTUE,
    ucal_wdWED,
    ucal_wdTHU,
    ucal_wdFRI,
    ucal_wdSAT,
    ucal_wdSUN
} ucal_WeekDayT;

// -------------------------------------------------------------------------------------
// Conversions from signed to unsigned int is well-defined, bu that's not necessarily
// true for the other direction. Many (most?) compilers do it the pattern-preserving way
// and that's the easiest option.  But at least on two's complement machines there's a
// way that does not exhibit UB and does not rely on the goodwill of the compiler.
// (Note: Clang & GCC are clever enough to recognise the pattern and elide the operation,
// resulting in a simple MOVE or even nothing!)
// -------------------------------------------------------------------------------------

static inline int32_t ucal_u32_i32(uint32_t v) {
    return (v > INT32_MAX) ? -(int32_t)(~v) - 1 : (int32_t)v;
}

static inline int64_t ucal_u64_i64(uint64_t v) {
    return (v > INT64_MAX) ? -(int64_t)(~v) - 1 : (int64_t)v;
}

// -------------------------------------------------------------------------------------
// Extraction lo/hi DWORD from a 64bit value.  Whiel formally using shifts and masks,
// most compilers will just do the "right thing (TM)" by selecting register (parts).
// -------------------------------------------------------------------------------------

static inline uint32_t ucal_u64lo(uint64_t v) {
    return (uint32_t)(v & UINT32_MAX);
}

static inline uint32_t ucal_u64hi(uint64_t v) {
    return (uint32_t)((v >> 32) & UINT32_MAX);
}


// -------------------------------------------------------------------------------------
// We need arithmetic shift right on negative signed integers.  Some compilers do,
// others don't.  We have to work around this.
// -------------------------------------------------------------------------------------
#ifndef MACHINE_ASR
# if (-1 >> 1) == -1
#  define MACHINE_ASR 1
# else
#  define MACHINE_ASR 0
# endif
#endif

#if MACHINE_ASR
static inline int32_t ucal_i32Asr(int32_t v, unsigned s) {
    return v >> s;
}
static inline int64_t ucal_i64Asr(int64_t v, unsigned s) {
    return v >> s;
}
#else
static inline int32_t ucal_i32Asr(int32_t v, unsigned s) {
    uint32_t m = -(v < 0);
    uint32_t u = m ^ ((m ^ v) >> s);
    return ucal_u32_i32(u);
}
static inline int64_t ucal_i64Asr(int64_t v, unsigned s) {
    uint64_t m = -(v < 0);
    uint64_t u = m ^ ((m ^ v) >> s);
    return ucal_u64_i64(u);
}
#endif /*MACHINE_ASR*/

// -------------------------------------------------------------------------------------
// We have some tuple-like types, mainly to represent the result of split (division)
// operations.

/// @brief result of splitting a @c time_t value by an @c uint32_t divider
typedef struct {
    time_t   q; ///< quotient (integer part)
    uint32_t r; ///< remainder (fractional part)
} ucal_TimeDivT;

/// @brief result of splitting a @c uint32_t value by an @c uint32_t divider
typedef struct {
    uint32_t q; ///< quotient (integer part)
    uint32_t r; ///< remainder (fractional part)
} ucal_u32DivT;

/// @brief result of splitting a @c int32_t value by an @c uint32_t divider
typedef struct {
    int32_t  q; ///< quotient (integer part)
    uint32_t r; ///< remainder (fractional part)
} ucal_iu32DivT;

/// @brief result of splitting a @c int64_t value by an @c uint32_t divider
typedef struct {
    int64_t  q; ///< quotient (integer part)
    uint32_t r; ///< remainder (fractional part)
} ucal_i64u32DivT;

/// @brief divide (as divmod) @c n by @c d, using floor convention
/// @param n    numerator (dividend)
/// @param d    denominator (divider)
/// @return     tuple with quotient @c .q and modulus (remainder) @c .r
static inline ucal_iu32DivT ucal_iu32Div(int32_t n, uint32_t d) {
    uint32_t m = -(n < 0);
    uint32_t q = m ^ ((m ^ (uint32_t)n) / d);
    return (ucal_iu32DivT){ .q = ucal_u32_i32(q), .r = (uint32_t)n - q * d };
}

/// @brief calculate (a - b) / d under floor division rules
/// @param a    dividend term 1
/// @param b    dividend term 2
/// @param d    divider
/// @return     tuple with quotient @c .q and modulus (remainder) @c .r
static inline ucal_iu32DivT ucal_iu32SubDiv(int32_t a, int32_t b, uint32_t d) {
    uint32_t m = -(a < b);
    uint32_t n = (uint32_t)a - (uint32_t)b;
    uint32_t q = m ^ ((m ^ n) / d);
    return (ucal_iu32DivT){ .q = ucal_u32_i32(q), .r = n - q * d };
}

// -------------------------------------------------------------------------------------
// some excercises (mod 7)

/// @brief mathematical/floor (mod 7) op
/// @param x operand
/// @return x (mod 7)
static inline int32_t ucal_i32Mod7(int32_t x) {
    uint32_t xred = (UINT32_C(7) << 17)
                  + (x & UINT32_C(0x7FFF)) + ucal_i32Asr(x, 15);
    return (int32_t)(xred % 7);
}

/// @brief mathematical/floor (mod 7) sum
/// @param a operand 1
/// @param b operand 1
/// @return (a + b) (mod 7)
static inline int32_t ucal_i32AddMod7(int32_t a, int32_t b) {
    uint32_t xred = (UINT32_C(7) << 17)
                  + (a & UINT32_C(0x7FFF)) + ucal_i32Asr(a, 15)
                  + (b & UINT32_C(0x7FFF)) + ucal_i32Asr(b, 15);
    return (int32_t)(xred % 7);
}

/// @brief mathematical/floor (mod 7) difference
/// @param a operand 1
/// @param b operand 1
/// @return (a - b) (mod 7)
static inline int32_t ucal_i32SubMod7(int32_t a, int32_t b) {
    uint32_t xred = (UINT32_C(7) << 17)
                  + (a & UINT32_C(0x7FFF)) + ucal_i32Asr(a, 15)
                  - (b & UINT32_C(0x7FFF)) - ucal_i32Asr(b, 15);
    return (int32_t)(xred % 7);
}

// -------------------------------------------------------------------------------------
// day-of-week shifts

/// @brief get next matching weekday strictly after the base date @note Sets @c errno to @c ERANGE
/// if the operation would overflow
/// @param rdn  base day
/// @param wd   target day of week (can be off-scale)
/// @return     target rdn or @c INT32_MAX on overflow
extern int32_t ucal_WdGT(int32_t rdn, int wd);

/// @brief get next matching weekday on or after the base date @note Sets @c errno to @c ERANGE if
/// the operation would overflow
/// @param rdn  base day
/// @param wd   target day of week (can be off-scale)
/// @return     target rdn or @c INT32_MAX on overflow
extern int32_t ucal_WdGE(int32_t rdn, int wd);

/// @brief get next matching weekday on or before the base date @note Sets @c errno to @c ERANGE
/// if the operation would overflow
/// @param rdn  base day
/// @param wd   target day of week (can be off-scale)
/// @return     target rdn or @c INT32_MIN on overflow
extern int32_t ucal_WdLE(int32_t rdn, int wd);

/// @brief get next matching weekday strictly before the base date @note Sets @c errno to @c
/// ERANGE if the operation would overflow
/// @param rdn  base day
/// @param wd   target day of week (can be off-scale)
/// @return     target rdn or @c INT32_MIN on overflow
extern int32_t ucal_WdLT(int32_t rdn, int wd);

/// @brief get closest matching weekday around the base date @note Sets @c errno to @c ERANGE if
/// the operation would overflow
/// @param rdn  base day
/// @param wd   target day of week (can be off-scale)
/// @return     target rdn or @c INT32_MIN / @c INT32_MAX on overflow
/// @note Since this can move in either direction, it can hit both boundaries!
extern int32_t ucal_WdNear(int32_t rdn, int wd);

// -------------------------------------------------------------------------------------

/// @brief date/time in CE calendare (Gregorian or Julian)
typedef struct {
    int16_t dYear;          ///< calendar year
    int16_t dYDay : 10;     ///< day in year, 1..366
    int16_t dWDay : 5;      ///< day of week, 1..7, Monday is 1
    int16_t fLeap : 1;      ///< year is a leap year
    int8_t dMonth;          ///< calendar month, 1..12. January is 1
    int8_t dMDay;           ///< day of month, 1..31
} ucal_CivilDateT;

/// @brief a ISO8601 week calendar datum
typedef struct {
    int16_t dYear;  ///< calendar year
    int8_t dWeek;   ///< calendar week, [1..53]
    int8_t dWDay;   ///< day of week, [1..7], 1==Monday
} ucal_WeekDateT;

/// @brief civil 24-h time
typedef struct {
    int8_t tHour;     ///< hour in 24-h clock
    int8_t tMin;      ///< minute in hour
    int8_t tSec;      ///< second in minute
} ucal_CivilTimeT;

// -------------------------------------------------------------------------------------
/// @brief Granlund-Möller division step
///
/// This is a single division core step a la Granlund/Möller.  As with other extended-precision
/// dicision algorithms, it imposes some restrictions on the diveder, namely that the LSB is
/// actually set. (Please note that this can be easily achieved by shifting/scaling of the
/// original problem!)
/// @param u1   high part of dividend, \f$ 0 \leq u1 < d \f$
/// @param u0   low part of dividend
/// @param d    normalised divider, \f$ 2^{31} \leq d < 2^{32} \f$
/// @param v    approximated fixpoint inverse, \f$ v \gets \lfloor \frac{2^{64} - 1}{d}\rfloor - 2^{32} @f$
/// @return     tuple with quotient and remainder
extern ucal_u32DivT ucal_u32DivGM(uint32_t u1, uint32_t u0, uint32_t d, uint32_t v);

// -------------------------------------------------------------------------------------
/// @brief chained Granlund-Möller division to divide @c int64_t by @c uint32_t
///
/// This function takes care of the gritty details when using Granlund/Möller style division on a
/// signed 64bit didend with an unsigned 32bit divider.  The divider and inverse must be adjusted
/// accordingly the core step, and the function needs to know which normlisation shift must be
/// applied to prepare the condition input and output of the core step.
/// @see ucal_u32DivGM
/// @param u    dividend (numerator)
/// @param d    normalised divider (denominator)
/// @param v    approximated inverse of d
/// @param s    pre/post shift value
/// @return     tuple with quotient and remainder
extern ucal_i64u32DivT ucal_i64u32DivGM(int64_t u, uint32_t d, uint32_t v, unsigned s);

// -------------------------------------------------------------------------------------
/// @brief split seconds into full days and seconds since midnight
///
/// Essentialy a floor division by 86400, the number of seconds per day.  This is not as nice as
/// it sounds, due to the somwhat fuzzy representation of @c time_t values.
///
/// @note This function does not assume an epoch or an era. If given the result of
///       @c time() or similar, it will yield days and seconds in the UNIX epoch.
///
/// @param  tt  @c time_t seconds to split
/// @returns    tuple with days as quotient and seconds in day as remainder 
extern ucal_TimeDivT ucal_TimeToDays(time_t tt);

// -------------------------------------------------------------------------------------
/// @brief split time stamp into full RDN days and seconds since midnight
///
/// This converts a @c time_t value as absolut time in the UNIX epoch into the RataDie number of
/// the Gregorian civil date and the seconds since midnight.
///
/// @note This function @e does assume the qc tt value is an absolute time stamp in
//        the UNIX epoch and applies the proper shifts.
///
/// @param  tt  @c time_t time stamp value to split
/// @returns    tuple with the RataDie Number as quotient and seconds in day as remainder
extern ucal_TimeDivT ucal_TimeToRdn(time_t tt);

// -------------------------------------------------------------------------------------
/// @brief split elapsed days in year to elapsed months and elapsed days in month
///
/// This uses the UNSHIFTED calender and therefore needs the leap year indicator.
/// @param ed   elapsed days
/// @param isLY year is a leap year
/// @return 
extern ucal_iu32DivT ucal_DaysToMonth(uint_fast16_t ed, bool isLY);

// -------------------------------------------------------------------------------------
/// @brief convert calendar month to accummulated days
///
/// Shifts the calendar by adding ten months, normalises the result and returns the years
/// resulting from the normalisation (can be negative!) and the accummulated days in the current
/// year.
/// 
/// @note This uses the SHIFTED calendar starting with March!
/// @param m    calendar month (can be off-scale)
/// @return 
extern ucal_iu32DivT ucal_MonthsToDays(int16_t m);

// -------------------------------------------------------------------------------------
/// @brief get the build date (from unspecified zone!) as RDN
/// @return RDN of build date 
int32_t ucal_BuildDateRdn(void);

// -------------------------------------------------------------------------------------
/// @brief split a time-of day (plus offset) into hh/mm/ss
///
/// Essentially adds the offset to the time-of-day, and then divides by 60 / 60 / 24.  The
/// execssive days (@c ofs might shift over day boundaries, and @c dt might be off-range)
/// are returned as function result.
///
/// @param into where to store the broken-down time
/// @param dt   time-of-day to split
/// @param ofs  offset to apply before splitting
/// @return     excessive days
int32_t ucal_DayTimeSplit(ucal_CivilTimeT *into, int32_t dt, int32_t ofs);

// -------------------------------------------------------------------------------------
/// @brief convert day time to seconds (simple Horner schema)
/// @param h    hours   (can be off-scale)
/// @param m    minutes (can be off-scale)
/// @param s    seconds (can be off-scale)
/// @return     accumulated secons
int32_t ucal_DayTimeMerge(int16_t h, int16_t m, int16_t s);

CDECL_END
#endif /*COMMON_H_D2078C60_0B6B_439F_B110_087913F54042*/
