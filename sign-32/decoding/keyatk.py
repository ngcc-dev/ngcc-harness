#!/usr/bin/env python3
"""Key attack on UVW: find a (u,u)D-type codeword of weight t in the public
[n,k]_3 code (or the (-h,h)D-type words of the dual; symmetric when k=n/2).
N_U(t) = C(n/2,t/2) 2^{t/2} / 3^{n/2-k1}   (spec Prop. 6 / Sendrier 2023 Sec. 4.2)
Generic decoder: Dumer/Stern over F3 (log2 costs, polynomial factors dropped),
iteration cost max(L, L^2/3^l) with L = C((k+l)/2, p/2) 2^{p/2};
success per iteration for a given word: C(k+l,p) C(n-k-l,t-p) / C(n,t);
total = iter_cost / min(1, N_U(t) * P_succ).  Also MMT-like depth-2 with
representations is not implemented; Dumer is an upper bound on cost."""
import math, sys
from math import lgamma, log2
LOG3 = log2(3)
def lb(n, k):
    if k < 0 or k > n or n < 0: return float('-inf')
    return (lgamma(n + 1) - lgamma(k + 1) - lgamma(n - k + 1)) / math.log(2)

def dumer_cost(n, k, t, nsol):
    best = (float('inf'), None)
    for p in range(0, 161, 4):
        for l in range(0, 1600, 16):
            if k + l > n: break
            L = lb((k + l) / 2, p / 2) + p / 2
            it = max(L, 2 * L - l * LOG3)
            ps = lb(k + l, p) + lb(n - k - l, t - p) - lb(n, t)
            tot = it - min(0.0, nsol + ps)
            if tot < best[0]:
                best = (tot, (p, l, L, ps))
    return best

def key_attack(n, k, k1):
    half = n // 2
    best = (float('inf'), None)
    for t in range(2, int(0.35 * n), 2 * max(1, n // 1000)):
        NU = lb(half, t / 2) + t / 2 - (half - k1) * LOG3
        if NU < -5:  # essentially no such codeword
            continue
        c, cfg = dumer_cost(n, k, t, NU)
        if c < best[0]:
            best = (c, (t, NU, cfg))
    return best

if __name__ == '__main__':
    for name, (n, k, k1) in {'UVW128': (9700, 4850, 3250), 'UVW256': (19200, 9600, 6433),
                             'UVW512': (39000, 19500, 13066), 'WaveI': (8576, 4288, 2966)}.items():
        c, (t, NU, cfg) = key_attack(n, k, k1)
        print(f'{name}: Dumer key attack log2 cost {c:.1f}  t={t} (t/n={t/n:.3f}) log2 N_U(t)={NU:.1f} p,l={cfg[0]},{cfg[1]} log2 L={cfg[2]:.1f} log2 Psucc={cfg[3]:.1f}')
        # GV-type weight where N_U(t) ~ 1
        t0 = min(t for t in range(2, n, 2) if lb(n // 2, t / 2) + t / 2 - (n // 2 - k1) * LOG3 >= 0)
        print(f'        first t with N_U(t)>=1: t0={t0} (t0/n={t0/n:.3f}); random-code GV distance (log2 #words=0): ', end='')
        d = min(d for d in range(1, n) if lb(n, d) + d - (n - k) * LOG3 >= 0)
        print(f'd_GV={d} (d/n={d/n:.3f})')
