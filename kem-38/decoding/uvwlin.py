"""Modular linear algebra over F_q (q prime < 2^11) with numpy, and the exact
UVW-KEM key structure from kem-38/pseudocode.md (own randomness).

Matmuls run in float64 (exact: entries < q, inner dimension <= ~2000,
so products < 433^2 * 2000 ~ 3.7e8 << 2^53) and are reduced mod q.
"""
import numpy as np

# ---------------------------------------------------------------- basics

def mmul(A, B, q):
    """(A @ B) mod q, exact via float64."""
    C = np.asarray(A, dtype=np.float64) @ np.asarray(B, dtype=np.float64)
    return (C % q).astype(np.int64)


def inv_table(q):
    t = np.zeros(q, dtype=np.int64)
    for a in range(1, q):
        t[a] = pow(a, q - 2, q)
    return t


class RREF:
    """Incremental reduced row echelon basis of a row space over F_q."""

    def __init__(self, n, q):
        self.n, self.q = n, q
        self.inv = inv_table(q)
        self.E = np.zeros((0, n), dtype=np.int64)   # rows in RREF
        self.piv = []                                 # pivot column per row

    @property
    def rank(self):
        return self.E.shape[0]

    def _reduce_against_E(self, B):
        if self.rank == 0:
            return B % self.q
        coef = B[:, self.piv]
        return (B - mmul(coef, self.E, self.q)) % self.q

    def add(self, B):
        """Add rows B (m x n). Returns number of new pivots."""
        q = self.q
        B = np.array(B, dtype=np.int64) % q
        B = self._reduce_against_E(B)
        new_rows, new_piv = [], []
        # Gaussian elimination within B (B is zero on existing pivots).
        m = B.shape[0]
        for col in range(self.n):
            if m == 0:
                break
            nz = np.nonzero(B[:, col])[0]
            if nz.size == 0:
                continue
            r = nz[0]
            row = (B[r] * self.inv[B[r, col]]) % q
            B = np.delete(B, r, axis=0)
            m -= 1
            if m:
                f = B[:, col:col + 1]
                B = (B - f * row[None, :]) % q
            # eliminate this column from previously found new rows
            for t in range(len(new_rows)):
                c = new_rows[t][col]
                if c:
                    new_rows[t] = (new_rows[t] - c * row) % q
            new_rows.append(row)
            new_piv.append(col)
            if self.rank + len(new_rows) == self.n:
                break
        if not new_rows:
            return 0
        N = np.array(new_rows, dtype=np.int64)
        # eliminate new pivot columns from the old rows
        if self.rank:
            coef = self.E[:, new_piv]
            self.E = (self.E - mmul(coef, N, q)) % q
        self.E = np.vstack([self.E, N])
        self.piv = self.piv + new_piv
        order = np.argsort(self.piv)
        self.E = self.E[order]
        self.piv = [self.piv[i] for i in order]
        return len(new_rows)


def rref(A, q):
    R = RREF(A.shape[1], q)
    R.add(A)
    return R.E, R.piv


def rank_mod(A, q):
    return rref(A, q)[0].shape[0]


def nullspace(A, q):
    """Basis (rows) of {x : A x^T = 0}, i.e. the dual code of rowspace(A)."""
    E, piv = rref(A, q)
    n = A.shape[1]
    free = [c for c in range(n) if c not in set(piv)]
    N = np.zeros((len(free), n), dtype=np.int64)
    for t, f in enumerate(free):
        N[t, f] = 1
        for r, p in enumerate(piv):
            N[t, p] = (-E[r, f]) % q
    return N


def dual(G, q):
    return nullspace(G, q)


def shorten(G, q, S):
    """Code shortened at coordinate set S: codewords zero on S, S deleted."""
    S = sorted(S)
    keep = [c for c in range(G.shape[1]) if c not in set(S)]
    # subcode with zeros on S = nullspace of G[:,S]^T applied to message
    if len(S) == 0:
        return G.copy()
    M = nullspace(G[:, S].T, q)   # messages m with m G_S = 0
    return mmul(M, G, q)[:, keep]


def puncture(G, q, S):
    keep = [c for c in range(G.shape[1]) if c not in set(S)]
    E, _ = rref(G[:, keep], q)
    return E


def intersect_dim(G1, G2, q):
    """dim(rowspace G1 ∩ rowspace G2)."""
    r1, r2 = rank_mod(G1, q), rank_mod(G2, q)
    r = rank_mod(np.vstack([G1, G2]), q)
    return r1 + r2 - r


def hull_dim(G, q):
    return intersect_dim(G, dual(G, q), q)


