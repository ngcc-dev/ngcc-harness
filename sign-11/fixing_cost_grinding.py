#!/usr/bin/env python3
"""Counts for sign-11/fixing_cost.md (sign-11-1). Standard library only.

Prints, for the eight FlexTree parameter sets (Table 1.3, physical p. 46):
  - the signature size from Equation (4.1), as a control against Table 1.3;
  - the exact forced-pruning acceptance probability tau(t, k, mMAX) and the
    expected number of H_PORS trials 1/tau per signature;
  - the hash-call counts of Section 4.1, as a control against Table 4.1;
  - the H_PORS output length, and the length when the bytes that select the
    PORS+FP instance (idxTree, idxLeaf) are taken from the same output;
  - the specification's bound (3.7) at 2^64 signatures.
It counts and implements no part of the scheme.

Run: python3 -B sign-11/fixing_cost_grinding.py        (about four minutes)
"""

from decimal import Decimal, localcontext
from fractions import Fraction
from math import ceil, comb, log2


def chains(*parts):
    out = []
    for width, count in parts:
        out += [width] * count
    return out


# name: n, h, d, t, k, mMAX, w, zb (Table 1.3, p. 46);
#       stated signature bytes (Table 1.3); signing and verification hash
#       calls (Table 4.1, p. 61).
SETS = {
    "160s": (20, 67, 9, 42429, 15, 160, chains((32, 1), (64, 25)), 5, 9580, 2904774, 7485),
    "160f": (20, 67, 17, 4721, 27, 155, chains((8, 1), (16, 39)), 1, 18672, 179947, 5309),
    "256s": (32, 67, 10, 81271, 25, 280, chains((64, 42)), 4, 25420, 3204392, 13628),
    "256f": (32, 68, 17, 16900, 35, 270, chains((8, 1), (16, 63)), 1, 46856, 322360, 8501),
    "384s": (48, 67, 8, 250491, 35, 398, chains((16, 94)), 8, 60180, 5213804, 6176),
    "384f": (48, 64, 12, 71828, 53, 486, chains((16, 3), (32, 74)), 2, 73396, 1448594, 14697),
    "512s": (64, 66, 8, 429271, 48, 560, chains((32, 101)), 7, 94948, 9790642, 13251),
    "512f": (64, 66, 11, 181922, 57, 586, chains((16, 1), (32, 101)), 3, 117296, 2836460, 18070),
}


def tau_exact(t, k, mmax):
    """Pr[|octopus(I)| <= mMAX] for a uniform k-subset I of the t PORS leaves.

    The PORS tree (Algorithms 17, 18 and 21) has 2^(hPORS-1) nodes at height
    1. The first s = t - 2^(hPORS-1) of them have two leaves below; the others
    are leaves themselves. A node adds one authentication node when exactly
    one of its two subtrees holds a selected leaf. For each subtree the code
    keeps, per number j of selected leaves, a polynomial in y whose
    coefficient of y^m counts the selections with m authentication nodes;
    polynomials are packed into integers and truncated above mMAX.
    """
    slot = comb(t, k).bit_length() + 8
    mask = (1 << (slot * (mmax + 1))) - 1
    hp = t.bit_length()
    base = 1 << (hp - 1)
    assert base <= t < 2 * base
    s = t - base

    def combine(left, right):
        out = [0] * (k + 1)
        for a in range(1, k):
            x = left[a]
            if x:
                for b in range(1, k + 1 - a):
                    y = right[b]
                    if y:
                        out[a + b] += x * y
        for j in range(1, k + 1):
            out[j] = (out[j] + ((left[j] + right[j]) << slot)) & mask
        return out

    leaf = [0, 1] + [0] * (k - 1)
    pairs = {0: leaf}                  # complete subtrees whose leaves are at height 0
    for z in range(1, hp + 1):
        pairs[z] = combine(pairs[z - 1], pairs[z - 1])
    singles = {1: leaf}                # complete subtrees whose leaves are at height 1
    for z in range(2, hp + 1):
        singles[z] = combine(singles[z - 1], singles[z - 1])

    def node(z, i):                    # height z >= 1, index i
        lo, hi = i << (z - 1), (i + 1) << (z - 1)
        if hi <= s:
            return pairs[z]
        if lo >= s:
            return singles[z]
        return combine(node(z - 1, 2 * i), node(z - 1, 2 * i + 1))

    packed = node(hp, 0)[k]
    good = sum((packed >> (slot * m)) & ((1 << slot) - 1) for m in range(mmax + 1))
    return Fraction(good, comb(t, k)), hp, s


