#!/usr/bin/env python3
"""sign-21-1 and sign-21-2: where a node or leaf index fits in the TCCR and
PRG inputs of ReSolveD-alpha, and how many calls share one tweak.

Prints, per parameter set, the cipher key and block bits that TCCR and the
LeafHash PRG leave free (spec 4.9.8 and 4.9.9, physical pp. 50-51; ciphers
of Table 6, physical p. 57), the tree size L from the submitted parameters (instances.c),
the index width, the cipher calls of the tree and of LeafHash (unchanged by
an index), and the expected number of tweak values shared by three, four or
five tree positions when the index is XORed into the 120-bit iv.

Costing aid only: implements no part of the scheme.
Run from the repository root: python3 -B sign-21/fixing_cost_tweak_encoding.py
"""
import sys

sys.dont_write_bytecode = True

import math
import os
import re

HEADER = "sign-21/Implementations/Reference_Implementation/ReSolveD-alpha-256s/parameters.h"
PREFIX = "RESOLVED_ALPHA"
IV_BITS = 120
# Table 6 (physical p. 57): (cipher, n_k, n_b) per lambda.
CIPHERS = {160: ("AES-192", 192, 128), 256: ("AES-256", 256, 128),
           384: ("Rijndael-256", 256, 256), 512: ("SHACAL-2", 512, 256)}


def read_params(path, prefix):
    text = open(path, encoding="utf-8").read()
    sets = {}
    for name, field, val in re.findall(
            r"#define\s+%s_(\d+[SF])_(CSP|POW_LEVEL|TAU)\s+(\d+)" % prefix, text):
        sets.setdefault(name, {})[field] = int(val)
    return sets


def log2_choose(n_log2, r):
    """log2 of n^r / r!, the expected r-way coincidences numerator."""
    return r * n_log2 - math.log2(math.factorial(r))


def main():
    if not os.path.exists(HEADER):
        sys.exit("run from the repository root")
    sets = read_params(HEADER, PREFIX)
    order = sorted(sets, key=lambda s: (int(s[:-1]), s[-1] == "F"))
    print("set   lam cipher        nk  nb | TCCR key pad, spare in block byte | PRG key pad, block pad |"
          "     L  2L  j | TCCR evals, cipher calls | LeafHash calls | fits?")
    for s in order:
        p = sets[s]
        lam, w, tau = p["CSP"], p["POW_LEVEL"], p["TAU"]
        name, nk, nb = CIPHERS[lam]
        k = (lam - w) // tau + 1                    # instances.c CALC_K
        tau1 = (lam - w) % tau
        tau0 = tau - tau1
        L = tau1 * (1 << k) + tau0 * (1 << (k - 1))  # instances.c CALC_L
        j = math.ceil(math.log2(2 * L))             # node ids in [1, 2L)
        nblock = math.ceil(lam / nb)
        tccr_pad = nk + nb - 128 - lam              # 4.9.8 line 10
        spare_i = 8 - math.ceil(math.log2(nblock)) if nblock > 1 else 8
        if lam == 384:                              # PRG built from TCCR
            prg_key_pad, prg_blk_pad = None, None
            leaf_calls = math.ceil(2 * lam / lam) * nblock
        else:
            prg_key_pad, prg_blk_pad = nk - lam, nb - 128
            leaf_calls = math.ceil(2 * lam / nb)
        evals = (L // 2 - 1) + 2 * (L // 2)         # BAVC.Commit lines 3-10
        if lam == 384:
            prg_fit = tccr_pad >= j
        else:
            prg_fit = max(prg_key_pad, prg_blk_pad) >= j
        fits = "yes" if tccr_pad >= j and prg_fit else "no"
        prg_txt = ("via TCCR" if prg_key_pad is None
                   else "%3d, %3d" % (prg_key_pad, prg_blk_pad))
        print("%-5s %3d %-13s %3d %3d | %3d, %d                              | %-22s | %6d %6d %2d |"
              " %6d, %6d            | %d per leaf      | %s"
              % (s, lam, name, nk, nb, tccr_pad, spare_i, prg_txt, L, 2 * L, j,
                 evals, evals * nblock, leaf_calls, fits))
    print()
    print("Shared tweaks when tweak = iv XOR IntToBits(id; 120), id in [1, 2L), one tweak per tree"
          " position (a bottom node makes 2 TCCR queries on it):")
    print("log2 of the expected number of tweak values shared by 3, 4 or 5 positions, over Q signatures;"
          " for iv || id, by 3 signatures")
    for s in order:
        p = sets[s]
        lam, w, tau = p["CSP"], p["POW_LEVEL"], p["TAU"]
        k = (lam - w) // tau + 1
        tau1 = (lam - w) % tau
        L = tau1 * (1 << k) + (tau - tau1) * (1 << (k - 1))
        row = []
        for qlog in (64, 80):
            n_log2 = qlog + math.log2(2 * L - 1)
            t3 = log2_choose(n_log2, 3) - 2 * IV_BITS
            t4 = log2_choose(n_log2, 4) - 3 * IV_BITS
            t5 = log2_choose(n_log2, 5) - 4 * IV_BITS
            c3 = log2_choose(qlog, 3) - 2 * IV_BITS
            row.append("Q=2^%d: XOR 3-way %+6.1f, 4-way %+6.1f, 5-way %+6.1f; concat 3-way %+6.1f"
                       % (qlog, t3, t4, t5, c3))
        print("%-5s %s" % (s, " | ".join(row)))
    print()
    print("Submitted: every TCCR query of one signature shares the tweak iv, so mu = 3L/2 - 1"
          " queries per signature.")


if __name__ == "__main__":
    main()
