#!/usr/bin/python3
# ----------------------------------------------------------------------------------------------
# edivconst.py by J.Perlinger (perlinger@nwtime.org)
#
# To the extent possible under law, the person who associated CC0 with
# µCal has waived all copyright and related or neighboring rights
# to µCal.
#
# You should have received a copy of the CC0 legalcode along with this
# work.  If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
# ----------------------------------------------------------------------------------------------
# calculate coefficients for division-by-constant for
#  - fast single-width division a la Granlund/Montgomery
#  - optimised double-width division a la Granlund/Moeller
# ----------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------
# Implement some of the bit-level stuff. Python's integer class provides most of the tools to
# do log2ceil/log2floor without the need to resort to manual bit operations in loops...

def log2floor(x: int) -> int:
    "floor(log2(abs(x)))"
    return abs(x).bit_length() - 1 if x else 0

assert 0 == log2floor(1)
assert 1 == log2floor(2)
assert 1 == log2floor(3)
assert 2 == log2floor(4)
assert 2 == log2floor(7)
assert 3 == log2floor(8)

def log2ceil(x: int) -> int:
    "ceil(log2(abs(x)))"
    return (abs(x) - 1).bit_length() if x else 0

assert 0 == log2ceil(1)
assert 1 == log2ceil(2)
assert 2 == log2ceil(3)
assert 2 == log2ceil(4)
assert 3 == log2ceil(5)
assert 3 == log2ceil(8)

def fdiv_choose_mult(d: int, bits: int, prec: int, short: bool = True) -> tuple[int, int, int]:
    "coefficients for Granlund-Montgomery style division"

    assert d > 1   , "'d' must be > 1"
    assert 1 < bits        , "'bits' out of range"
    assert 1 < prec <= bits, "'prec' out of range"

    l = log2ceil(d)
    assert 1 <= l <= bits, "'d' doesn't fir into 'bits'"

    shp = l
    m_l = (1 << (bits + l))
    m_h = m_l + (1 << (bits + l - prec))
    m_l //= d
    m_h //= d
    assert m_h > m_l, "internal: convergent range empty"

    if short:
        # For best performance when generating assembler code the multiplier can be reduced
        # in many (not all!) cases.  G.-M.'s paper describes it by a looped test-and-shift
        # sequence, but the equivalent can be reached by getting the bitlength of the symmetric
        # bit difference (aka XOR) of m_h and m_l.

        ## looped reduction as described in G.-M.'s paper:
        # while shp > 0 and m_l >> 1 != m_h >> 1:
        #     shp -= 1
        #     m_l >>= 1
        #     m_h >>= 1

        ## constant-time variation on the same topic:
        red = (m_h ^ m_l).bit_length() - 1
        red = max(0, min(l, red))
        m_h >>= red
        m_l >>= red
        shp  -= red

    m_x  = m_h >> bits
    m_h &= (1 << bits) - 1
    
    return m_x, m_h, shp, l


print(fdiv_choose_mult(1461, 32, 18))
print(fdiv_choose_mult(1461, 32, 31))
print([hex(x) for x in fdiv_choose_mult(7, 64, 64)])

print(fdiv_choose_mult(146097, 33, 33))
