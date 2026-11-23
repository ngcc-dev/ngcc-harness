#!/usr/bin/env python3
"""Fixing-cost count for kem-19-2 (Lore-384 and Lore-512, SHAKE backend).

Counts Keccak-f[1600] permutations per KEM operation in the submitted
reference code and after hash calls are moved to a sponge of capacity
2*lambda, in three variants: the calls that carry secret values, those plus
H(pk) and H(c), and every call including the public matrix expansion.  It
prints counts and an operation-count estimate only.  Nothing is measured and
no scheme is implemented.

Call lists follow kem.c, indcpa.c, sampler.c, poly.c and polyvec.c of
Reference_Implementation/Lore-SHAKE/Lore-L3 and Lore-L4.  The matrix
expansion is counted with the blocks that the code squeezes: 7 per polynomial
at n = 512 and 10 at n = 768, not the 5 of its first squeeze.
"""

def perms(inlen, outlen, rate):
    # fips202.c:89-130: absorb permutes lazily, finalize permutes once,
    # squeeze permutes lazily.
    return (inlen - 1) // rate + 1 + (outlen - 1) // rate

SETS = {
    # name: (lambda, n, k, t, R_BITS, HWT_TOTAL per polynomial)  params.h:45-70
    "Lore-384": (384, 512, 3, 4, 1, 220),
    "Lore-512": (512, 768, 3, 4, 1, 320),
}
NBLOCKS = 5            # polyvec.c:231, first squeeze of the matrix expansion
SHAKE128, SHAKE256 = 168, 136
KECCAK_OPS = 24 * 156  # word operations per permutation, fips202.c:33-71
MULACC_OPS = 2         # one 16-bit multiply and one add, toomcook.c:38-57

def sizes(n, k, rbits):
    pk = 32 + k * n + (k * n + 7) // 8 + k * ((n * rbits + 7) // 8) + 2   # params.h:108
    ct = k * n + (k * n + 7) // 8 + k * ((n * rbits + 7) // 8) + n // 8 + n * 2 // 8 + 2  # params.h:116
    return pk, ct

def gen_blocks(n):
    # polyvec.c:244-256 and poly.c:731-737: the q-part takes two bytes per
    # coefficient, first from the NBLOCKS buffer and then one block at a time.
    # The t-part rereads the buffer at one byte per coefficient
    # (polyvec.c:261, poly.c:761-771) and needs no further block for t = 4.
    return max(NBLOCKS, -(-2 * n // SHAKE128))

def count(lam, n, k, rbits, hwt, mode):
    """mode: 'submitted', 'secret' (secret-carrying calls widened),
    'all' (H(pk) and H(c) widened as well), 'every' (expandA as well)."""
    pk, ct = sizes(n, k, rbits)
    buf = k * hwt * 2 + 128        # sampler.c:22
    unif = 2 * n                   # poly.c:353
    msg = lam // 8                 # params.h:75
    if mode == "submitted":
        S, R, Rpub, Hpub = 32, SHAKE256, SHAKE256, 32
    else:
        S = lam // 8
        R = (1600 - 2 * lam) // 8
        Rpub, Hpub = (R, S) if mode in ("all", "every") else (SHAKE256, 32)
    Rgen = R if mode == "every" else SHAKE128
    gen = k * k * perms(34, gen_blocks(n) * SHAKE128, Rgen)  # public matrix
    hgen = perms(S, 32 + S, R)
    prf_s = perms(S + 1, buf, R)
    prf_e = perms(S + 1, unif, R)
    h_m = perms(msg, S, R)
    h_pk = perms(pk, Hpub, Rpub)
    h_ct = perms(ct, Hpub, Rpub)
    g = perms(S + Hpub, 2 * S, R)
    kdf = perms(S + Hpub, S, R)
    keygen = hgen + gen + prf_s + h_pk
    enc_core = gen + prf_s + prf_e
    encaps = h_m + h_pk + g + enc_core + h_ct + kdf
    decaps = h_m + g + enc_core + h_ct + kdf + h_ct + kdf
    return keygen, encaps, decaps

def mulacc(n):
    # toomcook.c: 7 Karatsuba calls, (n/16)^2 inner iterations, 9 products each
    return 7 * (n // 16) ** 2 * 9

WEIGHTS = (1, 4)       # cost of a Keccak word operation relative to one multiply or add of a multiply-accumulate

for name, (lam, n, k, t, rbits, hwt) in SETS.items():
    pk, ct = sizes(n, k, rbits)
    print(f"{name}: built pk {pk} B, ct {ct} B, wide rate {(1600 - 2 * lam) // 8} B, "
          f"matrix blocks per polynomial {gen_blocks(n)}")
    base = count(lam, n, k, rbits, hwt, "submitted")
    tmul = (k * k, k * k + k, k * k + 2 * k)   # mod-t products: keygen, encaps, decaps
    for mode in ("secret", "all", "every"):
        rep = count(lam, n, k, rbits, hwt, mode)
        for op, b, r, m in zip(("keygen", "encaps", "decaps"), base, rep, tmul):
            arith = m * mulacc(n) * MULACC_OPS
            hash_ops = b * KECCAK_OPS
            added = (r - b) * KECCAK_OPS
            incr = [100 * w * added / (w * hash_ops + arith) for w in WEIGHTS]
            share = [100 * w * hash_ops / (w * hash_ops + arith) for w in WEIGHTS]
            # weight at which the increase is 10%
            den = added - 0.1 * hash_ops
            w10 = f"{0.1 * arith / den:.1f}" if den > 0 else "never"
            print(f"  {mode:6s} {op}: permutations {b} -> {r} "
                  f"(+{100 * (r - b) / b:.1f}% of hashing); "
                  f"hash share {share[0]:.0f}% / {share[1]:.0f}%; "
                  f"operation +{incr[0]:.1f}% / +{incr[1]:.1f}% "
                  f"at weights {WEIGHTS[0]} / {WEIGHTS[1]}; 10% at weight {w10}")
