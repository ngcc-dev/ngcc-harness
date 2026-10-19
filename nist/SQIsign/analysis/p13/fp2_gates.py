#!/usr/bin/env python3
"""Boolean-gate estimate (2-input gates, all counted 1) for one F_{p^2} multiplication
at the SQIsign Round-3 primes p = c*2^f - 1, i^2 = -1.

Integer n x n multiplier:
  schoolbook  S(n) = n^2 AND + (n^2 - 2n) full adders (5 gates) + 2n-bit ripple CPA (5 gates/bit)
  Karatsuba   K(n) = 2 K(ceil(n/2)) + K(ceil(n/2)+1) + 25 n   (pre-adds, middle subtractions, recombination)
  with the base case chosen optimally (min over schoolbook / Karatsuba at every size).
Reduction mod p = c 2^f - 1 (Montgomery-friendly, -p^-1 = 1 mod 2^f):
  T_hi + c*T_lo plus a conditional subtraction: (popcount(c)+3) * 5 n gates.
F_{p^2} multiplication (Karatsuba 3M): 3 integer products + 2 reductions + 5 n-bit add/sub.
Also prints log2(log2(p)^2) (the 'log^2 p bit operations per Fp2 op' convention of SQIsign-4)
and an AND-only Karatsuba floor (n^log2(3) AND gates per product).
"""
import functools, json, math

SETS = {"p324_3": (3, 324), "p500_27": (27, 500), "p664_17": (17, 664)}


def school(n):
    return n * n + 5 * max(0, n * n - 2 * n) + 5 * 2 * n


@functools.lru_cache(maxsize=None)
def kara(n):
    best = school(n)
    if n >= 8:
        h = (n + 1) // 2
        best = min(best, 2 * kara(h) + kara(h + 1) + 25 * n)
    return best


@functools.lru_cache(maxsize=None)
def and_floor(n):
    if n <= 3:
        return n * n
    h = (n + 1) // 2
    return 2 * and_floor(h) + and_floor(h + 1)


def main():
    out = {}
    for name, (c, f) in SETS.items():
        p = c * 2 ** f - 1
        n = p.bit_length()
        red = (bin(c).count("1") + 3) * 5 * n
        fp2 = 2 * kara(n) + kara(n + 1) + 2 * red + 5 * 5 * n
        out[name] = {
            "n_bits": n,
            "int_mul_karatsuba_log2": math.log2(kara(n)),
            "int_mul_schoolbook_log2": math.log2(school(n)),
            "fp2_mul_gates_log2": math.log2(fp2),
            "fp2_mul_and_floor_log2": math.log2(3 * and_floor(n)),
            "log2p_squared_convention_log2": 2 * math.log2(math.log2(p)),
        }
    print(json.dumps(out, indent=1))


if __name__ == "__main__":
    main()
