import sys, numpy as np
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from uvwlin import *
q, n = 433, 860
rng = np.random.default_rng(11)
K = UVWKey(q, n, seed=2000 + n)
T = K.uu_subcode(); S = K.dual_xx_subcode()
inv = inv_table(q)
truth = set(tuple(sorted(p)) for p in K.pairs)
def ratio_match(words):
    r, nn = words.shape; sig = {}
    for c in range(nn):
        if words[0, c] == 0: continue
        key = tuple(int((words[t, c] * inv[words[0, c]]) % q) for t in range(1, r))
        sig.setdefault(key, []).append(c)
    cands = set((cs[a], cs[b]) for cs in sig.values() for a in range(len(cs)) for b in range(a + 1, len(cs)))
    tp = len(cands & truth); return tp, len(cands) - tp
for r in (2, 3, 4):
    W = mmul(rng.integers(0, q, (r, T.shape[0])), T, q)
    X = mmul(rng.integers(0, q, (r, S.shape[0])), S, q)
    print(f"n=860 r={r}: (u,u)D words of C: true/false = {ratio_match(W)};  (x,-x)D' words of C^perp: {ratio_match(X)}")
