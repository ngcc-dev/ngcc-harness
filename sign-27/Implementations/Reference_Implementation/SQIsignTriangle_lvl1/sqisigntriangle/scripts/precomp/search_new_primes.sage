#!/usr/bin/env sage
# search_new_primes.sage
#
# Find SQIsign-style primes  p = c * 2^f - 1   (so p+1 = c*2^f)  for the two new
# security levels, mirroring the existing levels:
#       lvl1:  5 * 2^248 - 1   (251 bits, f=248, lambda=128)
#       lvl3: 65 * 2^376 - 1   (383 bits, f=376, lambda=192)
#       lvl5: 27 * 2^500 - 1   (505 bits, f=500, lambda=256)
#
# Selection criterion (matching the existing levels): for a given 2-adic
# valuation f, take the SMALLEST odd cofactor c making p = c*2^f - 1 prime.
#   * f >= 2  =>  p ≡ 3 (mod 4) automatically (needed for the supersingular setup).
#   * bit_length(p) ≈ f + log2(c) ≈ 2*lambda  (the existing primes sit a few bits
#     below 2*lambda: 251<256, 383<384, 505<512).
#   * f = TORSION_EVEN_POWER (the available 2^f torsion) — the key functional knob.
#   * c = P_COFACTOR_FOR_2F provides the small odd torsion (lvl1 c=5, lvl3 c=65=5*13,
#     lvl5 c=27=3^3).
#
# NOTE on SECURITY_BITS: scripts/precomp/precompute_sizes.sage computes
#   SECURITY_BITS = round(p.bit_length()/128) * 64  -> only multiples of 64.
#   lvl6 (~1024 bits) -> 512 automatically. BUT lvl2 (~320 bits) -> 128 (wrong);
#   it must be overridden to 160 in that script. This script prints the auto value
#   so you can see the mismatch.
#
# Run:   sage scripts/precomp/search_new_primes.sage
#
# Review the candidates, then pick (per level) the row with the smallest c and
# bit_length closest to (but not exceeding) 2*lambda, mirroring lvl1/3/5.

from sage.all import ZZ, is_prime, factor, log


def smallest_c(f, max_c=200000):
    """Smallest odd c >= 1 with c*2^f - 1 prime. Returns (c, p) or (None, None)."""
    c = 1
    while c <= max_c:
        p = c * 2**f - 1
        if p > 2 and is_prime(p):
            return c, ZZ(p)
        c += 2
    return None, None


def report(level_name, lam, f_lo, f_hi):
    target = 2 * lam
    print("=" * 78)
    print(f"{level_name}: target lambda = {lam}   (aim p ~ {target} bits, f ~ {target})")
    print("=" * 78)
    print(f"  {'f':>5}  {'c':>7}  {'c factored':>14}  {'p.nbits':>7}  {'auto_SB':>7}")
    for f in range(f_lo, f_hi + 1):
        c, p = smallest_c(f)
        if p is None:
            print(f"  {f:>5}  (no small-c prime found)")
            continue
        auto_sb = round(p.nbits() / 128) * 64
        cf = str(factor(ZZ(c))) if c > 1 else "1"
        print(f"  {f:>5}  {c:>7}  {cf:>14}  {p.nbits():>7}  {auto_sb:>7}")
        print(f"            p = 0x{ZZ(p).hex()}")
    print()


# lvl2: lambda=160  ->  p ~ 320 bits  ->  f around 308..318
report("lvl2", 160, 308, 318)

# lvl6: lambda=512  ->  p ~ 1024 bits ->  f around 1008..1020
report("lvl6", 512, 1008, 1020)

print("Reminder: lvl2 needs SECURITY_BITS hard-set to 160 (auto-formula gives 128).")
