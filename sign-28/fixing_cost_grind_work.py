#!/usr/bin/env python3
"""Fixing-cost arithmetic for sign-28-1 (SYDO grinding).

Counts, per parameter set, the expected Hash3 (chall3) calls of the grinding
loop when wgrind-2 bits are enforced (submitted code) and when wgrind bits are
enforced (Algorithm 2 with a chall3 two bits longer), and sets the added
hashing against the other symmetric-primitive work of one signature.  It also
prints, for comparison, the work added when the two bits go to Delta instead
(two trees of twice the size) and the grinding stays as submitted.

Nothing is measured and no part of the scheme is implemented.  Inputs:
  specification Table 5.2 (physical p. 35), Table 5.3 (p. 36), Table 5.5 (p. 37);
  reference source sydo_512s/src (identical layout code in all six sets):
    internal.h:65-169   lengths (delta bits, challenge, proof, rows, layout)
    sydo.c:43-48        parameter table
    sydo.c:736-818      XOF calls of one signature
    vole_commit_impl.inc:53-98   leaf-commitment hashing
    bavc_impl.inc:154-240, primitives.c:236-300   tree and leaf PRG calls
    vole_impl.inc:40-46,220-290  VOLE tape PRG calls per leaf
    quicksilver.c:587-687        packed-H expansion (3n elements)
    lib/auxfunc.c:482-529        pseudoXOF = one SM3 pass per 256 output bits

Run: python3 -B sign-28/fixing_cost_grind_work.py
"""

from math import ceil

D = 4  # QuickSilver degree, SYDO_REF_RSD_QS_DEGREE

# name: (lambda, tau, log2 N, wgrind, Topen, n, w, n-k, |sig| Table 5.4,
#        Pr[BAVC.Open != bot] Table 5.5, PRG block bytes, PRG blocks per
#        packed-H element, sign Mcyc Table 8.3)
SETS = {
    "160s": (160, 14, 11, 8, 132, 16461, 59, 480, 5428, 3.09e-2, 24, 2, 708.516),
    "160f": (160, 20, 8, 2, 138, 16461, 59, 480, 6724, 1.91e-1, 24, 2, 274.576),
    "256s": (256, 23, 11, 5, 225, 26226, 94, 768, 14444, 1.06e-1, 32, 2, 1287.649),
    "256f": (256, 32, 8, 2, 236, 26226, 94, 768, 17604, 8.82e-1, 32, 2, 542.629),
    "512s": (512, 46, 11, 8, 445, 49941, 179, 1456, 56672, 1.47e-2, 64, 1, 3758.104),
    "512f": (512, 64, 8, 2, 446, 49941, 179, 1456, 67716, 1.54e-1, 64, 1, 1723.787),
}


def sm3_blocks(nbytes):
    """SM3 compression calls for one pass over nbytes (pad: 1 + 8 bytes)."""
    return ceil((nbytes + 9) / 64)


def xof_sm3(in_bytes, out_bits):
    """Compression calls of pseudoXOF: one SM3(msg || ct) per 256 output bits."""
    return ceil(out_bits / 256) * sm3_blocks(in_bytes + 4)


