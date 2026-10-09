"""Decoder-failure model: weight-w error e on n coordinates with 430 hidden
pairs; dh = pairs with both coordinates in supp(e); each such pair is
'cancelled' in c11 - c12 with probability 1/(q-1); cancelled positions are
invisible in ebar, lie in I, and any I1 containing one fails.
Per-attempt failure and 1000-attempt final failure, Poisson/binomial model."""
from math import comb, exp, factorial, log2
q, n, h, k1, w, cap = 433, 860, 430, 215, 116, 1000
# exact distribution of dh: number of pairs fully inside a random w-subset of 430 pairs
# count subsets with exactly d full pairs: C(430,d) * C(430-d, w-2d) * 2^(w-2d)
tot = comb(n, w)
P_dh = {d: comb(h, d) * comb(h - d, w - 2 * d) * 2 ** (w - 2 * d) / tot for d in range(0, w // 2 + 1)}
mean_dh = sum(d * p for d, p in P_dh.items())
pa_fail = 0.0; final = 0.0; P_c = {}
for d, pd in P_dh.items():
    if pd < 1e-30: continue
    for c in range(0, d + 1):
        pc = comb(d, c) * (1 / (q - 1)) ** c * (1 - 1 / (q - 1)) ** (d - c)
        pr = pd * pc
        P_c[c] = P_c.get(0, 0) * 0 + P_c.get(c, 0) + pr
        wt_ebar = w - d - c                      # d-c unequal double hits count once, c cancel
        I = h - wt_ebar
        succ = comb(I - c, k1) / comb(I, k1) if I - c >= k1 else 0.0
        pa_fail += pr * (1 - succ)
        final += pr * (1 - succ) ** cap
print(f"mean double-hit pairs {mean_dh:.3f}; P(c>=1)=2^{log2(1-P_c[0]):.2f}; "
      + " ".join(f"P(c={c})=2^{log2(P_c[c]):.1f}" for c in range(1, 8)))
print(f"per-attempt failure {pa_fail:.6f} (spec: 1-0.988123 = {1-0.988123:.6f})")
print(f"final failure after {cap} attempts: 2^{log2(final):.2f} (kem-38-5 exact: 2^-43.317)")
# contributions by c
for c in range(3, 9):
    contrib = 0.0
    for d, pd in P_dh.items():
        if d < c or pd < 1e-30: continue
        pc = comb(d, c) * (1 / (q - 1)) ** c * (1 - 1 / (q - 1)) ** (d - c)
        I = h - (w - d - c)
        succ = comb(I - c, k1) / comb(I, k1)
        contrib += pd * pc * (1 - succ) ** cap
    print(f"  c={c}: contribution 2^{log2(contrib) if contrib>0 else float('-inf'):.1f}")
