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
// Written anno 2025 by J.Perlinger (perlinger@nwtime.org)
//
// POSIX time zone string handling
// ----------------------------------------------------------------------------------------------
#ifndef TZPOSIX_H_D2078C60_0B6B_439F_B110_087913F54042
#define TZPOSIX_H_D2078C60_0B6B_439F_B110_087913F54042

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

CDECL_BEG

/// @brief POSIX TZ style transition rule, encoded
///
/// We encode type-I and type-II rules either als regular month/day tuple or as extended
/// January.  In these cases the day-of-week is zero, and 'rt_mdmw' is the day of the month.
/// (We need enough bits to cover a full-year range here!)  For the type-III 'M' rules, the
/// 'rt_mdmw' field holds the week-of month, with the value 5 designating 'the last'
/// occurrence of the given day-of-week (1->Monday, 7->Sunday), which can be the 4th or
/// 5th, really.
typedef struct {
    unsigned int rt_month : 4;  ///< 1..12    ; always 1 for type zero rules
    unsigned int rt_mdmw  : 9;  ///< 1..365   ; flexible operand, depends on rule type
    unsigned int rt_wday  : 3;  ///< 0 or 1..7; 0 for type  1/2 rules, otherwise day-of-week
    signed   int rt_ttloc : 16; ///< transition time in minutes since midnight, wallclock
} tziPosixRuleT;

/// @brief POSIX time zone description
///
/// Describes a time zone as defined by a POSIX time zone string with a single rule set
/// or a single static zone.
typedef struct tziPosixZone_S {
    //-*-  zone names first
    char            stdName[12];///< standard zone name/abbreviation
    char            dstName[12];///< name/abbreviation of associated DST zone
    // -*- zone offsets next
    int16_t         stdOffs;    ///< offset (STD - UTC) in minutes; negative if east of Greenwich!
    int16_t         dstOffs;    ///< offset (DST - UTC) in minutes
    // -*- transition rules last
    tziPosixRuleT   stdRule;    ///< When DST ends -- typically autumn, unless you're Irish
    tziPosixRuleT   dstRule;    ///< When DST becomes effective -- typically spring
} tziPosixZoneT;

/// @brief time zone conversion context
///
/// The conversion context caches the transition data for one year (with a slack
/// of roughly one day at both ends).  This saves a lot of calendar calculations,
/// fast as they might b, for the typical use case without much extra overhead when
/// the time stamps jump around wildly.
///
/// @note The time stamps are only calculated if the associated time zone has
/// transitions indeed.  For fixed / single zones they will never be looked at.
///
/// All time stamps are seconds since the UNIX epoch, that is 1970-01-01-T00:00:00Z.
typedef struct tziConvCtx_S {
    int64_t                 trLoBound;  ///< lo bound of calculated frame
    int64_t                 trHiBound;  ///< hi bound of calculated frame
    int64_t                 ttDST;      ///< transition STD --> DST
    int64_t                 ttSTD;      ///< transition DST --> STD
    tziPosixZoneT const*    pTZI;       ///< associated POSIX rule description
} tziConvCtxT;

typedef struct tziConvInfo_S {
    unsigned int isDst : 1; ///< time is in DST range
    unsigned int isHrA : 1; ///< time is in overlap before transition
    unsigned int isHrB : 1; ///< time is in overlap after transition
    signed   int offs  :29; ///< offset to add (sign depends on direction from/to UTC!)
} tziConvInfoT;

/// @brief conversion hints
///
/// Hints how to get the conversion info. The conversion of UTC to local time cannot fail
/// (apart from overflow issues), but a time stamp in local time can be ambiguous with the
/// spring gap and autumn overlap. So apart from the 'default' modes, we have some hints
/// we can give.
typedef enum tziCvtHints_E {
    tziCvtHint_None,    ///< system (UTC) to local time
    tziCvtHint_STD,     ///< local time to UTC, resolve to STD time
    tziCvtHint_DST,     ///< local time to UTC, resolve to DST
    tziCvtHint_HrA,     ///< local time to UTC, resolve to zone before transition
    tziCvtHint_HrB,     ///< local time to UTC, resolve to zone after transition
} tziCvtHintT;

/// @brief parse POSIX-conforming zone spec
///
/// Given a POSIX time zone string (with GNU extensions), parse this into the binary
/// representation used for conversions.
///
/// @note The function may succeed even without consuming the whole input string: Some
/// of the components are optional, so parsing might simply stop at a position where
/// an optional component might start but cannot be parsed.  Just checking the result
/// against @c NULL is insufficient to test if all input is consumed or not.
///
/// @param into     time zone info to fill
/// @param head     head position of string to parse
/// @param tail     tail position; can be @c NULL to stop at @c NUL byte
/// @return         parse end position on success, @c NULL on error
extern const char* tziFromPosixSpec(tziPosixZoneT *into, const char *head, const char *tail);

