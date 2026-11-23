#!/usr/bin/env python3
"""Counts for sign-04/fixing_cost.md (sign-04-1). Standard library only.

For the two 160-bit CEDRUS-alpha sets this prints the number of constant-sum
encodings |C_s| that the specified WOTS parameters admit (Table 1.1 and
Algorithm 6 line 4, specification physical pp. 37 and 23), the widest value
the encoder's table must hold, and the encoder's integer work per encoding.
It implements no part of the scheme.

Run: python3 -B sign-04/fixing_cost_encoding_count.py
"""

from math import log2

# name: (n bytes, d, chain widths w) from Table 1.1, physical p. 37.
SETS = {
    "CEDRUSalpha-160s": (20, 9, [49] * 2 + [48] * 28),
    "CEDRUSalpha-160f": (20, 17, [18] * 35 + [17] * 5),
}


def table(widths, total):
    """rows[i][j] = number of digit vectors over the first i chains with sum j."""
    rows = [[1] + [0] * total]
    for w in widths:
        prev = rows[-1]
        rows.append([sum(prev[j - x] for x in range(min(w - 1, j) + 1))
                     for j in range(total + 1)])
    return rows


for name, (n, d, widths) in SETS.items():
    s = sum(w - 1 for w in widths) // 2          # Algorithm 6, line 4
    rows = table(widths, s)
    count = rows[len(widths)][s]
    widest = max(max(r) for r in rows[:-1])      # entries the encoder compares
    print(name)
    print("  len = %d, constant sum cs = %d" % (len(widths), s))
    print("  log2 |C_cs| = %.3f, target 8n = %d, enough: %s"
          % (log2(count), 8 * n, count >= 1 << (8 * n)))
    print("  widest table entry used by the encoder: %d bits "
          "(64-bit words needed: %d; submitted WOTS_UINT_WORDS = n/8 = %d)"
          % (widest.bit_length(), -(-widest.bit_length() // 64), n // 8))
    print("  compare/subtract steps per encoding: at most sum(w) = %d; "
          "per signature or verification (d = %d): %d"
          % (sum(widths), d, d * sum(widths)))
