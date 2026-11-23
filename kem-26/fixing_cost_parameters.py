#!/usr/bin/env python3
"""Fixing-cost estimate for kem-26 (NSS-HQC): weights, failure model, sizes.

Stdlib only. Prints numbers; implements no part of the scheme. The values it
finds are costing aids, not recommended parameters.

1. Sizes. Reproduces Table 15 (physical p. 46) from Eqs. (95)-(96).

2. Weights. report.md gives two Stern bit costs at each ring length n, for the
   ephemeral weight 2*w_r (kem-26-3) and the key weight 2*w_sk (kem-26-7).
   The cost is taken as linear in the weight between and beyond them. The
   ephemeral instance keeps the sqrt(n) discount of kem-26-3; the key
   instance is taken without it, as in kem-26-7, and only at the 384- and
   512-bit sets that kem-26-7 covers.

3. Failure model, per Reed-Muller coordinate (5 repeated bits):
   - structural bit error rate p_struct from Eq. (63) with the product terms
     of the bullet below it (physical p. 28), bits independent;
   - the ciphertext symbol fixes only the majority of the 5 masked bits
     (Eqs. 23, 25; nss_hqc_core.c:615-650), and the mask s*r2 is dense, so a
     group holds 0, 1 or 2 flipped bits with probability 2/32, 10/32, 20/32;
   - the inner decoder correlates the soft sums with all codewords
     (code_layer.c:213-250). The 127 other Walsh coefficients are treated as
     independent of the correct one, ties count one half;
   - outer code, two variants of Eq. (67): "errors" (no erasures, the
     specification's fallback) and "erasures" (blocks under an amplitude
     threshold erased, no cap, threshold chosen to minimise the failure rate).
   Blocks are independent. Target 2^-128, the bound of section 3.6.

4. Search. Smallest ciphertext over ring lengths n' (the submitted n, or
   640*m) and outer lengths n1 <= min(n'/640, 255), then n' moved up to the
   next prime with 2 as a primitive root.

5. Work. Inner-loop steps of the submitted code: w*n per sparse product
   (nss_hqc_core.c:585-604; one in key generation, two in encryption, one in
   decryption) and 256*128*5 per inner block decoded (code_layer.c:225-235).
"""

from math import ceil, exp, lgamma, log, log2

SETS = {  # Tables 1 and 3, physical p. 26
    "NSS-HQC-128": dict(level=128, n=29443, w_sk=73, w_r=66, w_e=117, n1=46, k1=16, lam=256),
    "NSS-HQC-256": dict(level=256, n=54493, w_sk=117, w_r=106, w_e=187, n1=70, k1=32, lam=256),
    "NSS-HQC-384": dict(level=384, n=122579, w_sk=175, w_r=159, w_e=279, n1=100, k1=48, lam=384),
    "NSS-HQC-512": dict(level=512, n=217901, w_sk=233, w_r=213, w_e=372, n1=130, k1=64, lam=512),
}
TABLE_15 = {  # public key, KEM ciphertext
    "NSS-HQC-128": (3713, 5185), "NSS-HQC-256": (6844, 9084),
    "NSS-HQC-384": (15371, 18571), "NSS-HQC-512": (27302, 31462),
}
STERN = {  # report.md: raw Stern bit cost at weight 2*w_r and at weight 2*w_sk
    "NSS-HQC-128": (154.43, 167.887), "NSS-HQC-256": (234.38, 255.868),
    "NSS-HQC-384": (341.49, 372.995), "NSS-HQC-512": (450.26, 489.799),
}
TARGET = -128.0
MULT, RM_LEN = 5, 128
GROUP_FLIPS = {0: 2 / 32, 1: 10 / 32, 2: 20 / 32}


def sizes(n, n1, lam):
    pk = lam // 8 + ceil(n / 8)                       # Eq. (95)
    ct = ceil(n / 8) + ceil(n1 * RM_LEN * 2 / 8) + lam // 8   # Eq. (96) + salt
    return pk, ct


def stern(name, weight):
    p = SETS[name]
    e, k = STERN[name]
    slope = (k - e) / (2 * p["w_sk"] - 2 * p["w_r"])
    return e + slope * (weight - 2 * p["w_r"]), slope


def min_weight(name, start, need):
    w = start
    while stern(name, 2 * w)[0] < need:
        w += 1
    return w


