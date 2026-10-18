#!/usr/bin/env python3
"""Verifier: sensitivity of PXL (paper conventions) for uov-Ip TW system (44,44,q=256):
omega sweep, constant on the dense alpha x alpha elimination, and size of the omitted C(d)1 term
(sum_d |T_d|^omega, lower than the paper's bound (|T<=D|+alpha)^omega).
Run from repository root: python3 -I UOV/analysis/pxl_sensitivity.py"""
import sys; sys.path.insert(0, __file__.rsplit('/', 1)[0])
from math import comb, log2
from pxl_verify import D1, alpha
q, n, m = 256, 44, 44; r = 8; conv = log2(2*r*r + r)
def cost(w, cli2c=1.0, incl_c1=False):
    best = None
    for k in range(1, 15):
        nk = n - k; D = D1(nk, m); a = alpha(nk, m, D)
        t3 = k*k*a*comb(nk+D, D)*comb(n+D, D)
        tf = q**k * a*a*comb(k+D, D); tl = cli2c * q**k * a**w
        c1 = sum(comb(nk+d-1, d)**w for d in range(2, D+1)) if incl_c1 else 0
        c = log2(t3 + tf + tl + c1)
        if best is None or c < best[0]: best = (c, k, D, log2(t3), log2(tl), log2(c1) if c1 else None)
    return best
print("omega  fops  spec-bits(2r^2+r)  k D  log2C3 log2Cli2")
for w in (2.37, 2.5, 2.6, 2.7, 2.81, 2.9, 3.0):
    c = cost(w); print(f"{w:5}  {c[0]:.1f}  {c[0]+conv:.1f}  k={c[1]} D={c[2]} C3=2^{c[3]:.1f} Cli2=2^{c[4]:.1f}")
for cc in (4, 8, 16):
    c = cost(2.81, cli2c=cc); print(f"w=2.81, constant {cc} on alpha^w: {c[0]:.1f} fops -> {c[0]+conv:.1f} bits (k={c[1]})")
c = cost(2.81, incl_c1=True); print(f"w=2.81 incl. per-degree C(d)1 = sum|T_d|^w: {c[0]:.1f} fops -> {c[0]+conv:.1f} bits (k={c[1]}, C1=2^{c[5]:.1f})")
c = cost(3.0, incl_c1=True); print(f"w=3 incl. C(d)1: {c[0]:.1f} fops -> {c[0]+conv:.1f} bits (k={c[1]}, C1=2^{c[5]:.1f})")
