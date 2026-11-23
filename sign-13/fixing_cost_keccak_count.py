#!/usr/bin/env python3
"""Fixing-cost counts for sign-13 (GreatWall). Stdlib only; prints numbers.

1. Control: signature sizes of all eight profiles from the signature tuple
   of Algorithm 2 against Tables 10 and 11 (physical p. 60).
2. Keccak-f[1600] calls per signature at GreatWall-512s/f in the submitted
   code, and the calls added when H1 absorbs at a rate of 576 bits
   (capacity 1024) instead of SHAKE256's 1088 bits (capacity 512).

Nothing here implements the scheme.
Run: python3 -B sign-13/fixing_cost_keccak_count.py
"""

from math import ceil

B = 16
# name, lambda, witness bits, tau, w_grind, T_open, k0, k1, tau0, signature bytes
# (tau, w, T_open and sizes: Tables 10/11; witness bits l = 16*ceil(n/8) for
#  w = (pt, x1), n from Table 2, as WITNESS_BITS in owf_proof.h:10-36;
#  k0, k1, tau0 from Algorithm 18 with lambda - w_grind challenge bits)
SETS = [
    ("128s", 128, 288, 11, 7, 100, 11, 11, 0, 2758),
    ("128f", 128, 288, 16, 8, 108, 8, 7, 8, 3396),
    ("192s", 192, 400, 16, 8, 183, 12, 11, 8, 6804),
    ("192f", 192, 400, 24, 8, 184, 8, 7, 16, 8012),
    ("256s", 256, 528, 22, 6, 246, 12, 11, 8, 12236),
    ("256f", 256, 528, 32, 7, 248, 8, 7, 25, 14260),
    ("512s", 512, 1056, 44, 0, 512, 12, 11, 28, 50012),
    ("512f", 512, 1056, 64, 0, 512, 8, 8, 0, 57812),
]


def absorb_calls(nbytes, rate_bytes):
    """Keccak-f calls to absorb nbytes plus padding and give one output block."""
    return nbytes // rate_bytes + 1


def main():
    print("1. Signature size control (bytes)")
    for (name, lam, ell, tau, w, topen, k0, k1, tau0, spec) in SETS:
        assert tau0 * k0 + (tau - tau0) * k1 == lam - w, name
        ellhat = ell + 2 * lam + B
        bits = ((tau - 1) * ellhat + (lam + B) + ell + lam
                + (2 * tau + topen) * lam + lam + 128 + 32)
        assert bits % 8 == 0 and bits // 8 == spec, name
        print("   GreatWall-%s  %6d" % (name, bits // 8))

    print()
    print("2. Keccak-f[1600] calls per signature at lambda = 512")
    lam = 512
    seed, iv = lam // 8, 16
    assert seed + iv + 1 < 136            # PRG input fits one SHAKE256 block
    totals = {}
    for (name, lam_, ell, tau, w, topen, k0, k1, tau0, spec) in SETS:
        if lam_ != 512:
            continue
        sizes = [2 ** k0] * tau0 + [2 ** k1] * (tau - tau0)
        leaves = sum(sizes)
        ellhat = ell + 2 * lam + B
        # tree PRG: 2*lambda bits per node in SHAKE256 calls of 32 bytes
        tree = (leaves - 1) * ((2 * lam // 8) // 32)
        # leaf H0: one SHAKE256 call squeezing 3*lambda bits = 192 bytes
        leaf = leaves * ceil((3 * lam // 8) / 136)
        # VOLE PRG: ceil(l_hat / 128) SHAKE256 calls of 16 bytes per leaf seed
        vole = leaves * ceil(ellhat / 128)
        # H1 over the leaf commitments of each vector (2*lambda bits each, one
        # domain byte), then over the tau digests
        def h1(rate):
            n = sum(absorb_calls(s * 2 * lam // 8 + 1, rate) for s in sizes)
            return n + absorb_calls(tau * 2 * lam // 8 + 1, rate)
        base = tree + leaf + vole + h1(136)
        added = h1(72) - h1(136)
        totals[name] = base
        print("   GreatWall-%s: leaves %6d; tree %7d, leaf %7d, VOLE %8d, H1 %6d"
              % (name, leaves, tree, leaf, vole, h1(136)))
        print("        total %8d; H1 at rate 72 bytes %6d; added %6d = +%.2f%%"
              % (base, h1(72), added, 100.0 * added / base))

    print()
    print("3. Upper bounds on the Keccak share of signing from stated timings (ms)")
    for label, s_ms, f_ms in (("Table 11, reference", 6087.004, 2424.320),
                              ("Table 10, optimized", 392.83, 80.90)):
        calls_s, calls_f = totals["512s"], totals["512f"]
        per_call = s_ms / calls_s            # if all of 512s signing were Keccak
        share_f = calls_f * per_call / f_ms
        other = f_ms * (1.0 - share_f)       # non-Keccak work at 512f, at least
        share_s = (s_ms - other) / s_ms
        print("   %s: 512f at most %.0f%%, 512s at most %.0f%%"
              % (label, 100 * share_f, 100 * share_s))


if __name__ == "__main__":
    main()
