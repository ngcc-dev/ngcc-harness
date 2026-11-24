#!/usr/bin/env python3
"""Fixing-cost arithmetic for sign-24-1 (Sigurd Reed-Solomon encoding).

Counts GF(2^16) table multiplications of one RS_encode call for
  - the submitted reference encoder (32 chunks, Horner evaluation);
  - the same Horner loop applied to the one global code of the specification;
  - a radix-2 transform of the next power-of-two length, in a butterfly
    model: n/2 * log2(n) butterflies per lane, each one multiplication and
    two additions (additions of a basis conversion are not modelled);
  - Horner evaluation at the Q_VC opened positions only (a verifier);
  - the team's revised encoder (github.com/QinLangT/Sigurd at 302eeeb,
    Reference_Implementation/Sigurd-*/rsencode.c, rs_eval_codeword): per lane
    and per coset of the order-51 subgroup, a fold of the N' coefficients
    (N' products), a chirp (51) and a 51-point transform: 17 three-point DFTs
    of one product, 51 twiddles and 3 direct 17-point DFTs of 289 products.
    Products there cost two table lookups (gf_mul_log), against three in the
    submitted gf_mul.

Nothing is measured and no part of the scheme is implemented.  Inputs:
  specification Table 2 (physical p. 30), Table 3 (p. 43);
  Reference_Implementation/Sigurd-{128,256,512}:
    sig_core.h enum            M, TAIL_LEN, N_PRIME_LEN, N, RAW_POLY_LEN, opennum
    sig_core.c RS_encode       32 chunks, k = ceil(N'/32), rate*k outputs per chunk
    rsencode.c rs_encode_scalar  symbols * recovery * original multiplications
    sig_core.c Commitment_Raw (two calls), Commitment_Open_Verifier_Raw (one)
  revised specification in the same repository (dated 2026-09-21), Table 3.

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
# Revised specification, Table 3 (reference build): sign, verify Mcyc
REVISED = {"128": (16.41, 7.00), "256": (136.18, 46.63), "512": (1149.03, 348.35)}

H = 51                     # kEvalOrder
NTT51 = 17 * 1 + 3 * 17 + 3 * 17 * 17      # eval_ntt51 products: 935
NTT51_ADD = 17 * 6 + 3 * 17 * 17           # eval_dft3, eval_dft17 additions
MAX_COSETS, MAX_FOLD = 400, 25             # table dimensions in rsencode.c


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
        cosets = ceil(n / H)
        team = tau * cosets * (nprime + H + NTT51)
        team_add = tau * cosets * (nprime + NTT51_ADD)
        print(f"  team's encoder: {cosets} cosets of {H}, {tau} lanes, "
              f"{nprime + H + NTT51} products per lane and coset = {team} "
              f"multiplications ({team / enc_chunk:.2f} of the submitted call), "
              f"{team_add} additions ({team_add / enc_chunk:.2f})")
        print(f"    table lookups at 2 per product against 3: {2 * team} "
              f"against {3 * enc_chunk} ({2 * team / (3 * enc_chunk):.2f})")
        rs, rv = REVISED[name]
        print(f"    revised Table 3, reference: signing {rs} Mcyc "
              f"({100 * (rs / sign_mcyc - 1):+.1f}%), verification {rv} Mcyc "
              f"({100 * (rv / ver_mcyc - 1):+.1f}%)")
    tables = MAX_COSETS * H + MAX_COSETS * MAX_FOLD + H * H + 17 * 17 + 17 * 3
    setup = MAX_COSETS * (H + MAX_FOLD) + H * H
    print(f"\nteam's encoder tables: {tables} entries of 2 bytes "
          f"({2 * tables} bytes) beside the shared log/exp tables; setup "
          f"once per process: {setup} products")


if __name__ == "__main__":
    main()
