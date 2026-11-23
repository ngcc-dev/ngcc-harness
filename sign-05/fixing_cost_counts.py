#!/usr/bin/env python3
"""Fixing-cost counts for sign-05 (Chinith). Stdlib only; prints numbers.

1. Control: signature sizes from the formula of specification section 10.1
   (physical p. 113) against Table 16.
2. Bytes hashed per signature, and the bytes added when the final challenge
   also absorbs a~0 (lambda_F / 8 bytes per grinding attempt).
3. GF(2^512) multiplications of the Vistrutith QuickSilver prover and
   verifier, as submitted and with the three coefficient vectors that
   Vistrutith.OWFProve specifies (physical p. 90).

Nothing here implements the scheme. Run: python3 -B sign-05/fixing_cost_counts.py
"""

# name, lambda, lambda_F, ell, tau, w_grind, T_open, B, d, lambda_iv, N_block
# (specification Table 1, physical p. 12), signature bytes (Table 16, p. 113)
SETS = [
    ("SM4th-d3-128s-lo", 128, 128, 1808, 11, 7, 102, 16, 3, 128, 128, 5056),
    ("SM4th-d3-128f-lo", 128, 128, 1808, 16, 8, 110, 16, 3, 128, 128, 6724),
    ("SM4th-EM-d2-128s-lo", 128, 128, 1024, 11, 7, 103, 16, 2, 128, 128, 3818),
    ("SM4th-EM-d2-128f-lo", 128, 128, 1024, 16, 8, 112, 16, 2, 128, 128, 4932),
    ("SM4th-d3-128s-ti", 128, 160, 1808, 14, 7, 131, 16, 3, 256, 256, 9176),
    ("SM4th-d3-128f-ti", 128, 160, 1808, 21, 8, 138, 16, 3, 256, 256, 11864),
    ("SM4th-EM-d2-128s-ti", 128, 160, 1024, 14, 7, 132, 16, 2, 256, 256, 7556),
    ("SM4th-EM-d2-128f-ti", 128, 160, 1024, 21, 8, 140, 16, 2, 256, 256, 9450),
    ("uBlockith-d3-256s", 256, 256, 4608, 22, 6, 245, 16, 3, 256, 256, 24144),
    ("uBlockith-d3-256f", 256, 256, 4608, 32, 8, 246, 16, 3, 256, 256, 31556),
    ("uBlockith-EM-d3-256s", 256, 256, 3072, 22, 6, 218, 16, 3, 256, 256, 19056),
    ("uBlockith-EM-d3-256f", 256, 256, 3072, 32, 8, 234, 16, 3, 256, 256, 25028),
    ("Vistrutith-d3-512s", 512, 512, 6912, 44, 5, 480, 16, 3, 256, 512, 83004),
    ("Vistrutith-d3-512f", 512, 512, 6912, 64, 8, 481, 16, 3, 256, 512, 106788),
]


