#!/usr/bin/env python3
"""Fixing-cost arithmetic for kem-34-5 (Rudraksh2-512-I).  Standard library only.

Prints (1) the size formulas checked against Table 2 and evaluated at the
costed parameters, (2) a Gaussian failure-probability model checked against
the six values recorded in the submission and evaluated at the costed
parameters, and (3) XOF and multiplication counts of the submitted reference
code at module rank 8 and 9.  It implements no part of the scheme, and the
parameter values are costing aids, not recommended parameters.

Run:  python3 -B kem-34/fixing_cost_params.py
"""

import csv
import math
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
RECORDED = (HERE / "Implementations/Additional_Implementations/Security_and_DFR_Analysis"
            / "DFR_Computation/results/rudraksh_selected_parameters.csv")
# log2 of the DFR column of that file, keyed by (K, N, Q, ETA1), for checkouts
# that do not include the submission's result files.
RECORDED_LOG2 = {(9, 64, 3329, 2): -100.324, (9, 64, 4001, 1): -100.069,
                 (9, 128, 3329, 1): -161.328, (9, 128, 4001, 1): -167.269,
                 (9, 256, 4001, 1): -160.695, (8, 256, 7681, 2): -160.497}


# ---- 1. sizes (Algorithms 1, 10, 11, 13; lwekem512/params.h:40-55) ----------

def sizes(l, n, logq, logp, logt, lenk):
    pk = lenk // 8 + l * n * logq // 8
    ct = l * n * logp // 8 + n * logt // 8
    sk = l * n * logq // 8 + pk + 2 * (lenk // 8)
    return pk, sk, ct


def pct(new, old):
    return 100.0 * (new - old) / old


# ---- 2. failure-probability model ------------------------------------------

