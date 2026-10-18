#!/usr/bin/env python3
"""Verifier's independent PXL evaluation (Furue-Kudo, arXiv 2112.05023v2 = PQCrypto'24).

Paper conventions (read from the PDF text):
  D     = D1^(n-k) (eq. 3.3): min d>=2 with coeff_d[(1-z)^(m-(n-k)-1) (1+z)^m] <= 1   (affine, <=1)
  alpha = sum_{d<=D} max(coeff_d[(1-t)^(m-(n-k)) (1+t)^m], 0)                      (eq. 4.2, homogeneous)
  cost  = k^2 alpha C(n-k+D,D) C(n+D,D) + q^k (alpha^2 C(k+D,D) + alpha^w)        (eq. 4.8, field ops, no constants)
CE 2.1.1 convention: D = first non-positive coeff of the HOMOGENEOUS series (1-t^2)^m/(1-t)^(n-k)
  (= Dreg^(n-k), eq. 3.2), same alpha, cost = max(term1, term2) instead of the sum.
h-WXL paper eq. (2.7): q^k C(n-k,2) C(n-k+D,D)^2 with D = D0^(n-k) (eq. 2.8, coeff <= d)
h-WXL UOV spec p.24:   q^k 3 C(n-k+2,2) C(n-k+d,d)^2 (2r^2+r) bit ops, d = first non-pos coeff of (1-t^2)^M/(1-t)^(N+1)
Run from repository root: python3 -I UOV/analysis/pxl_verify.py
"""
from math import comb, log2

def series(a, b, dmax=400):
    """coefficients of (1-z)^a (1+z)^b, a may be negative."""
    if a >= 0:
        A = [(-1) ** i * comb(a, i) if i <= a else 0 for i in range(dmax + 1)]
    else:
        A = [comb(-a - 1 + i, i) for i in range(dmax + 1)]  # 1/(1-z)^(-a)
    B = [comb(b, i) if i <= b else 0 for i in range(dmax + 1)]
    return [sum(A[i] * B[d - i] for i in range(d + 1)) for d in range(dmax + 1)]

def D1(nk, m):
    s = series(m - nk - 1, m)
    return next(d for d in range(2, len(s)) if s[d] <= 1)

def D0(nk, m):
    s = series(m - nk - 1, m)
    return next(d for d in range(0, len(s)) if s[d] <= d)

def Dreg(nk, m):
    s = series(m - nk, m)
    return next(d for d in range(0, len(s)) if s[d] <= 0)

def alpha(nk, m, D):
    s = series(m - nk, m)
    return sum(max(s[i], 0) for i in range(D + 1))

def pxl(q, n, m, w, conv='paper', krange=None):
    best = None
    for k in (krange or range(1, n)):
        nk = n - k
        D = D1(nk, m) if conv == 'paper' else Dreg(nk, m)
        a = alpha(nk, m, D)
        t1 = k * k * a * comb(nk + D, D) * comb(n + D, D)
        tfix = q ** k * a * a * comb(k + D, D)
        tli2 = q ** k * a ** w
        c = log2(t1 + tfix + tli2) if conv == 'paper' else max(log2(t1), log2(tfix + tli2))
        rec = dict(cost=c, k=k, D=D, la=log2(a), t1=log2(t1), tfix=log2(tfix), tli2=log2(tli2),
                   mem_r5=log2(comb(k + D, D)) + 2 * log2(comb(nk + D, D)),
                   mem_a2=log2(a * a * comb(k + D, D)))
        if best is None or c < best['cost']:
            best = rec
    return best

def hwxl_paper(q, n, m):
    best = None
    for k in range(0, n):
        nk = n - k
        try:
            D = D0(nk, m)
        except StopIteration:
            continue
        c = k * log2(q) + log2(max(comb(nk, 2), 1)) + 2 * log2(comb(nk + D, D))
        if best is None or c < best[0]:
            best = (c, k, D)
    return best

