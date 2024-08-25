
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
# helper to create header file with constant expressions
# ----------------------------------------------------------------------------------------------

import re
import sys

def date2rdn(y,m,d):
    "pythonic implementation of a date --> RDN conversion"
    z, m = divmod(m-3, 12)
    y += z
    return (d + (y * 365) +
            (y // 4) - (y // 100) + (y // 400)
            + ((m * 153 + 2) // 5) - 306)

assert date2rdn(2001, 1, 1) == 5*146097 + 1
assert date2rdn(2000,12,31) == 5*146097


def evaluator(mo):
    " evaluation helper"
    expr = mo.groups(1)[0]
    return str(eval(expr, globals()))

pattern = re.compile(r"\<\[(.*)\]\>")

def process(line):
    "process a single line"
    return pattern.sub(evaluator, line)

def iofs(name, mode):
    "open-file wrapper"
    return open(name, mode, encoding="utf-8")

if __name__ == "__main__":
    if len(sys.argv) == 3:
        with iofs(sys.argv[1], "r") as ifp, iofs(sys.argv[2], "w") as ofp:
            for line in ifp:
                ofp.write(process(line))
# -*- that's all folks -*-
