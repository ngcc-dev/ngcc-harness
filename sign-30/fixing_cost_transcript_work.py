#!/usr/bin/env python3
"""Fixing-cost aid for sign-30 (TRINE): work of a 1024-bit-capacity transcript hash at TRINE-512.

sign-30-3: the TRINE-512 transcript is hashed with SHAKE256 (rate 136 bytes). The costed
repair keeps Keccak-f[1600] and lowers the rate to 72 bytes (capacity 1024). This script
counts the permutation calls that adds, and compares them with a lower bound on the field
and XOF work of the same signing or verification, in 64-bit word operations. Nothing is
implemented, run or measured.

Counted per operation (Reference_Implementation/TRINE-512-*/):
  - Transcript: r encoded forms of ceil(n^3 * 12 / 8) bytes (trine.c:123-136), absorbed in
    signing (trine.c:360-386) and verification (trine.c:578-637).
  - Keccak-f[1600]: 152 XOR, 58 ROL, 50 AND and 50 NOT per two rounds in fips202.c:126-345,
    so 3,720 word operations per permutation.
  - Sampled corank-one points: one per round in signing, one per base-seed round (r - K)
    in verification (trine_derive_round_commitment_vartime, trine.c:138-176).
  - Trials per point: q. Specification p. 19 (physical): a random trilinear form has about
    q^(n-2) corank-one points among about q^(n-1) projective points (estimate).
  - Per trial (corank1.c:37-53), lower bound:
      phi_u: n^3 terms of one 64-bit multiply and one add (matrixmod.c:147-155);
      rank:  Gauss-Jordan over all rows (matrixelim.c:29-91), at least (n-1) pivots times
             (n-1) other rows times n columns, less rows whose factor is 0 (1/q), each one
             GF_mul and one GF_sub, counted as 2 word operations;
      XOF:   n values of 2 bytes squeezed from SHAKE256 (util.c:40-63), at least 2n/136
             permutations.
  Left out (so the percentages are upper bounds): canonical forms (about n^4 per round,
  spec p. 5), modular reductions of phi_u, rejected XOF values, retries after a failed
  canonical form, loads, stores and loop overhead, key and form expansion.

Run:  python3 -B sign-30/fixing_cost_transcript_work.py
"""

KECCAK_WORD_OPS = 24 * (152 + 58 + 50 + 50) // 2      # 3,720
RATE_SUBMITTED, RATE_REPAIRED = 136, 72              # fips202.h:8; capacity 1024 bits

# params.h:51-67 and :105-121 (TRINE_lambda 512)
SETS = {
    "TRINE-512 Balanced": dict(n=81, q=4093, r=534, K=211),
    "TRINE-512 ShortSig": dict(n=81, q=4093, r=232, K=149),
}


def ceil_div(a, b):
    return -(-a // b)


def main():
    print(f"Keccak-f[1600]: {KECCAK_WORD_OPS} word operations per permutation")
    for name, c in SETS.items():
        n, q, r, K = c["n"], c["q"], c["r"], c["K"]
        form = ceil_div(n ** 3 * 12, 8)
        total = r * form
        p136 = ceil_div(total, RATE_SUBMITTED)
        p72 = ceil_div(total, RATE_REPAIRED)
        added = p72 - p136
        added_ops = added * KECCAK_WORD_OPS

        phi = 2 * n ** 3
        rank = 2 * (n - 1) * (n - 1) * n * (q - 1) / q
        xof = 2 * n / RATE_SUBMITTED * KECCAK_WORD_OPS
        trial = phi + rank + xof
        point = q * trial

        print(f"\n{name}: n={n}, q={q}, r={r}, K={K}")
        print(f"  transcript: {r} x {form} = {total} bytes; permutations {p136} at rate 136,"
              f" {p72} at rate 72; added {added} = {added_ops:.4e} word operations")
        print(f"  per trial: phi_u {phi}, rank {rank:.0f}, XOF {xof:.0f}; total {trial:.0f}"
              f" word operations; per sampled point (q trials) {point:.4e}")
        for op, points in (("signing", r), ("verification", r - K)):
            base = points * point
            print(f"  {op}: {points} sampled points, at least {base:.4e} word operations;"
                  f" added transcript work at most {100.0 * added_ops / base:.2f}%")


if __name__ == "__main__":
    main()
