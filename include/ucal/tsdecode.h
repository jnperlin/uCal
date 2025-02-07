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
// This module contains the interface for special time stamp format parsing functions.
// ----------------------------------------------------------------------------------------------
#ifndef TSDECODE_H_D2078C60_0B6B_439F_B110_087913F54042
#define TSDECODE_H_D2078C60_0B6B_439F_B110_087913F54042

#include <stdbool.h>
#include <stdint.h>
#include "common.h"

CDECL_BEG

/// @brief decode number string as binary Q0.32 fraction if looking at dot
/// @param pstr     ptr to current parse position
/// @param end      end of parse region
/// @return         seconds and fraction as {.q,.r} -- can be 1.0 after rounding
extern ucal_u32DivT ucal_decFrac(const char** str, const char* end);

/// @brief decode raw number string as binary Q0.32 fraction
/// @param pstr     ptr to current parse position
/// @param end      end of parse region
/// @return         seconds and fraction as {.q,.r} -- can be 1.0 after rounding
extern ucal_u32DivT ucal_decFrac_raw(const char** str, const char* end);


/// @brief decode number string as nano seconds if looking at dot
/// @param pstr     pointer to current parse position
/// @param end      end of parse region
/// @return         nanoseconds, zero if no digits 
extern uint32_t ucal_decNano(const char** pstr, const char* end);

/// @brief decode raw number string as nano seconds
/// @param pstr     pointer to current parse position
/// @param end      end of parse region
/// @return         nanoseconds, zero if no digits 
extern uint32_t ucal_decNano_raw(const char** pstr, const char* end);


/// @brief decode ASN.1 UTCTime (tag 23)
/// @param into     time value storage
/// @param pstr     pointer to current parse position
/// @param end      end of parse region
/// @param ybase    year base for century expansion
/// @return         @c true on success, @c false on error
extern bool ucal_decASN1UtcTime23(struct timespec *into, const char** pstr, const char *end, int ybase);

/// @brief decode ASN.1 GeneralizedTime (tag 24)
/// @param into     time value storage
/// @param pstr     pointer to current parse position
/// @param end      end of parse region
/// @return         @c true on success, @c false on error
extern bool ucal_decASN1GenTime24(struct timespec *into, const char** pstr, const char *end);

CDECL_END
#endif /*TSDECODE_H_D2078C60_0B6B_439F_B110_087913F54042*/
