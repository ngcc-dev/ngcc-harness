#!/usr/bin/env python3
"""Toy UVW keys over F3 (structure of spec Alg. 6 / pseudocode.md) versus
random codes: hull dimension, Schur-square dimensions of C, C^perp and of
shortened codes, and (u,u)-type codeword counts by exhaustive search at a
tiny size.  Pure numpy, mod-3 arithmetic."""
import numpy as np, sys, itertools

rng = np.random.default_rng(int(sys.argv[1]) if len(sys.argv) > 1 else 1)

def rref3(M):
    M = M.copy() % 3
    r = 0; piv = []
    rows, cols = M.shape
    for c in range(cols):
        if r >= rows: break
        nz = np.nonzero(M[r:, c])[0]
        if len(nz) == 0: continue
        i = r + nz[0]
        M[[r, i]] = M[[i, r]]
        if M[r, c] == 2: M[r] = (2 * M[r]) % 3
        for j in range(rows):
            if j != r and M[j, c]:
                M[j] = (M[j] - M[j, c] * M[r]) % 3
        piv.append(c); r += 1
    return M[:r], piv

def rank3(M):
    return rref3(M)[0].shape[0]

def nullspace3(H):
    """basis of {x : H x^T = 0} as rows."""
    R, piv = rref3(H)
    n = H.shape[1]
    free = [c for c in range(n) if c not in piv]
    B = []
    for f in free:
        v = np.zeros(n, dtype=np.int64); v[f] = 1
        for i, p in enumerate(piv):
            v[p] = (-R[i, f]) % 3
        B.append(v)
    return np.array(B, dtype=np.int64) if B else np.zeros((0, n), dtype=np.int64)

def schur_square(G):
    k = G.shape[0]
    rows = [(G[i] * G[j]) % 3 for i in range(k) for j in range(i, k)]
    return rank3(np.array(rows, dtype=np.int64))

def hull_dim(G):
    H = nullspace3(G)                       # dual generator
    return G.shape[0] - rank3(np.vstack([G, H])) + H.shape[0] - 0 if False else \
        G.shape[0] + H.shape[0] - rank3(np.vstack([G, H]))

def uvw_key(m, k1, k2):
    r1, r2 = m - k1, m - k2
    HX = rng.integers(0, 3, (r1, m)); HY = rng.integers(0, 3, (r1, m)); HZ = rng.integers(0, 3, (r2, m))
    top = np.hstack([HX, HY]); bot = np.hstack([(-HZ) % 3, HZ])
    Hsk = np.vstack([top, bot]) % 3
    perm = rng.permutation(2 * m); diag = rng.integers(1, 3, 2 * m)
    H = (Hsk[:, perm] * diag[None, :]) % 3    # column monomial map
    return H

def random_H(n, k):
    return rng.integers(0, 3, (n - k, n))

def invariants(H):
    n = H.shape[1]
    G = nullspace3(H)           # code generator
    k = G.shape[0]
    out = dict(k=k, hull=hull_dim(G), sq=schur_square(G), sqd=schur_square(rref3(H)[0]))
    # shortened code squares on random coordinate sets of size n-s, with s chosen so that
    # k_short(k_short+1)/2 is around n-s (where a square could be distinguishing)
    res = []
    for s in [int(0.4 * n), int(0.5 * n), int(0.6 * n)]:
        S = rng.choice(n, s, replace=False)
        keep = np.array([c for c in range(n) if c not in set(S)])
        # shortening: codewords zero on S -> generator rows of nullspace of H restricted plus constraints
        Hs = np.vstack([H, np.eye(n, dtype=np.int64)[S]]) % 3
        Gs = nullspace3(Hs)[:, keep]
        ks = Gs.shape[0]
        res.append((s, ks, len(keep), schur_square(Gs) if ks > 0 else 0, min(len(keep), ks * (ks + 1) // 2)))
    out['short'] = res
    return out

def count_uu(H, maxw):
    """count codewords with weight <= maxw (exhaustive for tiny n)."""
    G = nullspace3(H); k, n = G.shape
    cnt = {}
    for coeffs in itertools.product(range(3), repeat=k):
        c = np.array(coeffs) @ G % 3
        w = int(np.count_nonzero(c))
        cnt[w] = cnt.get(w, 0) + 1
    return cnt

if __name__ == '__main__':
    for (m, k1, k2) in [(24, 16, 8), (40, 27, 13), (60, 40, 20)]:
        n, k = 2 * m, k1 + k2
        print(f'-- n={n} k={k} k1={k1} k2={k2}')
        for trial in range(3):
            Hu = uvw_key(m, k1, k2); Hr = random_H(n, k)
            iu = invariants(Hu); ir = invariants(Hr)
            print('  UVW   :', iu)
            print('  random:', ir)
    # exhaustive weight distribution at a tiny size: n=20, k=10 (3^10 codewords)
    m, k1, k2 = 10, 7, 3
    print('-- weight distribution n=20 k=10 k1=7 k2=3 (3 keys each)')
    for trial in range(3):
        Hu = uvw_key(m, k1, k2); Hr = random_H(2 * m, k1 + k2)
        cu = count_uu(Hu, 20); cr = count_uu(Hr, 20)
        print('  UVW   :', [cu.get(w, 0) for w in range(0, 21)])
        print('  random:', [cr.get(w, 0) for w in range(0, 21)])