def main():
    print("sign-28-1 grinding cost; counts per signature")
    for name, (lam, tau, k, wg, topen, n, w, codim, sig_spec, p_open,
               blk, h_blocks, mcyc) in SETS.items():
        lb = lam // 8
        # --- sizes (control against Table 5.4) -----------------------------
        wit_bits = ceil(10 * w / 8) * 8
        rows = wit_bits + (D - 1) * lam + 8 * (lb + 2)
        commit = (rows // 8) * (tau - 1)
        sig = (commit + (lb + 2) + wit_bits // 8 + (D - 1) * lb
               + tau * 2 * lb + topen * lb + lb + lb + 4)
        assert sig == sig_spec, (name, sig, sig_spec)
        assert tau * k + wg == lam + 2          # Table 5.3 takes equality
        sig_rep = sig + 1                       # chall3 of lam+2 bits, one more byte

        # --- grinding loop -------------------------------------------------
        calls_code = 2 ** (wg - 2) / p_open     # wgrind-2 bits enforced
        calls_spec = 2 ** wg / p_open           # wgrind bits enforced
        added = calls_spec - calls_code
        chal2 = (6 * lam + 64) // 8
        g_in = chal2 + lb + (D - 1) * lb + 4 + 1
        g_sm3 = xof_sm3(g_in, lam)

        # --- other XOF work ------------------------------------------------
        leaves = tau * 2 ** k
        leaf_in = 2 * lb * 2 ** k + 1
        chal1_in = 2 * lb + 2 * lb + commit + lb + 1
        chal2_in = (5 * lam + 64) // 8 + (lam + 1) * (lb + 2) + wit_bits // 8 + 1
        xof_other_bytes = tau * leaf_in + (tau * 2 * lb + 1) + chal1_in + chal2_in
        xof_other_sm3 = (tau * xof_sm3(leaf_in, 2 * lam)
                         + xof_sm3(tau * 2 * lb + 1, 2 * lam)
                         + xof_sm3(chal1_in, 5 * lam + 64)
                         + xof_sm3(chal2_in, 6 * lam + 64))

        # --- PRG block calls -----------------------------------------------
        parts = lb // 16 if (lb > 16 and lb % 16 == 0) else 1
        per_leaf = ceil(ceil(rows / 128) / parts)
        prg_calls = 2 * (leaves - 1) + 2 * leaves + leaves * per_leaf
        prg_bytes = prg_calls * blk
        h_calls = 3 * n * h_blocks
        h_bytes = h_calls * (64 if lam == 512 else 16)

        base_xof = xof_other_bytes + calls_code * g_in
        add_xof = added * g_in
        print(f"\nSYDO-{name}: lambda={lam} tau={tau} N=2^{k} wgrind={wg} "
              f"leaves={leaves}")
        print(f"  signature bytes: formula {sig} (Table 5.4: {sig_spec}); "
              f"with a {lam + 2}-bit chall3: {sig_rep} "
              f"(+{100 * (sig_rep - sig) / sig:.3f}%)")
        print(f"  expected Hash3 calls: code {calls_code:.1f}, "
              f"wgrind enforced {calls_spec:.1f}, added {added:.1f}")
        print(f"  Hash3 input {g_in} bytes; pseudoXOF = {g_sm3} SM3 "
              f"compression calls per attempt")
        print(f"  XOF bytes absorbed: other {xof_other_bytes}, grinding code "
              f"{calls_code * g_in:.0f}, added {add_xof:.0f} "
              f"(+{100 * add_xof / base_xof:.1f}% of XOF bytes)")
        print(f"  SM3 compression calls as submitted: other {xof_other_sm3}, "
              f"grinding code {calls_code * g_sm3:.0f}, "
              f"added {added * g_sm3:.0f} "
              f"(+{100 * added * g_sm3 / (xof_other_sm3 + calls_code * g_sm3):.1f}%)")
        print(f"  PRG block calls: tree+leaf+VOLE {prg_calls} ({prg_bytes} bytes), "
              f"packed H {h_calls} ({h_bytes} bytes)")
        for r in (1, 2, 3):
            imp = add_xof * r / (base_xof * r + prg_bytes + h_bytes)
            print(f"  signing estimate, XOF byte = {r} PRG bytes, field "
                  f"arithmetic not counted: +{100 * imp:.1f}%")
        calls_all = xof_other_sm3 + calls_code * g_sm3 + prg_calls + h_calls
        print(f"  signing estimate, one SM3 compression = one PRG block call: "
              f"+{100 * added * g_sm3 / calls_all:.1f}%")
        print(f"  context: Table 8.3 reference signing {mcyc} Mcyc; added SM3 "
              f"compression calls per Mcyc of baseline: "
              f"{added * g_sm3 / mcyc:.1f}")

        # --- alternative, not the costed repair: the two bits go to Delta ---
        # Two trees get 2^(k+1) leaves; grinding stays as submitted.  Needs
        # Delta bits = lambda - wgrind + 4 <= lambda, so wgrind >= 4.
        if wg >= 4:
            alt_leaves = 2 * 2 ** k
            alt_prg_calls = alt_leaves * (4 + per_leaf)
            alt_prg_bytes = alt_prg_calls * blk
            alt_xof_bytes = 2 * (2 * lb * 2 ** k)
            alt_sm3 = 2 * (xof_sm3(2 * lb * 2 ** (k + 1) + 1, 2 * lam)
                           - xof_sm3(leaf_in, 2 * lam))
            alt = [100 * (alt_xof_bytes * r + alt_prg_bytes)
                   / (base_xof * r + prg_bytes + h_bytes) for r in (1, 2, 3)]
            alt4 = 100 * (alt_sm3 + alt_prg_calls) / calls_all
            print(f"  alternative, two trees of 2^{k + 1} leaves and the "
                  f"submitted grinding: leaves +{100 * alt_leaves / leaves:.1f}%"
                  f"; same four models: +{alt[0]:.1f}%, +{alt[1]:.1f}%, "
                  f"+{alt[2]:.1f}%, +{alt4:.1f}%")


if __name__ == "__main__":
    main()
