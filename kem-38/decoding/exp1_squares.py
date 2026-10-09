"""Schur-square / product / hull / shortening distinguishers: UVW structured
code vs random code of the same parameters.  q = 433 throughout."""
import sys, time
from pathlib import Path
import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parent))
from uvwlin import *

q = 433
sizes = [int(a) for a in sys.argv[1:]] or [60, 100, 200, 400, 860]
rng = np.random.default_rng(7)

def line(*a):
    print(*a, flush=True)

for n in sizes:
    t0 = time.time()
    K = UVWKey(q, n, seed=1000 + n)
    k, k1, k2 = K.k, K.k1, K.k2
    C = K.Gpub
    H = dual(C, q)
    Cr = random_code(rng, k, n, q)
    Hr = dual(Cr, q)
    line(f"\n=== n={n} k={k} k1={k1} k2={k2}  (keygen+dual {time.time()-t0:.1f}s)")
    rows = []
    rows.append(("dim C^2", schur_square_dim(C, q), schur_square_dim(Cr, q)))
    rows.append(("dim (C^perp)^2", schur_square_dim(H, q), schur_square_dim(Hr, q)))
    rows.append(("dim C*C^perp", schur_product_dim(C, H, q), schur_product_dim(Cr, Hr, q)))
    rows.append(("hull dim", hull_dim(C, q), hull_dim(Cr, q)))
    # structured subcodes (secret; positive controls)
    T = K.uu_subcode(); S = K.dual_xx_subcode()
    rows.append(("dim T^2 (uu)D secret", schur_square_dim(T, q, exact=True), "-"))
    rows.append(("dim S^2 (x,-x)D' secret", schur_square_dim(S, q, exact=True), "-"))
    rows.append(("dim T*S secret", schur_product_dim(T, S, q), "-"))
    rows.append(("dim T*C^perp secret", schur_product_dim(T, H, q), "-"))
    rows.append(("dim C*S secret", schur_product_dim(C, S, q), "-"))
    for name, a, b in rows:
        line(f"  {name:28s} structured={a!s:>5}  random={b!s:>5}")
    line(f"  [{time.time()-t0:.1f}s]")

    # shortening at s random positions: dims and squares
    line("  shortened at s random positions: (dim, square dim) structured vs random")
    for s in sorted(set([2, 10, n // 8, n // 4, n // 2 - n // 20, n // 2 - 12, n // 2 - 6])):
        if s <= 0 or s >= k:
            continue
        pos = rng.choice(n, s, replace=False)
        Hs, Hrs = shorten(H, q, pos), shorten(Hr, q, pos)
        Cs, Crs = shorten(C, q, pos), shorten(Cr, q, pos)
        line(f"    s={s:4d} n'={n-s:4d}: C^perp ({Hs.shape[0]},{schur_square_dim(Hs,q)}) vs ({Hrs.shape[0]},{schur_square_dim(Hrs,q)});"
             f"  C ({Cs.shape[0]},{schur_square_dim(Cs,q)}) vs ({Crs.shape[0]},{schur_square_dim(Crs,q)})")
    # puncturing at s random positions
    line("  punctured at s random positions: (dim, square dim) structured vs random")
    for s in sorted(set([2, n // 8, n // 4, n // 2 - 12, n // 2 - 2])):
        if s <= 0:
            continue
        pos = rng.choice(n, s, replace=False)
        Hp, Hrp = puncture(H, q, pos), puncture(Hr, q, pos)
        line(f"    s={s:4d} n'={n-s:4d}: C^perp ({Hp.shape[0]},{schur_square_dim(Hp,q)}) vs ({Hrp.shape[0]},{schur_square_dim(Hrp,q)})")
    # shortening at p TRUE pairs (secret knowledge; positive control)
    line("  shortened at p true pairs (secret): (dim, square dim) structured vs random code shortened at 2p positions")
    h = n // 2
    for p in sorted(set([h // 2, int(0.8 * h // 2), int(0.9 * h // 2), int(0.93 * h // 2), int(0.96 * h // 2)])):
        if p <= 0 or 2 * p >= k:
            continue
        pr = rng.choice(h, p, replace=False)
        pos = [c for t in pr for c in K.pairs[t]]
        Hs, Hrs = shorten(H, q, pos), shorten(Hr, q, pos)
        kk, nn = Hs.shape
        bound = (2 * (k2 - p) - 1) + (k2 - p) ** 2 + (k2 - p) * (k2 - p + 1) // 2
        line(f"    p={p:4d} (2p={2*p}) n'={nn:4d}: C^perp ({kk},{schur_square_dim(Hs,q)}) vs random ({Hrs.shape[0]},{schur_square_dim(Hrs,q)});"
             f" generic min(n',k'(k'+1)/2)={min(nn,kk*(kk+1)//2)} structured bound<={min(nn,bound)}")
    line(f"  [{time.time()-t0:.1f}s total]")
