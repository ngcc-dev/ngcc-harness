#!/usr/bin/env python3
"""Fixing-cost arithmetic for sign-24-1 (Sigurd Reed-Solomon encoding).

Counts GF(2^16) table multiplications of one RS_encode call for
  - the submitted reference encoder (32 chunks, Horner evaluation);
  - the same Horner loop applied to the one global code of the specification;
  - a radix-2 transform of the next power-of-two length, in a butterfly
    model: n/2 * log2(n) butterflies per lane, each one multiplication and
    two additions (additions of a basis conversion are not modelled);
  - Horner evaluation at the Q_VC opened positions only (a verifier).

Nothing is measured and no part of the scheme is implemented.  Inputs:
  specification Table 2 (physical p. 30), Table 3 (p. 43);
  Reference_Implementation/Sigurd-{128,256,512}:
    sig_core.h enum            M, TAIL_LEN, N_PRIME_LEN, N, RAW_POLY_LEN, opennum
    sig_core.c RS_encode       32 chunks, k = ceil(N'/32), rate*k outputs per chunk
    rsencode.c rs_encode_scalar  symbols * recovery * original multiplications
    sig_core.c Commitment_Raw (two calls), Commitment_Open_Verifier_Raw (one)

Run: python3 -B sign-24/fixing_cost_rs_work.py
"""

from math import ceil, log2

B = 6          # block size, Table 2
CHUNKS = 32    # RS_encode chunk_count

# name: (m, tau = RAW_POLY_LEN, Q_VC = opennum, rate factor, TAIL_LEN,
#        N' Table 2, N Table 2, sign Mcyc, verify Mcyc from Table 3)
SETS = {
    "128": (1302, 10, 160, 4, 175, 392, 1568, 16.18, 6.98),
    "256": (2748, 16, 171, 8, 193, 651, 5208, 140.72, 52.18),
    "512": (5676, 32, 256, 16, 293, 1239, 19824, 1277.34, 461.05),
}


def main():
    print("sign-24-1 Reed-Solomon encoding; GF(2^16) multiplications per call")
    for name, (m, tau, q, rate, tail, nprime_spec, n_spec,
               sign_mcyc, ver_mcyc) in SETS.items():
        nprime = m // B + tail
        n = rate * nprime
        assert (nprime, n) == (nprime_spec, n_spec), name   # control, Table 2

        k = ceil(nprime / CHUNKS)          # symbols_per_chunk
        rec = rate * k                     # recovery_block_count
        lanes = CHUNKS * tau               # elements_per_block
        enc_chunk = lanes * rec * k        # rs_encode_scalar as submitted
        enc_global = tau * n * nprime      # same Horner loop, one code
        n2 = 1 << ceil(log2(n))
        enc_fft = tau * (n2 // 2) * int(log2(n2))   # butterflies, one mult each
        enc_open = tau * q * nprime        # Horner at the opened positions

        print(f"\nSigurd-{name}: N'={nprime} N={n} tau={tau} "
              f"(Table 2: {nprime_spec}, {n_spec})")
        print(f"  submitted: {CHUNKS} x ({k} -> {rec}) chunks, {lanes} lanes "
              f"= {enc_chunk} multiplications, {enc_chunk} additions")
        print(f"  transform of length {n2}: {enc_fft} butterflies "
              f"= {enc_fft} multiplications ({enc_fft / enc_chunk:.2f} of the "
              f"submitted call), {2 * enc_fft} additions "
              f"({2 * enc_fft / enc_chunk:.2f}); basis conversion not counted")
        print(f"  Horner loop on one ({nprime} -> {n}) code: {enc_global} "
              f"multiplications, ratio {enc_global / enc_chunk:.1f}")
        add_s = 2 * (enc_global - enc_chunk)
        add_v = enc_global - enc_chunk
        print(f"    signing (2 calls) adds {add_s}, verification (1 call) "
              f"adds {add_v}")
        print(f"    per baseline cycle of Table 3 ({sign_mcyc} and {ver_mcyc} "
              f"Mcyc): signing {add_s / (sign_mcyc * 1e6):.2f}, verification "
              f"{add_v / (ver_mcyc * 1e6):.2f}")
        print(f"  Horner loop at the {q} opened positions only: {enc_open} "
              f"multiplications ({enc_open / enc_chunk:.2f} of the submitted "
              f"call)")


if __name__ == "__main__":
    main()
