#!/usr/bin/python3
# ----------------------------------------------------------------------------------------------
# µCal by J.Perlinger (perlinger@nwtime.org)
#
# To the extent possible under law, the person who associated CC0 with
# µCal has waived all copyright and related or neighboring rights
# to µCal.
#
# You should have received a copy of the CC0 legalcode along with this
# work.  If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
# ----------------------------------------------------------------------------------------------
# µCal -- a small calendar component in C99
# Written anno 2024 by J.Perlinger (perlinger@nwtime.org)
#
# calendar analytics: Get linear interpolations for months<-->days (shifted & unshifted)
# and for the ISO8601 week calendar (years<-->weeks)
# ----------------------------------------------------------------------------------------------

import itertools
import operator
import math

from typing  import Tuple, Iterable

# ----------------------------------------------------------------------------------------------
# print a title with some separator lines
def print_header(title :str) -> None:
    "print heder/separator"
    print("\n---------------------------------------------------------------------")
    print(title)
    print("---------------------------------------------------------------------")

# ----------------------------------------------------------------------------------------------
# Find the integer number with the most trailing zeros in a given range
def max_even(lo :int, hi :int) -> int:
    "get z, lo <= z < hi with most trailing zeros"
    z = lo + (-lo & lo)         # find least-position ONE-bit and add to lower bound
    while z < hi:               # not yet exceeded the limit?
        lo = z                  # take as new lower bound...
        z = lo + (-lo & lo)     # ... and flipnext bit!
    return lo

# ----------------------------------------------------------------------------------------------
# find the extrema (min & max) of a sequence of values
def minmax(seq):
    "get tuple with extrema of sequence"
    vmin = None
    vmax = None
    for item in seq:
        if vmin is None or item < vmin:
            vmin = item
        if vmax is None or item > vmax:
            vmax = item
    return (vmin, vmax)

# ----------------------------------------------------------------------------------------------
# Calculate RDN of 1st day of year in Gregorian calendar.
def yearstart_rdn(y: int) -> int:
    "first day of a year in the Gregorian calendar"
    y -= 1
    return y * 365 + y // 4 - y // 100 + y // 400 + 1

# ----------------------------------------------------------------------------------------------
# Calculate the ordinal continuous week number since Christian epoch for a year in ISO8601
# Week calendar. Simply done by calculating the RDN of the 1st day of the Gregorian year with
# the same number, then find the closest Monday, and divide by 7.
def isoweek_yearstart(y: int) -> int:
    "first week in ISO8601 week year since epoch"
    rdn = yearstart_rdn(y) - 3
    rdn += (1 - rdn) % 7
    w, d = divmod(rdn - 1, 7)
    assert d == 0               # paranoia prevails...
    return w + 1                # We want the ordinal number!

# ----------------------------------------------------------------------------------------------
# month length table, for an unshifted year, but with a February of 30 days
monlen_feb30 = (31,30,31,30,31,30,31,31,30,31,30,31)
# ----------------------------------------------------------------------------------------------
# month length table, for year start at March, also with a February of 30 days
monlen_shift = monlen_feb30[2:] + monlen_feb30[:2]

# ----------------------------------------------------------------------------------------------
# enumerates AND accumulates the value of a table
def enumerated(seq: Iterable[int]) -> Iterable[Tuple[int,int]]:
    "enumerate accumulated sequence"
    return enumerate(itertools.accumulate(seq, operator.add, initial=0))

# ----------------------------------------------------------------------------------------------
# transpose a sequence of pairs by swapping the x- and y-values.  If the new X step is greater
# than one, intercalate the proper bounding values, too.
def transposed(seq: Iterable[Tuple[int,int]]) -> Iterable[Tuple[int,int]]:
    "transpose & intercalate enumerated sequence"
    lastx = 0
    lasty = 0
    for y,x in seq:
        if x - 1 > lastx:       # step width > 1?
            yield (x-1, lasty)  #  yup, intercalate boundary value
        lastx = x               # remember values for next pass
        lasty = y
        yield (x, y)            # yield the current transposed tuple

# ----------------------------------------------------------------------------------------------
# enumerate & transpose in one sweep
def enum_transposed(seq: Iterable[int]) -> Iterable[Tuple[int,int]]:
    "enumerate, transpose & intercalate enumerated sequence"
    return transposed(enumerated(seq))

# ----------------------------------------------------------------------------------------------
# slope calculations for the min/max slope, either against between two point pairs or a point
# pair and zero (the origin)

