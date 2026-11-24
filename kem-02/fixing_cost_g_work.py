#!/usr/bin/env python3
"""Fixing-cost count for kem-02-8 (Amoeba-1728 and Amoeba-2304).

Compares the work of G in one encapsulation as submitted (four SM3-256 calls
on m || counter || ID(pk), ccakem.c:35-45) with one call of a Keccak sponge of
capacity 2*lambda that returns the 1024 bits of G from ID(pk) || m.  It also
prints the SM3 compressions of the rest of encapsulation, as built and under
an idealized calls-and-bytes model of the contest pseudoXOF, and the
word-operation counts used to compare a Keccak-f[1600] call with an SM3
compression.  It prints counts only.  Nothing is measured and no scheme is
implemented.
"""
import math

# ---- word operations of one SM3 compression (GB/T 32905-2016, section 5.3)
# Message expansion W16..W67: W_j = P1(W_{j-16} ^ W_{j-9} ^ (W_{j-3} <<< 15))
#   ^ (W_{j-13} <<< 7) ^ W_{j-6}, with P1(X) = X ^ (X <<< 15) ^ (X <<< 23):
#   1 rotate + 2 xor, P1 2 rotates + 2 xor, 1 rotate + 2 xor = 10 operations.
SM3_EXPAND = 52 * 10
SM3_WPRIME = 64                      # W'_j = W_j ^ W_{j+4}
# One round: A<<<12 (1); SS1 = ((A<<<12) + E + T'_j) <<< 7 with T'_j a
# constant (3); SS2 (1); TT1 = FF + D + SS2 + W'_j (3 adds); TT2 = GG + H +
# SS1 + W_j (3 adds); B<<<9 (1); F<<<19 (1); E = P0(TT2) (4).  17 plus FF and
# GG: rounds 0-15 FF = GG = x^y^z (2 each); rounds 16-63 majority (4) and
# choose (3), in their cheapest forms.
SM3_ROUNDS = 16 * (17 + 2 + 2) + 48 * (17 + 4 + 3)
SM3_FEED = 8
SM3_OPS = SM3_EXPAND + SM3_WPRIME + SM3_ROUNDS + SM3_FEED

# ---- word operations of one Keccak-f[1600] permutation (FIPS 202, 3.2)
# theta: 20 xor for C, 5 rotate + 5 xor for D, 25 xor = 55; rho 24 rotates;
# pi none; chi 25 * (not, and, xor) = 75; iota 1: 155 per round, because rho
# leaves lane (0,0) unrotated.
KECCAK_LANE_OPS = 24 * (55 + 24 + 75 + 1)
KECCAK_32 = 2 * KECCAK_LANE_OPS      # bit-interleaved 64-bit lanes on 32-bit words

def sm3_calls(L):
    """SM3 compressions of one SM3 call on L bytes."""
    return (L + 9 + 63) // 64

def sponge_perms(inlen, outlen, rate):
    """Keccak-f calls of one sponge call: pad10*1 needs at least one byte."""
    return (inlen + 1 + rate - 1) // rate + (outlen + rate - 1) // rate - 1

SETS = {
    # name: (lambda, ct bytes (Table 4), Parse coefficients N*K,
    #        CBD output blocks per pseudoXOF call ceil(K*M/2 / 256))
    "Amoeba-1728": (384, 2769, 1728, 11),
    "Amoeba-2304": (512, 3914, 2304, 14),
}
SEED, MSG = 64, 64                   # RLWE_SEED_LEN, RLWE_MSG_LEN
P_ACC = 3457 / 4096                  # Parse acceptance, cpapke.c:117-124

print(f"SM3 compression: {SM3_OPS} 32-bit word operations")
print(f"Keccak-f[1600]: {KECCAK_LANE_OPS} 64-bit lane operations, "
      f"{KECCAK_32} 32-bit word operations")
rho_lo, rho_hi = KECCAK_LANE_OPS / SM3_OPS, KECCAK_32 / SM3_OPS
print(f"one permutation = {rho_lo:.2f} to {rho_hi:.2f} SM3 compressions")

for name, (lam, ct, nk, cbd_blocks) in SETS.items():
    # as built: SM3 compressions of one encapsulation
    g_sub = 4 * sm3_calls(MSG + 1 + SEED)                       # ccakem.c:41-45
    parse_calls = math.ceil(nk / (128 * P_ACC))                 # cpapke.c:107-127
    parse = parse_calls * 6 * sm3_calls(SEED + 1 + 4)           # 192 B = 6 blocks
    cbd = 3 * 2 * cbd_blocks * sm3_calls(SEED + 2 + 4)          # cpapke.c:41-95
    kdf = 2 * sm3_calls(ct + SEED + 4)                          # ccakem.c:51
    total = g_sub + parse + cbd + kdf
    # idealized pseudoXOF: absorb once, 64-byte blocks (calls and bytes)
    i_parse = parse_calls * sponge_perms(SEED + 1, 192, 64)
    i_cbd = 3 * 2 * sponge_perms(SEED + 2, cbd_blocks * 32, 64)
    i_kdf = sponge_perms(ct + SEED, 64, 64)
    i_total = g_sub + i_parse + i_cbd + i_kdf
    rate = (1600 - 2 * lam) // 8
    g_new = sponge_perms(SEED + MSG, 128, rate)
    g_sha3 = 2 * sponge_perms(1 + SEED + MSG, 64, 72)          # SHA3-512 twice
    print(f"\n{name}: capacity {2 * lam}, rate {rate} bytes")
    print(f"  as built, SM3 compressions: G {g_sub}, Parse {parse} "
          f"({parse_calls} calls), CBD {cbd}, KDF {kdf}, total {total}")
    print(f"  pseudoXOF idealized (64-byte blocks, absorbed once): Parse "
          f"{i_parse}, CBD {i_cbd}, KDF {i_kdf}, total with G {i_total}")
    for label, perms in (("sponge c=2*lambda", g_new), ("SHA3-512 twice", g_sha3)):
        lo, hi = perms * rho_lo, perms * rho_hi
        print(f"  repaired G, {label}: {perms} permutations = {lo:.1f} to "
              f"{hi:.1f} compressions against {g_sub}; change "
              f"{100 * (lo - g_sub) / total:+.1f}% to {100 * (hi - g_sub) / total:+.1f}% "
              f"of the as-built total, {100 * (hi - g_sub) / i_total:+.1f}% at most "
              f"of the idealized total")
    for pct in (1, 10):
        print(f"  sponge G: {pct}% reached at more than "
              f"{(g_sub + pct / 100 * total) / g_new:.1f} (as built) or "
              f"{(g_sub + pct / 100 * i_total) / g_new:.1f} (idealized) "
              f"compressions per permutation")
