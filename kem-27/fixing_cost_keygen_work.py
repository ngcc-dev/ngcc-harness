#!/usr/bin/env python3
"""Fixing-cost count for kem-27-1 (NTRE-512 = NTRE-3457-2304 key generation).

Counts the dominant work of one key generation of the reference code
(KEM_AlgorithmInstance.c:87-115, src/ntt.c, src/poly.c, src/symmetric.c) with a
32-byte and a 64-byte sampling seed: the two sampling XOF calls, G(pk), two
NTTs, the blockwise inversion of f, one blockwise product and the packing.
The XOF is the contest pseudoXOF.  It is counted as built (one SM3 call per
32-byte output block, auxfunc.c:482-530) and idealized by calls and bytes
(the input absorbed once, then blocks of b bytes).  It prints counts only.
Nothing is measured and no scheme is implemented.
"""
N, Q = 2304, 3457
SAMPLEBYTES = N // 4           # src/params.h:19
PKBYTES = 3456                 # src/params.h:21

# ---- word operations of one SM3 compression (GB/T 32905-2016, section 5.3):
# 52 expanded words at 10 operations, 64 for W', 16 rounds at 21 and 48 at 24
# operations, 8 for the feed-forward.
SM3_OPS = 52 * 10 + 64 + 16 * 21 + 48 * 24 + 8

# ---- arithmetic, counted as 16- or 32-bit word operations
MONT = 4        # montgomery_reduce: two products, one subtraction, one shift
BARRETT = 5     # barrett_reduce: two products, an add, a shift, a subtraction
FQMUL = 1 + MONT

def ntt_ops():
    # src/ntt.c:167-209: one radix-2 layer over N/2 pairs, radix-3 layers at
    # steps N/6 and N/18 (384 and 128), radix-2 layers at steps 64..4.
    ops = (N // 2) * (FQMUL + 3)                      # t1, two adds, one sub
    for _ in (384, 128):
        ops += (N // 3) * (3 * FQMUL + 1 + 8)         # t1,t2,t3; t1-t2; 8 add/sub
    for _ in (64, 32, 16, 8, 4):
        ops += (N // 2) * (FQMUL + 2 + 2 * BARRETT)
    return ops

def baseinv_ops():
    # src/ntt.c:255-282 per 4-coefficient block: 12 products, 5 reductions
    # and about 8 add/sub for t0..t3; 8 products, 4 adds and 4 reductions for
    # r; fqinv with 12 squarings and 12 conditional products plus 3 fqmul;
    # 4 products and reductions for the scaling.
    fqinv = (12 + 12 + 3) * FQMUL + 12
    per = 12 + 5 * MONT + 8 + 8 + 4 + 4 * MONT + fqinv + 4 * FQMUL + 2
    return (N // 4) * per

def basemul_ops():
    # src/ntt.c:284-299: 16 products, 3 by zeta, 4 by R^2, 11 reductions, 12 adds
    return (N // 4) * (16 + 3 + 4 + 11 * MONT + 12)

def cbd_ops():
    return SAMPLEBYTES * 4 * 4 + N * 2               # poly_cbd1 and doubling

def pack_ops():
    return 2 * N * 3                                 # poly_tobytes twice

def ntt_free_arith():
    return 2 * ntt_ops() + baseinv_ops() + basemul_ops() + cbd_ops() + pack_ops()

def sm3_calls(L):
    return (L + 9 + 63) // 64

def xof_as_built(inlen, outlen):
    # one SM3 call on input || 4-byte counter per 32-byte output block
    return (outlen + 31) // 32 * sm3_calls(inlen + 4)

def xof_ideal(inlen, outlen, b):
    return (inlen + 1 + b - 1) // b + (outlen + b - 1) // b - 1

arith = ntt_free_arith()
g_pk = sm3_calls(PKBYTES)
print(f"SM3 compression {SM3_OPS} operations; one NTT {ntt_ops()}, "
      f"inversion {baseinv_ops()}, product {basemul_ops()}, "
      f"sampling and packing {cbd_ops() + pack_ops()}; arithmetic total {arith}")
for seed in (32, 64):
    inlen = 1 + seed                                 # src/symmetric.c:36-41
    built = 2 * xof_as_built(inlen, SAMPLEBYTES) + g_pk
    line = [f"seed {seed} B: as built {built} SM3 compressions "
            f"(XOF {2 * xof_as_built(inlen, SAMPLEBYTES)}, G(pk) {g_pk})"]
    for b in (64, 136):
        line.append(f"idealized b={b}: {2 * xof_ideal(inlen, SAMPLEBYTES, b)} XOF blocks")
    print("; ".join(line))

base_b = 2 * xof_as_built(33, SAMPLEBYTES) + g_pk
rep_b = 2 * xof_as_built(65, SAMPLEBYTES) + g_pk
tot = base_b * SM3_OPS + arith
print(f"as built: hash share {100 * base_b * SM3_OPS / tot:.0f}% of key generation; "
      f"key generation +{100 * (rep_b - base_b) * SM3_OPS / tot:.1f}%")
for b in (64, 136):
    # an idealized XOF block costed like one SM3 compression at b = 64, and
    # like 136/64 of one at b = 136 (same cost per byte)
    w = SM3_OPS * b / 64
    xb = 2 * xof_ideal(33, SAMPLEBYTES, b)
    xr = 2 * xof_ideal(65, SAMPLEBYTES, b)
    tot = xb * w + g_pk * SM3_OPS + arith
    print(f"idealized b={b}: XOF blocks {xb} -> {xr}; hash share "
          f"{100 * (xb * w + g_pk * SM3_OPS) / tot:.0f}%; key generation "
          f"+{100 * (xr - xb) * w / tot:.1f}%")
# The contest DRBG (drng.c:265-310) draws both seeds: per call one SM3
# compression of V (55 bytes) per 32 output bytes and two for the update.
# It is a placeholder for a perfect generator; idealized, 32 or 64 output
# bytes are one block at b >= 64.
drbg = {seed: 2 * ((seed + 31) // 32 + sm3_calls(56)) for seed in (32, 64)}
tot = (base_b + drbg[32]) * SM3_OPS + arith
print(f"DRBG as built: {drbg[32]} -> {drbg[64]} SM3 compressions; with it, as built "
      f"{base_b + drbg[32]} -> {rep_b + drbg[64]}, key generation "
      f"+{100 * (rep_b + drbg[64] - base_b - drbg[32]) * SM3_OPS / tot:.1f}%")
# arithmetic weighted half or double (e.g. reductions dearer than assumed)
for scale in (0.5, 2.0):
    tot = base_b * SM3_OPS + scale * arith
    xb = 2 * xof_ideal(33, SAMPLEBYTES, 64)
    tot_i = xb * SM3_OPS + g_pk * SM3_OPS + scale * arith
    print(f"arithmetic x{scale}: as built +{100 * (rep_b - base_b) * SM3_OPS / tot:.1f}%, "
          f"idealized b=64 +{100 * 2 * SM3_OPS / tot_i:.1f}%")