def hwxl_spec(q, n, m):
    r = log2(q); best = None
    for k in range(0, n):
        nk = n - k
        # (1-t^2)^m/(1-t)^(nk+1) = (1-t)^(m-nk-1)(1+t)^m
        s = series(m - nk - 1, m)
        try:
            d = next(dd for dd in range(1, len(s)) if s[dd] <= 0)
        except StopIteration:
            continue
        c = k * r + log2(3 * comb(nk + 2, 2)) + 2 * log2(comb(d + nk, d)) + log2(2 * r * r + r)
        if best is None or c < best[0]:
            best = (c, k, d)
    return best

if __name__ == '__main__':
    print("== Calibration against Furue-Kudo Table 1 (n=m; field ops log2) ==")
    table1 = {256: {20: (62, 64, 3, 3, 9, 9, 14, 14, 75), 40: (117, 121, 6, 5, 14, 15, 27, 29, 129),
                    60: (169, 178, 8, 7, 19, 20, 42, 44, 182), 80: (220, 233, 10, 8, 24, 27, 56, 60, 234)},
              31: {20: (57, 57, 5, 5, 7, 7, 11, 11, 65), 40: (105, 107, 8, 8, 12, 12, 24, 24, 116),
                   60: (152, 158, 11, 10, 16, 17, 37, 38, 162), 80: (197, 208, 13, 12, 21, 22, 51, 53, 208)}}
    for q in (256, 31):
        for n, ref in table1[q].items():
            out = []
            for i, w in enumerate((2.37, 2.81)):
                b = pxl(q, n, n, w)
                c = pxl(q, n, n, w, conv='ce')
                out.append(f"w={w}: paper-conv {b['cost']:.1f} (k={b['k']},D={b['D']},a=2^{b['la']:.1f}) "
                           f"[Table1 {ref[i]} k={ref[2+i]} D={ref[4+i]} a=2^{ref[6+i]}] | CE-conv {c['cost']:.1f} (k={c['k']},D={c['D']})")
            hw = hwxl_paper(q, n, n)
            print(f"q={q} n=m={n}: h-WXL(2.7) {hw[0]:.1f} [Table1 {ref[8]}]\n   " + "\n   ".join(out))
    print("\n== UOV R3 direct attack, TW-reduced n'=m'=m-1 ==")
    for name, q, m, tgt, spec in [('Ip', 256, 45, 143, 146), ('Is', 16, 64, 143, 162),
                                   ('III', 256, 72, 207, 216), ('V', 256, 96, 272, 277)]:
        r = log2(q); n2 = m2 = m - 1
        hp = hwxl_paper(q, n2, m2); hs = hwxl_spec(q, n2, m2)
        print(f"{name}: spec Table5 direct {spec}; my spec-formula h-WXL MQ(q;{n2},{m2}) = {hs[0]:.1f} bits (k={hs[1]},d={hs[2]}); "
              f"paper-(2.7) h-WXL {hp[0]:.1f} field ops (k={hp[1]},D={hp[2]})")
        for w in (2.37, 2.81, 3.0):
            b = pxl(q, n2, m2, w); c = pxl(q, n2, m2, w, conv='ce')
            print(f"   w={w}: PXL paper-conv {b['cost']:.1f} fops (k={b['k']},D={b['D']},alpha=2^{b['la']:.1f}; "
                  f"terms C3=2^{b['t1']:.1f} Cfix=2^{b['tfix']:.1f} Cli2=2^{b['tli2']:.1f}; mem R5 2^{b['mem_r5']:.1f}, a^2C(k+D,D) 2^{b['mem_a2']:.1f}) "
                  f"-> +log2(2r^2+r) {b['cost']+log2(2*r*r+r):.1f}, +2log2 r {b['cost']+2*log2(r):.1f}, "
                  f"relative (spec - (2.7) h-WXL + PXL) {spec - hp[0] + b['cost']:.1f} | "
                  f"CE-conv {c['cost']:.1f} fops (k={c['k']},D={c['D']}) +2log2r {c['cost']+2*log2(r):.1f}")