/// @brief get information for a conversion from local time to UTC time scale
///
/// Get the zone info to convert a local time to UTC time.  This function will fail
/// if the time given is in the spring/autumn discontinuity range and no proper hint
/// is provided.  While most people consider only the back-step a problem, the other
/// direction has a symmetric issue: A local time stamp in the critical/omitted
/// transition period can also be before or after the transition time in UTC.
/// Sane data shouldn't contain time stamps in the omitted range, but reality bites
/// in unexpected places.
///
/// @note Giving a fixed hint (like always using STD time) will prevent the function
///       from failing.  It should be noted that the result is possibly not quite what
///       you expect for the critical values when you do so.  OTOH, it just might be;
///       just remember that your mileage may vary, especially around the transition
///       where the clock steps backward.
///
/// @param into     where to store the results
/// @param ctx      conversion context to use/update
/// @param tsfrom   time stamp in UNIX scale
/// @param hint     how to resolve ambiguities
/// @return         @c true on success, @c false otherwise
extern bool tziGetInfoLocal2Utc(tziConvInfoT *into, tziConvCtxT *ctx, int64_t tsfrom, tziCvtHintT hint);

/// @brief get information for a conversion from local time to UTC time scale
///
/// This uses a different strategy: It gives the zone that yields the time closest to the pivot
/// still smaller than the pivot. (Again, the tie break is only used where a time stamp @e is
/// ambiguous!) This strategy works well with producer and consumer of time stamps at least
/// roughly in sync and update delays smaller than 0.5h.
///
/// @note Communication / update failures during the critical intervals can flip results to
///       to the wrong side when the wallclock steps backward.
/// @note This function does @e not set the HourA/B indicators.
///
/// @param into     where to store the results
/// @param ctx      conversion context to use/update
/// @param tsfrom   time stamp in UNIX scale
/// @param pivot    pivot time for disambiguation
/// @return         @c true on success, @c false otherwise
extern bool tziGetInfoLocal2Utc_alt(tziConvInfoT *into, tziConvCtxT *ctx, int64_t tsfrom, int64_t pivot);

/// @brief get information for a conversion from UTC time to local time scale
/// Get the zone info to convert a UTC time stamp to local time.  While formally returning
/// a boolean value, for now this function cannot fail unless NULL pointers are passed into
/// the arguments...
///
/// @param into     where to store the results
/// @param ctx      conversion context to use/update
/// @param tsfrom   time stamp in UNIX scale
/// @return         @c true on success, @c false otherwise
extern bool tziGetInfoUtc2Local(tziConvInfoT *into, tziConvCtxT *ctx, int64_t tsfrom);

/// @brief align a period around a time stamp to local time
///
/// Get an aligned period of give length in @e local time that contains a given
/// time stamp. This is a typical problem for e.g Power Quality measurement
/// equipment, where aggregation periods are 10 or 30 minutes, 1, 2, 6 or 12
/// hours or a day. The periods longer than one hour are likely sensitive to DST
/// transitions.  For some applications (notably those generating PQDIFF data)
/// all samples in a set must be from the same local time zone;
/// a sample interval has to start or end at the respective transition, if such
/// falls into the sample range.
///
/// @note Whether DST/STD transitions hurt or not depends on few GCD properties of the DST/STD
///    difference, the phase shift and the period. For a @e sane time zone, with 1h difference
///    and switching on a full hour, 1h is the threshold where madness starts to take its toll.
///
/// @note The phase shift 'phi' is zero for most applications.  It may be needed to align cycles
/// that are multiples of days or cycles that do not cleanly divide a day.
///
/// @param tlohi    lo/hi range boundaries
/// @param cvInfo   conversion info at query point
/// @param ctx      conversion context to evaluate/update
/// @param tsfrom   pivot time; tlohi[0] <= tsfrom < tlohi[1] on success
/// @param period   time period to align
/// @param phi      phase shift (probably zero, see note)
/// @return
extern bool tziAlignedLocalRange(int64_t tlohi[2], tziConvInfoT *cvInfo, tziConvCtxT *ctx,
                                 int64_t const tsfrom, int32_t period, int32_t phi);

CDECL_END
#endif /*TZPOSIX_H_D2078C60_0B6B_439F_B110_087913F54042*/