def is_primitive_prime(p):
    if p < 3 or p % 2 == 0:
        return False
    d = 3
    while d * d <= p:
        if p % d == 0:
            return False
        d += 2
    m, f, factors = p - 1, 2, set()
    while f * f <= m:
        while m % f == 0:
            factors.add(f)
            m //= f
        f += 1
    if m > 1:
        factors.add(m)
    return all(pow(2, (p - 1) // q, p) != 1 for q in factors)


def next_primitive_prime(n):
    while not is_primitive_prime(n):
        n += 1
    return n


def p_struct(n, w_sk, w_r, w_e):
    prod = 0.5 * (1 - (1 - 2 / n) ** (w_sk * w_r))
    return 0.5 * (1 - (1 - 2 * prod) ** 2 * (1 - 2 * w_e / n))


def binom(n, k):
    return exp(lgamma(n + 1) - lgamma(k + 1) - lgamma(n - k + 1))


def soft_distribution(ps, compressed=True):
    """P[soft sum = 5 - 2j], j = wrong bits among the 5 copies."""
    flips = GROUP_FLIPS if compressed else {0: 1.0}
    out = [0.0] * (MULT + 1)
    for k, pk in flips.items():
        for a in range(k + 1):               # flipped bits flipped back
            for b in range(MULT - k + 1):    # further bits flipped
                pr = (pk * binom(k, a) * ps ** a * (1 - ps) ** (k - a)
                      * binom(MULT - k, b) * ps ** b * (1 - ps) ** (MULT - k - b))
                out[k - a + b] += pr
    return out[::-1]                         # index i <-> soft sum 2i - 5


def convolve(a, b):
    out = [0.0] * (len(a) + len(b) - 1)
    for i, x in enumerate(a):
        if x:
            for j, y in enumerate(b):
                out[i + j] += x * y
    return out


def block_tables(ps, compressed=True):
    half = soft_distribution(ps, compressed)
    for _ in range(6):                       # sum over 64 coordinates
        half = convolve(half, half)
    top = MULT * RM_LEN // 2                 # 320
    right = convolve(half, half)             # correct coefficient, 2i - 640
    other = convolve(half, half[::-1])       # any other coefficient, 2i - 640
    abs_other = [0.0] * (top + 1)
    abs_right = [0.0] * (top + 1)
    for i in range(2 * top + 1):
        abs_other[abs(i - top)] += other[i]
        abs_right[abs(i - top)] += right[i]
    below = [0.0] * (top + 2)                # below[j] = P[|other| < 2j]
    for j in range(top + 1):
        below[j + 1] = below[j] + abs_other[j]
    good = [0.0] * (top + 1)                 # correct, with amplitude 2j
    for j in range(1, top + 1):
        good[j] = right[top + j] * (below[j] + 0.5 * abs_other[j]) ** 127
    return good, below, abs_right


def block_rates(tables, thr):
    """(P wrong and kept, P erased) for amplitude threshold 2*thr."""
    good, below, abs_right = tables
    if thr == 0:
        return 1 - sum(good), 0.0
    erased = sum(abs_right[:thr]) * below[thr] ** 127
    return max(0.0, 1 - erased - sum(good[thr:])), erased


def log2_failure(n1, k1, p_err, p_era):
    """log2 P[2T + E > n1 - k1], (T, E) multinomial over n1 blocks."""
    red = n1 - k1
    total = 0.0
    lf = [lgamma(i + 1) for i in range(n1 + 1)]
    p_ok = 1 - p_err - p_era
    le, la, lo = log(p_err), (log(p_era) if p_era > 0 else 0.0), log(p_ok)
    for t in range(n1 + 1):
        e0 = max(0, red + 1 - 2 * t)
        if p_era == 0.0:
            if e0 == 0:
                total += exp(lf[n1] - lf[t] - lf[n1 - t] + t * le + (n1 - t) * lo)
            continue
        mode = (n1 - t) * p_era / (p_era + p_ok)
        part = 0.0
        for e in range(e0, n1 - t + 1):
            term = exp(lf[n1] - lf[t] - lf[e] - lf[n1 - t - e]
                       + t * le + e * la + (n1 - t - e) * lo)
            part += term
            if e > mode and term < 1e-18 * part:
                break
        total += part
    return log2(total) if total > 0 else -1e9


THRESHOLDS = tuple(range(20, 61, 4))         # amplitudes 40 .. 120


def failure(tables, n1, k1, decoder):
    p_err, _ = block_rates(tables, 0)
    best = (log2_failure(n1, k1, p_err, 0.0), 0)
    if decoder == "erasures":
        for thr in THRESHOLDS:
            p_err, p_era = block_rates(tables, thr)
            if p_err <= 0 or p_era >= 0.95:
                continue
            best = min(best, (log2_failure(n1, k1, p_err, p_era), thr))
    return best


def smallest_n1(tables, k1, cap, decoder):
    if failure(tables, cap, k1, decoder)[0] > TARGET:
        return None
    lo, hi = k1 + 1, cap
    while lo < hi:
        mid = (lo + hi) // 2
        if failure(tables, mid, k1, decoder)[0] <= TARGET:
            hi = mid
        else:
            lo = mid + 1
    return hi


def search(name, w_sk, w_r, decoder, compressed=True, grow=True):
    p = SETS[name]
    per_symbol = 2 if compressed else MULT   # bits per coordinate sent
    cache = {}

    def point(n2):
        if n2 not in cache:
            tables = block_tables(p_struct(n2, w_sk, w_r, p["w_e"]), compressed)
            n1 = smallest_n1(tables, p["k1"], min(n2 // 640, 255), decoder)
            ct = None if n1 is None else ceil(n2 / 8) + 16 * per_symbol * n1
            cache[n2] = (ct, n1)
        return cache[n2]

    best = None
    if point(p["n"])[0] is not None:
        best = (point(p["n"])[0], p["n"])
    if grow:
        m0 = p["n"] // 640 + 1
        coarse = None
        for m in range(m0, 4 * m0, 8):
            ct = point(640 * m)[0]
            if ct is None:
                continue
            if coarse is None or ct < coarse[0]:
                coarse = (ct, m)
            elif ct > coarse[0] * 1.03:
                break
        if coarse is not None:
            for m in range(max(m0, coarse[1] - 8), coarse[1] + 9):
                ct = point(640 * m)[0]
                if ct is not None and (best is None or ct < best[0]):
                    best = (ct, 640 * m)
    if best is None:
        return None
    n2 = best[1]
    n1 = point(n2)[1]
    if n2 != p["n"]:
        n2 = next_primitive_prime(n2)
    tables = block_tables(p_struct(n2, w_sk, w_r, p["w_e"]), compressed)
    return dict(n=n2, n1=n1, fail=failure(tables, n1, p["k1"], decoder),
                ps=p_struct(n2, w_sk, w_r, p["w_e"]), at_submitted_n=point(p["n"])[1])


def work(n, w_sk, w_r, n1):
    keygen = w_sk * n
    encaps = 2 * w_r * n
    decaps = (w_sk + 2 * w_r) * n + 256 * RM_LEN * MULT * n1
    return keygen, encaps, decaps


def pct(new, old):
    return f"{100 * (new / old - 1):+.1f}%"


def rs_trial_steps(rows, unknowns):
    return rows * (unknowns + 1) * (unknowns + 2) // 2


def main():
    print("1. Sizes (public key, ciphertext), Eqs. (95)-(96) against Table 15")
    for name, p in SETS.items():
        got = sizes(p["n"], p["n1"], p["lam"])
        print(f"   {name}: {got}  table {TABLE_15[name]}  match={got == TABLE_15[name]}")

    print("\n2. Weights (linear in the weight through the report's Stern costs)")
    repaired = {}
    for name, p in SETS.items():
        disc = 0.5 * log2(p["n"])
        slope = stern(name, 0)[1]
        w_r = min_weight(name, p["w_r"], p["level"] + disc)
        w_sk = p["w_sk"]
        if name in ("NSS-HQC-384", "NSS-HQC-512"):          # kem-26-7
            w_sk = min_weight(name, p["w_sk"], p["level"])
        w_sk_disc = min_weight(name, p["w_sk"], p["level"] + disc)
        repaired[name] = (w_sk, w_r)
        print(f"   {name}: slope {slope:.4f} bit per unit weight, discount {disc:.3f}; "
              f"w_r {p['w_r']} -> {w_r} ({pct(w_r, p['w_r'])}, "
              f"cost {stern(name, 2 * w_r)[0] - disc:.2f}); "
              f"w_sk {p['w_sk']} -> {w_sk} (raw cost {stern(name, 2 * w_sk)[0]:.2f}); "
              f"w_sk with the discount {w_sk_disc}")
    p = SETS["NSS-HQC-128"]
    print(f"   NSS-HQC-128, w_r 66 -> 67: Stern cost +{stern('NSS-HQC-128', 134)[0] - 154.43:.2f} bits, "
          f"encapsulation steps {pct(67, 66)}")

    print("\n3. Failure model at the submitted sets (log2 of the failure rate)")
    for name, p in SETS.items():
        ps = p_struct(p["n"], p["w_sk"], p["w_r"], p["w_e"])
        tables = block_tables(ps)
        p_blk = block_rates(tables, 0)[0]
        f_err = failure(tables, p["n1"], p["k1"], "errors")
        f_era = failure(tables, p["n1"], p["k1"], "erasures")
        print(f"   {name}: p_struct {ps:.4f}, inner block error {p_blk:.4f}, "
              f"errors {f_err[0]:.1f}, erasures {f_era[0]:.1f} (threshold {2 * f_era[1]})")
    print(f"   report.md, kem-26-2: 7 failures in 50,000 at NSS-HQC-256 = 2^{log2(7 / 50000):.1f}")

    print("\n4. Smallest ciphertext at failure rate 2^-128, compressor kept (b = 2)")
    for name in ("NSS-HQC-256", "NSS-HQC-384", "NSS-HQC-512"):
        p = SETS[name]
        w_sk, w_r = repaired[name]
        pk0, ct0 = TABLE_15[name]
        base = work(p["n"], p["w_sk"], p["w_r"], p["n1"])
        print(f"   {name}: w_sk {w_sk}, w_r {w_r}; steps at the submitted n and n1: "
              + ", ".join(f"{lab} {pct(x, y)}" for lab, x, y in
                          zip(("keygen", "encaps", "decaps"),
                              work(p["n"], w_sk, w_r, p["n1"]), base)))
        for decoder in ("errors", "erasures"):
            r = search(name, w_sk, w_r, decoder)
            if r is None:
                print(f"      {decoder}: no point found")
                continue
            pk, ct = sizes(r["n"], r["n1"], p["lam"])
            new = work(r["n"], w_sk, w_r, r["n1"])
            print(f"      {decoder:8s}: n1 that fits the submitted n: {r['at_submitted_n']}; "
                  f"n {r['n']} ({pct(r['n'], p['n'])}), n1 {r['n1']}, "
                  f"p_struct {r['ps']:.4f}, failure 2^{r['fail'][0]:.1f}; "
                  f"pk {pk} ({pct(pk, pk0)}), ct {ct} ({pct(ct, ct0)}); "
                  + ", ".join(f"{lab} {pct(x, y)}" for lab, x, y in
                              zip(("keygen", "encaps", "decaps"), new, base)))

    print("\n5. Alternative: second component sent uncompressed (5 bits per coordinate), n kept")
    for name in ("NSS-HQC-256", "NSS-HQC-384", "NSS-HQC-512"):
        p = SETS[name]
        w_sk, w_r = repaired[name]
        pk0, ct0 = TABLE_15[name]
        r = search(name, w_sk, w_r, "errors", compressed=False, grow=False)
        ct = ceil(p["n"] / 8) + 80 * r["n1"] + p["lam"] // 8
        ct_same = ceil(p["n"] / 8) + 80 * p["n1"] + p["lam"] // 8
        print(f"   {name}: n1 {r['n1']}, failure 2^{r['fail'][0]:.1f}, ct {ct} ({pct(ct, ct0)}); "
              f"with the submitted n1 = {p['n1']}: ct {ct_same} ({pct(ct_same, ct0)})")

    print("\n6. Outer decoder (kem-26-5): field multiplications against decapsulation steps")
    for name, p in SETS.items():
        red = p["n1"] - p["k1"]
        one = rs_trial_steps(p["n1"], p["k1"] + 2 * (red // 2))
        every = sum(rs_trial_steps(p["n1"], p["k1"] + 2 * e) for e in range(red // 2 + 1))
        dec = work(p["n"], p["w_sk"], p["w_r"], p["n1"])[2]
        print(f"   {name}: first trial {one} ({100 * one / dec:.2f}% of {dec}), "
              f"all {red // 2 + 1} trials {every} ({100 * every / dec:.1f}%)")


if __name__ == "__main__":
    main()
