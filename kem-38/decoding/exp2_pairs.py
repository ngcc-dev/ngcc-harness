"""Pair tests: does a true hidden pair (i, pi(i)) with its ratio give a signal
distinguishable from a false pair?  Plus the m-known-pairs threshold test,
codeword-level recognition of structured words, and (u,u)D-word exposure."""
import sys, time
from pathlib import Path
import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parent))
from uvwlin import *

q = 433
n = int(sys.argv[1]) if len(sys.argv) > 1 else 200
full = n >= 800
rng = np.random.default_rng(11)
K = UVWKey(q, n, seed=2000 + n)
k, k1, k2, h = K.k, K.k1, K.k2, n // 2
C = K.Gpub
H = dual(C, q)
T = K.uu_subcode()
S = K.dual_xx_subcode()
print(f"=== n={n} k={k} k1={k1} k2={k2}", flush=True)

def functional(G, i, j, lam):
    return (G[:, i] - lam * G[:, j]) % q

def subcode_eq(G, i, j, lam):
    """{c in rowspace(G): c_i = lam c_j}"""
    f = functional(G, i, j, lam)
    M = nullspace(f[None, :], q)
    return mmul(M, G, q)

# ---- A: single-pair statistics -------------------------------------------
true_pairs = [K.pairs[t] for t in rng.choice(h, 3, replace=False)]
false_pairs = []
pairset = set(K.pairs) | set((j, i) for i, j in K.pairs)
while len(false_pairs) < 3:
    i, j = rng.choice(n, 2, replace=False)
    if (int(i), int(j)) not in pairset:
        false_pairs.append((int(i), int(j)))
t0 = time.time()
print("A. single-pair tests (true pair with true ratio | false pair with random ratio)")
hdr = ["dim C sh{i,j}", "dim Cp sh{i,j}", "dim C pu{i,j}", "dim {c_i=l c_j}", "sq {c_i=l c_j}", "sq C sh{i,j}", "sq Cp sh{i,j}", "dim C∩J1C"]
print("   kind      " + " ".join(f"{x:>16s}" for x in hdr))
for kind, plist in (("true", true_pairs), ("false", false_pairs)):
    for (i, j) in plist:
        lam = int(K.ratio(i, j)) if kind == "true" else int(rng.integers(1, q))
        Csub = subcode_eq(C, i, j, lam)
        Csh, Hsh = shorten(C, q, [i, j]), shorten(H, q, [i, j])
        # C ∩ J1(C): J1 swaps i,j with ratio lam (c_i -> lam c_j, c_j -> c_i/lam)
        J1C = C.copy()
        J1C[:, i] = (lam * C[:, j]) % q
        J1C[:, j] = (pow(lam, q - 2, q) * C[:, i]) % q
        vals = [Csh.shape[0], Hsh.shape[0], puncture(C, q, [i, j]).shape[0], Csub.shape[0],
                schur_square_dim(Csub, q), schur_square_dim(Csh, q), schur_square_dim(Hsh, q),
                intersect_dim(C, J1C, q)]
        print(f"   {kind:5s} {i:4d},{j:4d} " + " ".join(f"{v:16d}" for v in vals), flush=True)
print(f"   per-pair cost of the cheapest test (one rank): {(time.time()-t0)/6/8:.3f}s;"
      f" all C({n},2)*{q-1} (pair,ratio) triples = {n*(n-1)//2*(q-1):.3e}")

# ---- B: threshold test with m known true pairs ----------------------------
print("B. rank of m known true pair functionals + 1 candidate (true | false), rank on C and dim C∩J_mC")
order = rng.permutation(h)
cand_true = K.pairs[order[-1]]
cand_false = false_pairs[0]
for m in sorted(set([1, 2, k2 // 2, k2 - 2, k2 - 1, k2, k2 + 1, k2 + 5])):
    known = [K.pairs[t] for t in order[:m]]
    F = np.array([functional(C, i, j, int(K.ratio(i, j))) for (i, j) in known])
    rk = rank_mod(F, q)
    ft = functional(C, *cand_true, K.ratio(*cand_true))
    ff = functional(C, *cand_false, int(rng.integers(1, q)))
    r_true = rank_mod(np.vstack([F, ft[None, :]]), q)
    r_false = rank_mod(np.vstack([F, ff[None, :]]), q)
    print(f"   m={m:4d}: rank(known)={rk:4d}  +true -> {r_true:4d}   +false -> {r_false:4d}   "
          f"(signal iff ranks differ; dim C∩J_mC = k - rank = {k-rk})", flush=True)

# ---- C: codeword-level recognition ----------------------------------------
print("C. codeword-level test dim(s*Q ∩ s'*Q), Q = C^perp: two secret S-words | two random Q-words; and primal T-words in C")
def diag_code(v, G):
    return (G * v[None, :]) % q
res = []
for trial in range(3):
    s1 = mmul(rng.integers(0, q, (1, S.shape[0])), S, q)[0]
    s2 = mmul(rng.integers(0, q, (1, S.shape[0])), S, q)[0]
    q1 = mmul(rng.integers(0, q, (1, H.shape[0])), H, q)[0]
    q2 = mmul(rng.integers(0, q, (1, H.shape[0])), H, q)[0]
    t1 = mmul(rng.integers(0, q, (1, T.shape[0])), T, q)[0]
    t2 = mmul(rng.integers(0, q, (1, T.shape[0])), T, q)[0]
    c1 = mmul(rng.integers(0, q, (1, C.shape[0])), C, q)[0]
    c2 = mmul(rng.integers(0, q, (1, C.shape[0])), C, q)[0]
    res.append((intersect_dim(diag_code(s1, H), diag_code(s2, H), q),
                intersect_dim(diag_code(q1, H), diag_code(q2, H), q),
                intersect_dim(diag_code(t1, C), diag_code(t2, C), q),
                intersect_dim(diag_code(c1, C), diag_code(c2, C), q)))
print("   (S,S) | (Q,Q) | (T,T) | (C,C):", res)

# ---- D: exposure of pairs from a few (u,u)D words or (x,-x)D' words --------
print("D. pairs recovered by ratio matching from r structured words (true found / false positives)")
def ratio_match(words):
    r, nn = words.shape
    inv = inv_table(q)
    # signature of coordinate c: (w_2/w_1, ..., w_r/w_1) if w_1 != 0
    sig = {}
    for c in range(nn):
        if words[0, c] == 0:
            continue
        key = tuple(int((words[t, c] * inv[words[0, c]]) % q) for t in range(1, r))
        sig.setdefault(key, []).append(c)
    cands = set()
    for key, cs in sig.items():
        for a in range(len(cs)):
            for b in range(a + 1, len(cs)):
                cands.add((cs[a], cs[b]))
    truth = set(tuple(sorted(p)) for p in K.pairs)
    tp = len(cands & truth)
    return tp, len(cands) - tp
for r in (2, 3, 4):
    W = mmul(rng.integers(0, q, (r, T.shape[0])), T, q)
    X = mmul(rng.integers(0, q, (r, S.shape[0])), S, q)
    print(f"   r={r}: from (u,u)D words of C: {ratio_match(W)};  from (x,-x)D' words of C^perp: {ratio_match(X)}")
