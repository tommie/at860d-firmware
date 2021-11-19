#!/usr/bin/python3
"""
Test program to show that precomputing a ratio reciprocal using
limited fixed-point gives good ZCC patterns.

The table itself can be generated with MPASM macros. It should
be the same as Precomputed.pc.
"""

import math

def true_pattern(num_frac_bits, i):
    if not i: return set()

    N = 2**num_frac_bits - 1
    rr = N / i
    return set(round(rr * j) for j in range(i))

class Precomputed(object):
    def __init__(self, num_frac_bits, num_extra_bits):
        N = 2**num_frac_bits - 1
        self.ne = 2**num_extra_bits
        self.pc = [2**(num_frac_bits + num_extra_bits) - 1] + [round(self.ne * N / i) for i in range(1, N + 1)]

    def pattern(self, i):
        NE = self.ne
        rr = self.pc[i]
        out = set()
        v = 0
        for j in range(i):
            out.add(math.floor((v + NE / 2) / NE))
            v += rr
        return out

def pattern_string(n, p):
    # For all but zero, the first cycle is always on. It's prettier to
    # show it in the middle.
    return ''.join('##' if (i + n//2) % n in p else '  ' for i in range(n))

def diff_string(n, a, b):
    return ''.join(DIFF_TABLE[((i + n//2) % n in a, (i + n//2) % n in b)] for i in range(n))

DIFF_TABLE = {
    (False, False): ' ',
    (False, True): '+',
    (True, False): '-',
    (True, True): ' ',
}

def main():
    num_frac_bits = 5
    num_extra_bits = 3

    n = 2**num_frac_bits - 1

    print('True patterns (unbounded precision)')
    print()
    for i in range(n + 1):
        p = true_pattern(num_frac_bits, i)
        if len(p) != i:
            raise ValueError('invalid pattern: length {} vs {}'.format(len(p), i))
        print('{:2d} '.format(i), pattern_string(n, p))

    print()
    print('Approximate patterns (fixed-point arithmetic)')
    print()
    pc = Precomputed(num_frac_bits, num_extra_bits)
    for i in range(n + 1):
        p = pc.pattern(i)
        if len(p) != i:
            raise ValueError('invalid pattern: length {} vs {}'.format(len(p), i))
        print('{:2d} '.format(i), pattern_string(n, p))

    print()
    print('Pattern diff')
    print()
    for i in range(n + 1):
        tp = true_pattern(num_frac_bits, i)
        ap = pc.pattern(i)
        print('{:2d} '.format(i), diff_string(n, tp, ap))

    print()
    print('Precomputation table:', ', '.join('{:02X}'.format(v) for v in pc.pc))

main()