def compress_error(q, mod):
    """Distribution of Decompress(Compress(x)) - x over x in Z_q (spec p. 6)."""
    c = Counter()
    for x in range(q):
        y = ((2 * mod * x + q) // (2 * q)) % mod
        z = (2 * q * y + mod) // (2 * mod)
        d = (z - x) % q
        if d > q // 2:
            d -= q
        c[d] += 1
    return {d: k / q for d, k in c.items()}


def variance(dist):
    m = sum(d * p for d, p in dist.items())
    return sum(d * d * p for d, p in dist.items()) - m * m


def ln_q(x):
    """Natural log of the Gaussian tail Q(x)."""
    if x < 30:
        return math.log(math.erfc(x / math.sqrt(2)) / 2)
    return -(x * x) / 2 - math.log(x * math.sqrt(2 * math.pi))


def log_sum_exp(a):
    m = max(a)
    return m + math.log(sum(math.exp(v - m) for v in a))


def failure_log2(l, n, q, logp, logt, eta, beta):
    """log2 of the union bound on the decryption failure probability.

    Noise on one coefficient pair (spec p. 13): <e,r> - <s,e1+du> + e2 + dv.
    The products and e2 are taken as Gaussian with variance
    l*n*(s2*s2 + s2*(s2 + var(du))) + s2, s2 = eta/2; dv is the exact
    v-compression error.  Failure is leaving the half-distance to a
    neighbouring codeword of the B2 Minal code (Algorithm 6), summed over
    neighbours within 1.5 minimum distances, averaged over the 16 codewords
    and multiplied by the n/2 coefficient pairs.
    """
    s2 = eta / 2
    du = compress_error(q, 1 << logp)
    dv = sorted(compress_error(q, 1 << logt).items())
    sd = math.sqrt(l * n * (s2 * s2 + s2 * (s2 + variance(du))) + s2)
    a, h = round(q / 4), round(q / 2)
    cw = []
    for m in range(16):
        m0, m1, m2, m3 = [(m >> i) & 1 for i in range(4)]
        cw.append(((h * m0 + a * m2 + beta * m3) % q, (h * m1 + beta * m2 + a * m3) % q))
    nb = []
    for i in range(16):
        for j in range(16):
            if i != j:
                for zx in (-1, 0, 1):
                    for zy in (-1, 0, 1):
                        dx = cw[j][0] - cw[i][0] + zx * q
                        dy = cw[j][1] - cw[i][1] + zy * q
                        nb.append((math.hypot(dx, dy), dx, dy))
    dmin = min(t[0] for t in nb)
    terms = []
    for d, dx, dy in nb:
        if d <= 1.5 * dmin:
            for ex, px in dv:
                for ey, py in dv:
                    shift = (ex * dx + ey * dy) / d
                    terms.append(math.log(px * py) + ln_q((d / 2 - shift) / sd))
    return (log_sum_exp(terms) - math.log(16) + math.log(n / 2)) / math.log(2), dmin / 2, sd


# ---- 3. work counts of the submitted reference code ------------------------

def work(l, n, logq, eta, lenk):
    """XOF output bytes per operation, as lwekem512 calls pseudoXOF.

    gen_matrix asks for LOG2Q*KEM_N bytes per matrix entry (indcpa.c:211,234),
    each CBD polynomial for KEM_ETA*KEM_N/4 bytes (indcpa.c:297,303,309,360-378),
    G returns 2*lenK/8 and H lenK/8 bytes (symmetric.h:12-15).
    """
    kb = lenk // 8
    matrix = l * l * logq * n
    cbd = eta * n // 4
    keygen = 2 * kb + matrix + 2 * l * cbd + kb            # G, A, s and e, H(pk)
    encaps = kb + 2 * kb + matrix + (2 * l + 1) * cbd      # H(pk), G, A^T, s' e' e''
    decaps = 2 * kb + matrix + (2 * l + 1) * cbd + kb      # G, re-encryption, H(c||z)
    mults = {"matrix XOF calls": l * l, "pointwise products keygen": l * l,
             "pointwise products encaps": l * l + l, "forward NTTs keygen": 2 * l}
    return {"keygen": keygen, "encaps": encaps, "decaps": decaps, "matrix": matrix}, mults


def is_prime(x):
    return x > 1 and all(x % d for d in range(2, int(math.isqrt(x)) + 1))


def main():
    print("0. Primes q = 1 mod 2n below 2^14 for n = 256 (full negacyclic NTT, spec p. 5):",
          [q for q in range(513, 1 << 14, 512) if is_prime(q)])
    print()
    print("1. Sizes in bytes: pk = lenK/8 + l*n*ceil(log2 q)/8, ct = l*n*log2(p)/8 + n*log2(t)/8,")
    print("   sk = l*n*ceil(log2 q)/8 + pk + 2*lenK/8")
    table2 = (("Rudraksh2-128-I", (9, 64, 12, 12, 6, 128), (880, 1776, 912)),
              ("Rudraksh2-256-I", (9, 128, 12, 11, 9, 256), (1760, 3552, 1728)),
              ("Rudraksh2-512-I", (8, 256, 13, 13, 7, 512), (3392, 6848, 3552)))
    for name, par, want in table2:
        got = sizes(*par)
        print(f"   control {name}: pk {got[0]} sk {got[1]} ct {got[2]}  Table 2: {want}  "
              f"{'ok' if got == want else 'MISMATCH'}")
    base = sizes(8, 256, 13, 13, 7, 512)
    for label, par in (("l=9 q=7681 p=2^13 t=2^7", (9, 256, 13, 13, 7, 512)),
                       ("l=9 q=7681 p=2^12 t=2^7", (9, 256, 13, 12, 7, 512)),
                       ("l=9 q=7681 p=2^11 t=2^7", (9, 256, 13, 11, 7, 512)),
                       ("l=9 q=7681 p=2^13 t=2^13", (9, 256, 13, 13, 13, 512)),
                       ("l=9 14-bit q, p=2^14 t=2^7", (9, 256, 14, 14, 7, 512)),
                       ("l=8 14-bit q, p=2^14 t=2^7", (8, 256, 14, 14, 7, 512)),
                       ("l=9 q=4001 p=2^12 t=2^8 (512-II)", (9, 256, 12, 12, 8, 512))):
        pk, sk, ct = sizes(*par)
        print(f"   {label}: pk {pk} ({pct(pk, base[0]):+.2f}%)  ct {ct} ({pct(ct, base[2]):+.2f}%)  "
              f"sk {sk} ({pct(sk, base[1]):+.2f}%)")

    print("\n2. Failure-probability model, log2 (controls against the submission's recorded values)")
    recorded = dict(RECORDED_LOG2)
    if RECORDED.exists():
        with open(RECORDED, newline="") as fh:
            for row in csv.DictReader(fh):
                key = (int(row["K"]), int(row["N"]), int(row["Q"]), int(row["ETA1"]))
                recorded[key] = math.log2(float(row["DFR"]))
    controls = (("128-I", 9, 64, 3329, 12, 6, 2, 220), ("128-II", 9, 64, 4001, 9, 7, 1, 270),
                ("256-I", 9, 128, 3329, 11, 9, 1, 220), ("256-II", 9, 128, 4001, 11, 5, 1, 250),
                ("512-I", 8, 256, 7681, 13, 7, 2, 510), ("512-II", 9, 256, 4001, 12, 8, 1, 270))
    for name, l, n, q, lp, lt, eta, beta in controls:
        lg, half, sd = failure_log2(l, n, q, lp, lt, eta, beta)
        rec = recorded.get((l, n, q, eta))
        rec_s = f"{rec:.1f}" if rec is not None else "not found"
        print(f"   control {name}: model {lg:.1f}  recorded {rec_s}  (half distance {half:.1f}, sd {sd:.1f})")
    print("   (beta = 510 at q = 7681 as submitted, scaled by q/7681 at the 14-bit primes;")
    print("    'v uncompressed' sets t to the full coefficient width)")
    cases = [("l=8 q=7681 eta=3", 8, 7681, 13, 7, 3)]
    for q in (10753, 11777, 12289, 13313, 15361):
        for eta in (3, 4, 5):
            cases.append((f"l=8 q={q} eta={eta}", 8, q, 14, 7, eta))
    cases += [("l=9 q=7681 eta=2", 9, 7681, 13, 7, 2),
              ("l=9 q=7681 eta=1", 9, 7681, 13, 7, 1),
              ("l=9 q=7681 eta=1 p=2^12", 9, 7681, 12, 7, 1),
              ("l=9 q=7681 eta=1 p=2^11", 9, 7681, 11, 7, 1),
              ("l=9 q=7681 eta=1 p=2^10", 9, 7681, 10, 7, 1)]
    for label, l, q, lp, lt, eta in cases:
        beta = round(510 * q / 7681)
        lg = failure_log2(l, 256, q, lp, lt, eta, beta)[0]
        lg_u = failure_log2(l, 256, q, lp, q.bit_length(), eta, beta)[0]
        print(f"   {label}: t=2^{lt} {lg:.1f}, v uncompressed {lg_u:.1f}  "
              f"({'within' if lg <= -160 else 'above'} the 2^-160 bound at t=2^{lt})")

    print("\n3. XOF output bytes per operation, reference lwekem512 call pattern")
    b, bm = work(8, 256, 13, 2, 512)
    print(f"   submitted l=8 eta=2 q=7681: keygen {b['keygen']}  encaps {b['encaps']}  decaps {b['decaps']};  "
          f"matrix sampling {b['matrix']} = {100.0 * b['matrix'] / b['keygen']:.1f}% of keygen")
    for label, par in (("l=9 eta=1 q=7681", (9, 256, 13, 1, 512)),
                       ("l=9 eta=2 q=7681", (9, 256, 13, 2, 512)),
                       ("l=9 eta=1 q=4001 (512-II shape, same call pattern)", (9, 256, 12, 1, 512))):
        w, wm = work(*par)
        print(f"   {label}: " + "  ".join(
            f"{op} {w[op]} ({pct(w[op], b[op]):+.1f}%)" for op in ("keygen", "encaps", "decaps"))
            + f"  matrix {w['matrix']} ({pct(w['matrix'], b['matrix']):+.1f}%)")
    _, wm = work(9, 256, 13, 1, 512)
    print("   counts at l=8 -> l=9: "
          + "  ".join(f"{k} {bm[k]} -> {wm[k]} ({pct(wm[k], bm[k]):+.1f}%)" for k in bm))

if __name__ == "__main__":
    main()