def min_slope_p(t: Tuple[Tuple[int, int],Tuple[int, int]]) -> float:
    "lower slope of pair of points"
    t1,t2 = t
    dx = t2[0] - t1[0]
    dy = t2[1] - t1[1] - 1
    return dy / dx

def max_slope_p(t: Tuple[Tuple[int, int],Tuple[int, int]]) -> float:
    "upper slope of pair of points"
    t1,t2 = t
    dx = t2[0] - t1[0]
    dy = t2[1] - t1[1] + 1
    return dy / dx

def min_slope_0(t2: Tuple[int, int]) -> float:
    "lower slope of point against zero"
    dx = t2[0]
    dy = t2[1] - 1
    return dy / dx

def max_slope_0(t2: Tuple[int, int]) -> float:
    "upper slope of point against zero"
    dx = t2[0]
    dy = t2[1] + 1
    return dy / dx

# ----------------------------------------------------------------------------------------------
# Calculating the abscissa intercepts is very sensitive to rounding errors when done in float.
# But since we get the slope and the point both as rational pair, and we need the scaled(!)
# intercept anyways, we can do all the calculations in integer quantities and forget all about
# rounding errors!

def intercept_y(pt: Tuple[int, int], slope: Tuple[int, int]) -> int:
    "calculate lower intercept"
    dx, dy = pt
    sy, sx = slope
    return dy * sx - sy * dx

# ----------------------------------------------------------------------------------------------
# Provide a sequence of rational pairs where the denominator is a power of two.  Will yield odd
# numerators, exclusuvely: Since the denominator is a pwer of two, common factors of two exist
# in even numerator and therefore have already been deliverd...
def power_iter(lo:float, hi:float) -> Iterable[Tuple[int,int]]:
    "create a series of approximations with power-of-2 denominator"
    series = (1 << p for p in range(64))
    for mult in series:
        nlo = int(math.ceil(lo * mult))
        nhi = int(math.ceil(hi * mult))
        for n in range(nlo, nhi):
            if n & 1:
                yield (n, mult)

# ----------------------------------------------------------------------------------------------
# linear probing to find rational pairs in [lo,hi)
# Checks the GCD to see if a pair is reducible; if so, the reduced pair has already been seen
# and shouldn't be delivered again.
def lin_iter(lo:float, hi:float, limit:int) -> Iterable[Tuple[int,int]]:
    "linear fraction search -- brute force"
    for mult in range(1,limit):
        nlo = int(math.ceil(lo * mult))
        nhi = int(math.ceil(hi * mult))
        for n in range(nlo, nhi):
            if math.gcd(n, mult) == 1:
                yield (n, mult)

# ----------------------------------------------------------------------------------------------
# now to the serious stuff:
#  - months to days and reverse, shifted and unshifted calendar
# ----------------------------------------------------------------------------------------------

def m2d(seq, maxden, deep_slope=True):
    "calculate linear interpolation for month to days"

    # make sure 'seq' can be used several times -- a simple generator cannot restart.
    pairs = list(seq)

    # get the slope boundaries is not too difficult, either:
    if deep_slope:
        sl_min = max(map(min_slope_p, itertools.combinations(pairs, 2)))
        sl_max = min(map(max_slope_p, itertools.combinations(pairs, 2)))
    else:
        sl_min = max(map(min_slope_0, pairs[1:]))
        sl_max = min(map(max_slope_0, pairs[1:]))

    assert sl_min < sl_max, "cannot find non-empty slope range!"

    print("   --> slope range:", sl_min, "--", sl_max)
    assert sl_min < sl_max, "cannot find non-empty slope range!"

    for loops in range(2):
        # start with linear probing fro fraction range, then try again with powers of two
        if loops:
            slopes = power_iter(sl_min, sl_max)
        else:
            slopes = lin_iter(sl_min, sl_max, maxden)

        # now for getting the Y-abscissas where the slopes cross at x==0...
        for slv in slopes:
            yi_lo, yi_hi = minmax(map(lambda p,s=slv: intercept_y(p,s), pairs[1:]))
            yi_lo, yi_hi = yi_hi, yi_lo + slv[1]
            if yi_lo < yi_hi:
                print(f"n={slv[0]} d={slv[1]}  c={yi_lo}..{yi_hi-1} c0={max_even(yi_lo,yi_hi)}")
                if not slv[1] & (slv[1] - 1):
                    return
                break

print_header("months to days, unshifted calendar, February with 30 days")
m2d(enumerated(monlen_feb30), 257)