def sizes_and_hash_bytes():
    print("1-2. Signature size control and bytes hashed per signature")
    print("%-22s %7s %7s %9s %7s %7s" %
          ("set", "sig", "L", "hashed", "added", "share"))
    for (name, lam, lf, ell, tau, w, topen, b, d, liv, nb, spec) in SETS:
        ellhat = ell + d * lf + b
        bits = tau * ellhat + topen * nb + 2 * nb * tau + lf + liv + 32
        assert bits % 8 == 0 and bits // 8 == spec, name
        # leaves (Table 3 formulas, physical p. 13)
        tau1 = (lf - w) % tau
        tau0 = tau - tau1
        k = (lf - w) // tau + 1
        leaves = tau1 * 2 ** k + tau0 * 2 ** (k - 1)
        # bytes absorbed: H1 over leaf commitments and tree digests,
        # chall1 = H2^1(mu | com | c_1.. | iv), chall2 = H2^2(chall1 | u~ | V~ | d),
        # chall3 = H2^3(chall2 | a~_1.. | ctr | domain), 2^w_grind attempts.
        h1 = (leaves + tau) * 2 * nb // 8
        c1 = 2 * lam // 8 + 2 * nb // 8 + (tau - 1) * ellhat // 8 + liv // 8
        c2 = (5 * lf + 64) // 8 + (lf + b) // 8 + (lf + b) * lf // 8 + ell // 8
        attempts = 2 ** w
        grind = attempts * ((3 * lf + 64) // 8 + (d - 1) * lf // 8 + 5)
        added = attempts * lf // 8
        total = h1 + c1 + c2 + grind
        print("%-22s %7d %7d %9d %7d %6.2f%%" %
              (name, bits // 8, leaves, total, added, 100.0 * added / total))


def vistrutith_multiplications():
    print()
    print("3. Vistrutith GF(2^512) multiplications per signature")
    print("   (bf512_mul = 1, bf512_mul_64 = 1/8; 576 S-boxes per layer type)")
    sboxes = 576              # 9 ForwardByNorm calls x 64 bytes, same for Append
    zk = 1.0 + 1.0 / 8.0      # one zk_hash update of one coefficient
    comb = 7                  # bf512_byte_combine: 7 multiplications

    # Submitted prover (vistrutith_constraints.c, (val, tag) tracks)
    fwd_sub = 8 * comb + 12 + 4 + 16 + 32 + 16 + 2 * comb
    app_sub = 2 * comb + 4
    prov_sub = sboxes * (fwd_sub + app_sub) + 577 * 3 * zk

    # Corrected prover, values kept in GF(2^8): coefficients a0, a1 of every
    # degree-2 value and a0, a1, a2 of every degree-3 constraint.
    fwd_min = 8 * comb + 12 + 8 + 8 * 3 + 32 + 16 + 2 * comb
    app_min = 2 * comb + 5 + 5
    prov_min = sboxes * (fwd_min + app_min) + 1729 * 3 * zk

    # Corrected prover, direct extension of the submitted code: its value
    # track kept in GF(2^512) and a third full-width track added.
    fwd_dir = 8 * comb + 12 + 9 + 8 * 4 + 48 + 24 + 2 * comb
    app_dir = 2 * comb + 5 + 5
    prov_dir = sboxes * (fwd_dir + app_dir) + 1729 * 3 * zk

    # Verifier (one key track)
    fwd_ver = 8 * comb + 12 + 2 + 8 + 16 + 8 + 2 * comb
    app_ver = 2 * comb + 2
    ver_sub = sboxes * (fwd_ver + app_ver) + 576 * (2 + zk) + 1 + zk
    ver_fix = sboxes * (fwd_ver + app_ver) + 1 + 1729 * zk

    print("   prover, submitted            %9.0f" % prov_sub)
    print("   prover, corrected (minimal)  %9.0f  x%.3f  +%d" %
          (prov_min, prov_min / prov_sub, round(prov_min - prov_sub)))
    print("   prover, corrected (direct)   %9.0f  x%.3f  +%d" %
          (prov_dir, prov_dir / prov_sub, round(prov_dir - prov_sub)))
    print("   verifier, submitted          %9.0f" % ver_sub)
    print("   verifier, corrected          %9.0f  x%.3f" %
          (ver_fix, ver_fix / ver_sub))

    # Share of the prover in signing.
    # Optimized build: Fig. 9 (physical p. 120), ZKP share of running time.
    zkp_opt = {"512s": 0.02314, "512f": 0.14698}
    # Reference build: Table 19 (physical p. 118), Mcyc. If the whole gap
    # between signing and verification is the prover's extra multiplications,
    # the gap bounds the cost of one multiplication from above.
    sign = {"512s": 6329.695, "512f": 1210.835}
    verify = {"512s": 6236.407, "512f": 1100.714}
    gap_mults = prov_sub - ver_sub
    print("   prover minus verifier        %9.0f multiplications" % gap_mults)
    for g in ("512f", "512s"):
        cyc = (sign[g] - verify[g]) * 1e6 / gap_mults
        print("   reference build, %s gap: at most %.0f cycles per multiplication"
              % (g, cyc))
        for s_ in ("512s", "512f"):
            ref_share = prov_sub * cyc / (sign[s_] * 1e6)
            print("     %s: prover share of signing %.1f%% (Fig. 9) to at most %.1f%%"
                  % (s_, 100 * zkp_opt[s_], 100 * ref_share))
            for label, fixed in (("minimal", prov_min), ("direct", prov_dir)):
                r = fixed / prov_sub - 1.0
                print("          signing, %-7s: +%.1f%% to +%.1f%%"
                      % (label, 100 * r * zkp_opt[s_], 100 * r * ref_share))


if __name__ == "__main__":
    sizes_and_hash_bytes()
    vistrutith_multiplications()