def constant_sum_count(widths):
    total = sum(w - 1 for w in widths) // 2
    row = [1] + [0] * total
    for w in widths:
        row = [sum(row[j - x] for x in range(min(w - 1, j) + 1)) for j in range(total + 1)]
    return total, row[total]


def bound_3_7(h, t, k, log2_qsig=64, terms=400):
    """-log2 of the sum in Equation (3.7), physical p. 57."""
    with localcontext() as ctx:
        ctx.prec = 400
        two = Decimal(2)
        qsig = 1 << log2_qsig
        p = two ** (-h)
        load = ((Decimal(1) - p).ln() * Decimal(qsig)).exp()
        total = Decimal(0)
        denom = Decimal(comb(t, k))
        for q in range(terms):
            if q:
                total += load * Decimal(min(comb(k * q, k), comb(t, k))) / denom
            load *= (Decimal(qsig - q) / Decimal(q + 1)) * p / (Decimal(1) - p)
        return float(-(total.ln() / two.ln()))


def main():
    for name, (n, h, d, t, k, mmax, widths, zb, sig, sign_calls, verify_calls) in SETS.items():
        length = len(widths)
        size = n + (4 + k * n + mmax * n) + d * (4 + length * n) + h * n      # (4.1)
        tau, hp, s = tau_exact(t, k, mmax)
        trials = float(1 / tau)

        cs, count = constant_sum_count(widths)
        prod_w = 1
        for w in widths:
            prod_w *= w
        sign_ots = (1 << zb) * prod_w / count + cs + length                   # (4.5)
        node0 = sum(widths) + 1                                               # (4.2)
        quotient, rest = divmod(h, d)
        heights = [quotient + 1] * rest + [quotient] * (d - rest)
        leaves = sum(1 << x for x in heights) - d
        fts = trials + 3 * t - 1
        sign_text = 1 + fts + d * sign_ots + (node0 + 1) * leaves             # p. 60
        sign_table = 1 + fts + d * sign_ots + node0 * leaves
        verify = 1 + (2 * k + mmax) + ceil(sum(w - 1 for w in widths) / 2) * d + h   # (4.7)

        out_now = n + ceil(log2(comb(t, k)) / 8)                              # Tables 1.4, 1.5
        index_bytes = ceil((h - heights[0]) / 8) + ceil(heights[0] / 8)       # Algorithm 23
        out_new = out_now + index_bytes
        extra_blocks = ceil(out_new / 32) - ceil(out_now / 32)

        print("FlexTree-%s" % name)
        print("  signature bytes, Equation (4.1): %d (Table 1.3: %d)" % (size, sig))
        print("  hPORS = %d, s = %d, tau = 2^%.3f, expected H_PORS trials 1/tau = %.1f"
              % (hp, s, log2(tau), trials))
        print("  signing hash calls: %.0f by the text of p. 60, %.0f with C_xmssNode(0) "
              "in place of C_xmssNode(0)+1 (Table 4.1: %d)" % (sign_text, sign_table, sign_calls))
        print("  verification hash calls, Equation (4.7): %d (Table 4.1: %d)" % (verify, verify_calls))
        print("  H_PORS output: %d bytes; with %d index bytes: %d bytes; "
              "extra 32-byte output blocks per trial: %d" % (out_now, index_bytes, out_new, extra_blocks))
        print("  trials as a share of signing hash calls: %.3f%%; one extra hash call per trial "
              "would add at most that" % (100 * trials / sign_calls))
        print("  minimal-counter alternative: %.1f more H_PORS calls per verification, "
              "%.1f%% of the verification hash calls" % (trials - 1, 100 * (trials - 1) / verify_calls))
        print("  specification bound (3.7) at 2^64 signatures: %.3f bits (8n = %d)"
              % (bound_3_7(h, t, k), 8 * n))


if __name__ == "__main__":
    main()