def schur_square_dim(G, q, exact=False, margin=24, rng=None, batch=512):
    """Dimension of the Schur (componentwise) square of rowspace(G).

    First uses products of random codeword pairs (dim+margin of them); if
    that already saturates at n, return n.  Otherwise (or if exact=True)
    stream all basis products g_i*g_j with early exit at rank n.
    """
    rng = rng or np.random.default_rng(1)
    E, _ = rref(G, q)
    k, n = E.shape
    R = RREF(n, q)
    if not exact:
        target = min(n, k * (k + 1) // 2) + margin
        while R.rank < n and target > 0:
            m = min(batch, target)
            target -= m
            A = mmul(rng.integers(0, q, (m, k)), E, q)
            B = mmul(rng.integers(0, q, (m, k)), E, q)
            R.add((A * B) % q)
        if R.rank == n:
            return n
    # exact streaming over basis products
    R = RREF(n, q)
    rows = []
    for i in range(k):
        P = (E[i:i + 1, :] * E[i:, :]) % q
        rows.append(P)
        if sum(r.shape[0] for r in rows) >= batch:
            R.add(np.vstack(rows))
            rows = []
            if R.rank == n:
                return n
    if rows:
        R.add(np.vstack(rows))
    return R.rank


def schur_product_dim(G1, G2, q, margin=24, rng=None, batch=512, exact=False):
    rng = rng or np.random.default_rng(2)
    E1, _ = rref(G1, q)
    E2, _ = rref(G2, q)
    k1, n = E1.shape
    k2 = E2.shape[0]
    R = RREF(n, q)
    if not exact:
        target = min(n, k1 * k2) + margin
        while R.rank < n and target > 0:
            m = min(batch, target)
            target -= m
            A = mmul(rng.integers(0, q, (m, k1)), E1, q)
            B = mmul(rng.integers(0, q, (m, k2)), E2, q)
            R.add((A * B) % q)
        if R.rank == n:
            return n
    R = RREF(n, q)
    rows = []
    for i in range(k1):
        rows.append((E1[i:i + 1, :] * E2) % q)
        if sum(r.shape[0] for r in rows) >= batch:
            R.add(np.vstack(rows))
            rows = []
            if R.rank == n:
                return n
    if rows:
        R.add(np.vstack(rows))
    return R.rank


# ---------------------------------------------------------------- UVW keys

def random_full_rank(rng, rows, cols, q):
    while True:
        A = rng.integers(0, q, (rows, cols), dtype=np.int64)
        if rank_mod(A, q) == rows:
            return A


def rs_generator(q, n_half, k2):
    """[n/2, k2] RS code, alpha = (1, 2, ..., n/2), rows x^j (spec Def. 5)."""
    alpha = np.arange(1, n_half + 1, dtype=np.int64) % q
    G = np.zeros((k2, n_half), dtype=np.int64)
    row = np.ones(n_half, dtype=np.int64)
    for j in range(k2):
        G[j] = row
        row = (row * alpha) % q
    return G


class UVWKey:
    """Exact keygen structure of kem-38 Algorithm 3 with own randomness."""

    def __init__(self, q, n, k1=None, k2=None, seed=0):
        rng = np.random.default_rng(seed)
        self.q, self.n = q, n
        h = n // 2
        k1 = k1 if k1 is not None else h // 2
        k2 = k2 if k2 is not None else h - k1
        self.k1, self.k2, self.k = k1, k2, k1 + k2
        assert h < q, "RS evaluation points 1..n/2 must be distinct mod q"
        self.GU = random_full_rank(rng, k1, h, q)
        self.GW = random_full_rank(rng, k2, h, q)
        self.GRS = rs_generator(q, h, k2)
        self.GV = (self.GRS + self.GW) % q
        top = np.hstack([self.GU, self.GU])
        bot = np.hstack([self.GV, self.GW])
        G0 = np.vstack([top, bot]) % q
        while True:
            perm = rng.permutation(n)
            scal = rng.integers(1, q, n, dtype=np.int64)
            # D = P * Lambda : column c of Gsk = scal[c] * column perm[c] of G0
            Gsk = (G0[:, perm] * scal[None, :]) % q
            if rank_mod(Gsk[:, :self.k], q) == self.k:
                break
        self.perm, self.scal = perm, scal
        self.Gsk = Gsk
        # inverse map: public coordinate c holds original coordinate perm[c]
        inv = np.empty(n, dtype=np.int64)
        inv[perm] = np.arange(n)
        self.pos = inv  # pos[orig] = public coordinate
        # hidden pairs (left coordinate i, right coordinate j) and ratio
        # c_i / scal[i] - c_j / scal[j] = (RS part) for every codeword.
        self.pairs = [(int(inv[t]), int(inv[t + h])) for t in range(h)]
        # systematic public key
        E, piv = rref(Gsk, q)
        assert piv == list(range(self.k))
        self.Gpub = E
        self.T = E[:, self.k:]

    # structured subcodes (secret knowledge; positive controls)
    def uu_subcode(self):
        """(u,u)D codewords of C: dimension k1."""
        top = np.hstack([self.GU, self.GU])
        return (top[:, self.perm] * self.scal[None, :]) % self.q

    def dual_xx_subcode(self):
        """(x,-x)D' codewords of C^perp, x in RS^perp: dimension n/2-k2."""
        q, h = self.q, self.n // 2
        X = nullspace(self.GRS, q)            # RS^perp, GRS code
        S0 = np.hstack([X, (-X) % q])
        inv_scal = np.array([pow(int(s), q - 2, q) for s in self.scal])
        return (S0[:, self.perm] * inv_scal[None, :]) % q

    def ratio(self, i, j):
        """lambda with c_i - lambda c_j = scal_i * (RS coordinate)."""
        return (self.scal[i] * pow(int(self.scal[j]), self.q - 2, self.q)) % self.q


def random_code(rng, k, n, q):
    return random_full_rank(rng, k, n, q)
