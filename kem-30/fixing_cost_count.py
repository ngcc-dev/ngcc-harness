#!/usr/bin/env python3
"""Operation counts for the kem-30-1 fixing-cost note (PolarLAC).

Counts the work of the submitted reference decapsulation and the work added
by a fixed-work screened sampler that always generates 2k + s candidates.
Nothing is measured and no scheme is implemented.  Standard library only.

Inputs, all read from the frozen specification and the reference source:
  n, k, q          Table 3, physical p. 22
  rej              rejection rate of T0 per polynomial, Table 2, physical p. 21
  dfr              claimed failure-rate exponent, Table 3, physical p. 22
  pk, ct           sizes in bytes, Table 3, physical p. 22
  bits             XOF bits per ternary coefficient, sample.c of each set
  base             fqmul calls per base_mul block, ntt.c of each set
  msg, pkseed      MESSAGE_LEN_BYTES, PK_SEED_LEN_BYTES, params.h of each set
"""
from math import comb, log2

SEED = 64      # KEM_SEED_LEN_BYTES, params.h:12
XOFHDR = 5     # nonce and block counter, symmetric.c:109-117
FQMUL = 3      # integer multiplications per fqmul, ntt.c:62-83
TWMUL = 3      # integer multiplications per ci16_mul_tw, fft.c:317-335

SETS = [
    # name            n    k  q    rej    dfr  pk    ct    bits base msg pkseed
    ("PolarLAC-Light", 256, 2, 257, 0.017, 159, 530, 608, 3, 5, 16, 16),
    ("PolarLAC-128", 256, 2, 257, 0.011, 146, 530, 640, 4, 5, 16, 16),
    ("PolarLAC-256", 512, 2, 257, 0.011, 265, 1060, 1280, 3, 13, 32, 32),
    ("PolarLAC-512", 1024, 2, 257, 0.014, 324, 2116, 2560, 7, 43, 64, 64),
    ("PolarLAC-512*", 1024, 2, 769, 0.012, 545, 2522, 2970, 4, 43, 64, 64),
]


def fft_twiddles(n):
    """ci16_mul_tw calls of real_fft_unique_raw_i16_n<n>, fft.c:347-398."""
    return 1 if n == 2 else 2 * fft_twiddles(n // 2) + n // 4


def overflow_log2(cands, spare, p):
    """log2 Pr[more than `spare` of `cands` candidates are rejected]."""
    tail = sum(comb(cands, j) * p ** j * (1 - p) ** (cands - j)
               for j in range(spare + 1, cands + 1))
    return log2(tail)


def spares_needed(screened, p, dfr):
    s = 0
    while overflow_log2(screened + s, s, p) > -dfr:
        s += 1
    return s


print("set             cand_bytes cand_mults  base_bytes base_mults"
      "   +1 spare (hash, mult)   s   +s spares (hash, mult)")
PER_SPARE = {}
for name, n, k, q, rej, dfr, pk, ct, bits, base, msg, pkseed in SETS:
    screened = 2 * k
    # One ternary candidate: one XOF call and one FFT bound check.
    cand_bytes = SEED + XOFHDR + n * bits // 8
    cand_mults = TWMUL * fft_twiddles(n) + n        # n/2 norms, 2 mults each
    # Decapsulation hashing, bytes absorbed plus bytes produced.
    g = (msg + pk) + (msg + SEED)                    # derive_g
    h = (msg + ct) + msg                             # derive_reject_key
    a_out = n if q == 257 else 1229                  # one matrix polynomial
    a = k * k * (pkseed + 2 + XOFHDR + a_out)
    expected = screened / (1 - rej) + 1              # candidates incl. e2
    base_bytes = g + h + a + expected * cand_bytes
    # Decapsulation arithmetic, integer multiplications.
    ntt = 7 * n // 2 * FQMUL
    intt = (7 * n // 2 + n) * FQMUL
    pw = 128 * base * FQMUL
    if q == 257:    # pke.c:144-234
        dec = k * ntt + k * pw + intt
        enc = k * ntt + (k * k + k) * pw + (k + 1) * intt
    else:           # 512*: u stays in the NTT domain
        dec = k * pw + intt
        enc = 2 * k * ntt + (k * k + k) * pw + intt
    base_mults = dec + enc + (expected - 1) * cand_mults
    s = spares_needed(screened, rej, dfr)
    one = (cand_bytes / base_bytes, cand_mults / base_mults)
    PER_SPARE[name] = (one, expected - 1 - screened)
    # The expected rejections of the submitted loop are already in the base.
    extra = s - (expected - 1 - screened)
    many = (extra * cand_bytes / base_bytes, extra * cand_mults / base_mults)
    print(f"{name:15s} {cand_bytes:10d} {cand_mults:10d} {base_bytes:11.0f}"
          f" {base_mults:10.0f}   +{100 * one[0]:4.1f}%  +{100 * one[1]:4.1f}%"
          f"   {s:3d}   +{100 * many[0]:4.0f}%  +{100 * many[1]:4.0f}%")

print()
print("Overflow probability, log2, of 2k + s candidates (PolarLAC-128):")
for s in (0, 1, 2, 3, 4, 8, 16, 24):
    print(f"  s = {s:2d}: 2^{overflow_log2(4 + s, s, 0.011):.1f}")

# Sensitivity only: the looser design threshold T of Table 2, whose rejection
# rate is stated only as an upper bound lambda^-3.  The specification advises
# against using T as the real threshold (physical p. 26).
print()
print("Spare candidates if T (rate below lambda^-3) replaced T0:")
for (name, n, k, q, rej, dfr, *_), lam in zip(SETS, (128, 128, 256, 512, 512)):
    s = spares_needed(2 * k, lam ** -3.0, dfr)
    (hash_one, mult_one), already = PER_SPARE[name]
    print(f"  {name:15s} rate < {lam}^-3: s = {s:2d}"
          f"   +{100 * (s - already) * hash_one:3.0f}%"
          f"  +{100 * (s - already) * mult_one:3.0f}%")