print_header("days to months, unshifted calendar, February with 30 days")
m2d(enum_transposed(monlen_feb30), 2**12)

print_header("months to days, shifted calendar")
m2d(enumerated(monlen_shift), 257)

print_header("days to months, shifted calendar")
m2d(enum_transposed(monlen_shift), 2**12)

# ----------------------------------------------------------------------------------------------
# another unwiely beast: the ISO8601 week calendar
# - length of centuries in a quadricentennial
# ----------------------------------------------------------------------------------------------

print_header("length of centuries in ISO8601 week calendar")

cwlen = [isoweek_yearstart(c*100 + 1) for c in range(5)]
for c in range(4):
    print(f"length of {c}. century = {cwlen[c+1] - cwlen[c]}")

# ----------------------------------------------------------------------------------------------
# what comes next has some similiarity with the month/days conversion evaluation...
# but we filter the slope for all 4 centuries into one range.

def wdn(maxden, deep_slope = True):
    "find interpolation coeffs for years-to-weeks"
    sl_min = 0.0
    sl_max = math.inf
    # get the slope boundaries is not too difficult, but we have to aggregate over the ccenturies:
    for pairs in ytable:
        if deep_slope:
            v1 = max(map(min_slope_p, itertools.combinations(pairs, 2)))
            v2 = min(map(max_slope_p, itertools.combinations(pairs, 2)))
        else:
            v1 = max(map(min_slope_0, pairs[1:]))
            v2 = min(map(max_slope_0, pairs[1:]))
        sl_min = max(sl_min, v1)
        sl_max = min(sl_max, v2)

    print("   --> slope range:", sl_min, "--", sl_max)
    assert sl_min < sl_max, "cannot find non-empty slope range!"

    for loops in range(3):
        # start with linear probing, the do power-of-two probing for slopes
        if loops:
            slopes = power_iter(sl_min, sl_max)
        else:
            slopes = lin_iter(sl_min, sl_max, maxden)

        # now for getting the (scaled!) Y-abscissas where the slopes cross at x==0...
        for slv in slopes:
            olist = []
            for pairs in ytable:
                yi_lo, yi_hi = minmax(map(lambda p,s=slv: intercept_y(p,s), pairs[1:]))
                yi_lo, yi_hi = yi_hi, yi_lo + slv[1]
                if yi_lo < yi_hi:
                    olist.append((yi_lo, yi_hi-1, max_even(yi_lo, yi_hi)))
            if len(olist) == 4:
                print(f"n={slv[0]} d={slv[1]}  c={olist}")
                if not slv[1] & (slv[1] - 1):
                    return
                break

# ----------------------------------------------------------------------------------------------
# cardinal (elapsed) years in century to cardinal weeks

ytable = []
for cc in range(4):
    cs = isoweek_yearstart(cc * 100 + 1)
    yt = []
    for yi in range(100):
        ys = isoweek_yearstart(cc * 100 + yi + 1)
        yt.append((yi, (ys - cs)))
    ytable.append(yt)
#ytable = ytable[2:4] + ytable[0:2]

print_header("ISO8601 week calendar, years in century to weeks")
wdn(128)

# ----------------------------------------------------------------------------------------------
# cardinal (elapsed) weeks in century to cardinal years
for ci, seqn in enumerate(ytable):
    ytable[ci] = list(transposed(seqn))

print_header("ISO8601 week calendar, weeks in century to years")
wdn(2048)


xc1=((84, 86, 84), (128, 131, 128), (16, 18, 16), (61, 63, 62))

def xc1_calc(i: int) -> int:
    " arithmetic interpolation "
    i = (2 + i) & 3
    k = (i << 1) - (i >> 1)
    return 18 + k * 22
    #return 17 + ((k * 45) >> 1)

for ci in range(4):
    co = xc1_calc(ci)
    print(f"{xc1[ci][0]} <= {co} <= {xc1[ci][1]}")
    assert xc1[ci][0] <= co <= xc1[ci][1]

xc2=((436, 462, 448), (144, 170, 160), (876, 902, 896), (584, 610, 608))

def xc2_calc(i: int) -> int:
    " arithmetic interpolation "
    i = (1 - i) & 3
    k = (i << 1) - (i >> 1)
    #return 160 + k * 145
    return 157 + k * 146

for ci in range(4):
    co = xc2_calc(ci)
    #print(f"{xc2[ci][0]} <= {co} <= {xc2[ci][1]}")
    assert xc2[ci][0] <= co <= xc2[ci][1]

# -*- that's all folks -*-
