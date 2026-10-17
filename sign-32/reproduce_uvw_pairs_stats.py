"""Public pair-likelihood scoring of UVW full error vectors (H(m,r)-eR^T || e).
Uses only the vectors; the truth file is read only to SCORE the output.
usage: reproduce_uvw_pairs_stats.py LEVEL VECFILE TRUTHFILE [NSIG] [--control-uniform] [--out cand.tsv]"""
import sys, time, numpy as np
P = {128: (9700, 4850, 3250, 1600, 8633), 256: (19200, 9600, 6433, 3167, 17088),
     512: (39000, 19500, 13066, 6434, 34710)}
lev = int(sys.argv[1]); n, k, k1, k2, w = P[lev]; h = n // 2
args = sys.argv[2:]
ctrl = "--control-uniform" in args; args = [a for a in args if a != "--control-uniform"]
out = None
if "--out" in args: i = args.index("--out"); out = args[i + 1]; del args[i:i + 2]
V = np.fromfile(args[0], dtype=np.uint8).reshape(-1, n)
if len(args) > 2: V = V[:int(args[2])]
N = V.shape[0]
assert np.all((V != 0).sum(1) == w), "weight check failed"
rng = np.random.default_rng(1)
if ctrl:  # control: uniformly random weight-w vectors over F3 (the distribution the spec claims)
    V = np.zeros((N, n), np.uint8)
    for s in range(N):
        pos = rng.choice(n, w, replace=False); V[s, pos] = rng.integers(1, 3, w)
T = np.fromfile(args[1], dtype=np.dtype([("p", "<u4"), ("d", "u1")]))
tp, td = T["p"].astype(np.int64), T["d"].astype(np.int64)
partner = np.empty(n, np.int64); partner[tp[:h]] = tp[h:]; partner[tp[h:]] = tp[:h]
# analytic per-signature category model for a true pair (spec Alg. 4/5, code dec_z/dec_xy)
rI = k1 / h; pi = (k2 / h) * 0.5 + (1 - k2 / h) * (2 / 3)
pZ = (1 - rI) * (1 - pi) / 3; pO = (1 - rI) * pi * 2 / 3
pSame = rI * (1 - pi) + (1 - rI) * (1 - pi) * 2 / 3; pNeg = rI * pi + (1 - rI) * pi / 3
rho = w / n; qZ = (1 - rho) ** 2; qO = 2 * rho * (1 - rho); qB = rho ** 2 / 2
lZ, lO, lS, lN = np.log(pZ / qZ), np.log(pO / qO), np.log(pSame / qB), np.log(pNeg / qB)
kl = pZ * lZ + pO * lO + pSame * lS + pNeg * lN
print(f"UVW-{lev} N={N} control={ctrl} model: pZ={pZ:.4f} pO={pO:.4f} same={pSame:.4f} neg={pNeg:.4f} KL/sig={kl:.4f} nats")
t0 = time.time()
Zi = (V == 0).astype(np.float32); X = np.where(V == 0, 0, np.where(V == 1, 1, -1)).astype(np.float32)
Z = Zi.T @ Zi; S = X.T @ X; n0 = Zi.sum(0)
O = n0[:, None] + n0[None, :] - 2 * Z; B = N - Z - O
# with unknown relative scale c: E,Q = (B +/- S)/2 ; choose orientation maximizing LLR
base = Z * lZ + O * lO + B * (lS + lN) / 2
L = base + np.abs(S) * abs(lN - lS) / 2
np.fill_diagonal(L, -np.inf)
print(f"score matrix {n}x{n} in {time.time()-t0:.1f}s")
best = L.argmax(1); bestv = L[np.arange(n), best]
mutual = best[best] == np.arange(n)
idx = np.nonzero(mutual & (np.arange(n) < best))[0]
order = idx[np.argsort(-bestv[idx])]
truehit = partner[order] == best[order]
print(f"mutual-best pairs: {len(order)}  of which true: {truehit.sum()}  (hidden pairs: {h})")
Ks = sorted(set([100, 500, 1000, h - k1, 2 * (h - k1), len(order)]))
for K in Ks:
    if K <= len(order): print(f"  top {K:5d} mutual pairs: precision {truehit[:K].mean():.4f} ({truehit[:K].sum()} true)")
# first false positive rank
ff = np.nonzero(~truehit)[0]
print(f"  first false mutual pair at rank {ff[0] if len(ff) else None}; needed span dim n/2-k1 = {h-k1}")
# true-pair vs random-pair score separation
tl = L[tp[:h], tp[h:]]
rr = rng.integers(0, n, (200000, 2)); rr = rr[rr[:, 0] != rr[:, 1]]
rl = L[rr[:, 0], rr[:, 1]]
print(f"  true-pair LLR mean {tl.mean():.2f} sd {tl.std():.2f}; random-pair mean {rl.mean():.2f} sd {rl.std():.2f}; max random {rl.max():.2f}")
# relative scale check on true mutual pairs: predicted c from dominant 'neg' orientation
a, b = order[truehit], best[order][truehit]
sgn = np.sign(S[a, b])  # dominant: v_b/v_a = -d_b'/d_a'  (neg category more likely)
pos_of = np.empty(n, np.int64); pos_of[tp] = np.arange(n)
dmap = np.where(td == 1, 1, -1)
true_rel = dmap[pos_of[a]] * dmap[pos_of[b]]
print(f"  relative-scale prediction correct on {(-sgn == true_rel).mean():.4f} of true mutual pairs")
if out:
    with open(out, "w") as f:
        for i in order:
            j = best[i]; zs = abs(S[i, j]) / max(np.sqrt(B[i, j]), 1.0)
            f.write(f"{i}\t{j}\t{int(-np.sign(S[i, j]))}\t{bestv[i]:.3f}\t{zs:.3f}\n")
print(f"total {time.time()-t0:.1f}s")
