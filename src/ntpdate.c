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
// This module contains some minimal NTP time scale support
// ----------------------------------------------------------------------------------------------

#include "ucal/common.h"
#include "ucal/calconst.h"
#include "ucal/ntpdate.h"

/// @file
/// NTP time scale mappings

time_t
ucal_NtpToTime(
    uint32_t      secs,
    const time_t* pivot)
{
    if (sizeof(time_t) > sizeof(uint32_t)) {
        // we have some real work to do... start with getting the expansion base.
        time_t tbase = pivot ? *pivot : time(NULL);
        if (tbase > INT32_MAX) {
            tbase -= 0x80000000ul;
        } else {
            tbase = 0;
        }
        // now do a periodic expansiom (mod 2³²). Which is dead-pan easy :)
        secs += UCAL_sysPhiNTP;     // align NTP scale to Unix scale (mod 2³² implicitt!)
        secs -= tbase;              // get cycle difference          (mod 2³² implicitt!)
        return tbase + secs;        // add difference to base, and that's it!
    } else {
        // Hmpf. This system will have trouble soon... but here we go anyway:
        return ucal_u32_i32(secs += UCAL_sysPhiNTP);
    }
}

// -*- that's all folks -*-
