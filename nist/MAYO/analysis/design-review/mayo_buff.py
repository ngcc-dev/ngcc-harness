#!/usr/bin/env python3
"""BUFF-property witnesses for MAYO On-Ramp Round 3 (spec-level, verified with the
submitted reference library loaded read-only through the shared harness ABI).

Usage: python3 -I mayo_buff.py <repo-root> <mayo_1|mayo_2|mayo_3|mayo_5> [trials]

Witnesses (all checked with the reference library's crypto_sign_open):
  NR / S-CEO : given only (pk, sig) -- no message knowledge -- build pk' != pk
               such that sig stays valid under pk' for whatever message it signs.
  S-DEO      : given (pk, M1, sig), build pk' under which sig is valid for M2 != M1.
  MBS        : build a (malicious) pk' and ONE sig valid for two chosen messages
               M1 != M2 (uses the On-Ramp Round 3 linear term L(Lambda, s)).
Only P3 (the part of cpk after seed_pk) is changed; it enters P*(s) linearly.
"""
import ctypes, hashlib, json, os, sys

# ---------------- GF(16) = GF(2)[x]/(x^4+x+1) ----------------
def gmul(a, b):
    r = 0
    for i in range(4):
        if (b >> i) & 1:
            r ^= a << i
    for i in (7, 6, 5, 4):
        if (r >> i) & 1:
            r ^= 0b10011 << (i - 4)
    return r
MUL = [[gmul(a, b) for b in range(16)] for a in range(16)]
INV = [0] + [next(b for b in range(1, 16) if MUL[a][b] == 1) for a in range(1, 16)]

def solve(rows, rhs, ncols):
    """Solve A x = b over GF(16); rows: list of lists. Returns one solution or None."""
    A = [list(r) + [b] for r, b in zip(rows, rhs)]
    piv = []
    r = 0
    for c in range(ncols):
        p = next((i for i in range(r, len(A)) if A[i][c]), None)
        if p is None:
            continue
        A[r], A[p] = A[p], A[r]
        iv = INV[A[r][c]]
        A[r] = [MUL[iv][v] for v in A[r]]
        for i in range(len(A)):
            if i != r and A[i][c]:
                f = A[i][c]
                A[i] = [v ^ MUL[f][w] for v, w in zip(A[i], A[r])]
        piv.append(c)
        r += 1
    if any(A[i][ncols] for i in range(r, len(A))):
        return None
    x = [0] * ncols
    for i, c in enumerate(piv):
        x[c] = A[i][ncols]
    return x

# ---------------- parameters ----------------
def load_params(root, name):
    ip = json.load(open(os.path.join(root, 'MAYO/internal-parameters.json')))
    P = next(p for p in ip['parameter_sets'] if p['parameter'] == name)
    P['Z'] = ip['whipping_exponents'][str(P['k'])]
    return P

def zmul_pow(vec, e, tail):
    """vec (length m, coeff of z^a) times z^e mod f, z^m = tail[0]+tail[1]z+..."""
    v = list(vec)
    m = len(v)
    for _ in range(e):
        top = v[-1]
        v = [0] + v[:-1]
        if top:
            for j, tj in enumerate(tail):
                v[j] ^= MUL[top][tj]
    return v

def kmul_matrix(w, tail, m):
    """m x m matrix of multiplication by polynomial w (len m) in K."""
    cols = []
    for a in range(m):
        e = [0] * m; e[a] = 1
        cols.append(e)
    # column a = z^a * w
    res = []
    cur = list(w)
    for a in range(m):
        res.append(cur)
        cur = zmul_pow(cur, 1, tail)
    return [[res[a][i] for a in range(m)] for i in range(m)]

def nib_decode(b, cnt):
    out = []
    for byte in b:
        out += [byte & 15, byte >> 4]
    return out[:cnt]

def nib_encode(v):
    if len(v) % 2:
        v = v + [0]
    return bytes(v[i] | (v[i + 1] << 4) for i in range(0, len(v), 2))

# ---------------- hashing: t and Lambda ----------------
def t_lambda(P, msg, salt):
    m, n = P['m'], P['n']
    md = hashlib.shake_256(msg).digest(P['digest_bytes'])
    h = hashlib.shake_256(md + salt).digest(m // 2 + 32)
    t = nib_decode(h[:m // 2], m)
    key, iv = h[m // 2:m // 2 + 16], h[m // 2 + 16:m // 2 + 32]
    nbytes = n * m // 2
    from Crypto.Cipher import AES
    ecb = AES.new(key, AES.MODE_ECB)
    ks = b''
    ctr = 0
    while len(ks) < nbytes:
        blk = bytearray(iv)
        for j in range(4):
            blk[12 + j] ^= (ctr >> (24 - 8 * j)) & 0xff
        ks += ecb.encrypt(bytes(blk))
        ctr += 1
    ks = ks[:nbytes]
    Lam = [nib_decode(ks[c * m // 2:(c + 1) * m // 2], m) for c in range(n)]  # Lam[col][row]
    return t, Lam

def L_eval(P, Lam, s_blocks):
    m, k = P['m'], P['k']
    tail = P['f_tail_constant_to_degree3']
    acc = [0] * m
    for i in range(k):
        v = [0] * m
        for c, sc in enumerate(s_blocks[i]):
            if sc:
                col = Lam[c]
                v = [x ^ MUL[sc][y] for x, y in zip(v, col)]
        v = zmul_pow(v, P['Z'][i][0], tail)
        acc = [x ^ y for x, y in zip(acc, v)]
    return acc

# ---------------- P3 linear contribution ----------------
def p3_index(o, r, c):
    return sum(o - rr for rr in range(r)) + (c - r)

def W_rc(P, s_blocks, r, c):
    """W_rc(z) = sum_{i<=j} w_ij[r,c] z^{e_ij}; contribution of Delta_a[r,c]=d_a is (sum_a d_a z^a)*W_rc."""
    m, n, o, k = P['m'], P['n'], P['o'], P['k']
    tail = P['f_tail_constant_to_degree3']
    x = [blk[n - o:] for blk in s_blocks]
    W = [0] * m
    for i in range(k):
        for j in range(i, k):
            if i == j:
                w = MUL[x[i][r]][x[i][c]]
            else:
                w = 0 if r == c else MUL[x[i][r]][x[j][c]] ^ MUL[x[i][c]][x[j][r]]
            if w:
                e = [0] * m; e[0] = w
                e = zmul_pow(e, P['Z'][i][j], tail)
                W = [a ^ b for a, b in zip(W, e)]
    return W

def apply_delta(P, pk, deltas):
    """deltas: dict (r,c) -> list of m nibbles to XOR into P3_a[r,c]."""
    m, o = P['m'], P['o']
    pk = bytearray(pk)
    base = P['pk_seed_bytes']
    for (r, c), d in deltas.items():
        idx = p3_index(o, r, c)
        off = base + idx * m // 2
        cur = nib_decode(pk[off:off + m // 2], m)
        pk[off:off + m // 2] = nib_encode([a ^ b for a, b in zip(cur, d)])
    return bytes(pk)

def kdiv(P, T, W):
    """delta(z) with delta*W = T in K."""
    m = P['m']
    M = kmul_matrix(W, P['f_tail_constant_to_degree3'], m)
    return solve(M, T, m)

# ---------------- library ----------------
class Lib:
    def __init__(self, path):
        self.l = ctypes.CDLL(path)
        class Api(ctypes.Structure):
            _fields_ = [('abi', ctypes.c_uint32), ('scheme', ctypes.c_char_p), ('param', ctypes.c_char_p),
                        ('impl', ctypes.c_char_p), ('pkb', ctypes.c_uint64), ('skb', ctypes.c_uint64),
                        ('sigb', ctypes.c_uint64)]
        self.l.nist_sig_get_api.restype = ctypes.POINTER(Api)
        a = self.l.nist_sig_get_api().contents
        self.pkb, self.skb, self.sigb = a.pkb, a.skb, a.sigb
    def seed(self, s):
        self.l.nist_sig_seed(ctypes.c_char_p(s))
    def keypair(self):
        pk = ctypes.create_string_buffer(self.pkb); sk = ctypes.create_string_buffer(self.skb)
        assert self.l.nist_sig_keypair(pk, sk) == 0
        return pk.raw, sk.raw
    def sign(self, msg, sk):
        sm = ctypes.create_string_buffer(len(msg) + self.sigb); sl = ctypes.c_uint64(0)
        assert self.l.nist_sig_sign(sm, ctypes.byref(sl), msg, ctypes.c_uint64(len(msg)), sk) == 0
        return sm.raw[:sl.value - len(msg)]
    def verify(self, msg, sig, pk):
        sm = sig + msg
        out = ctypes.create_string_buffer(len(sm)); ol = ctypes.c_uint64(0)
        return self.l.nist_sig_open(out, ctypes.byref(ol), sm, ctypes.c_uint64(len(sm)), pk) == 0

def main():
    root, name = sys.argv[1], sys.argv[2]
    trials = int(sys.argv[3]) if len(sys.argv) > 3 else 3
    P = load_params(root, name)
    m, n, o, k = P['m'], P['n'], P['o'], P['k']
    lib = Lib(os.path.join(root, 'MAYO/lib', name + '.so'))
    assert lib.pkb == P['cpk_bytes'] and lib.sigb == P['sig_bytes']
    res = {'param': name, 'trials': trials, 'ok': {'honest': 0, 'NR_SCEO': 0, 'SDEO': 0, 'MBS': 0,
           'MBS_round2_style_check': 0}}
    for tr in range(trials):
        lib.seed(hashlib.sha384(b'mayo-buff-%d' % tr).digest())
        pk, sk = lib.keypair()
        M1 = b'message one %d' % tr
        M2 = b'message two %d' % tr
        sig = lib.sign(M1, sk)
        sb = P['sig_bytes'] - P['salt_bytes']
        s = nib_decode(sig[:sb], k * n)
        blocks = [s[i * n:(i + 1) * n] for i in range(k)]
        salt = sig[sb:]
        res['ok']['honest'] += lib.verify(M1, sig, pk)
        # sanity: our L/t model is consistent (P*(s) unknown here, so only used below)

        # --- NR / S-CEO: kernel Delta using two diagonal positions (no message used) ---
        W0, W1 = W_rc(P, blocks, 0, 0), W_rc(P, blocks, 1, 1)
        d0 = [0] * m; d0[0] = 1                    # delta0(z) = 1
        T = W0                                    # need delta1*W1 = delta0*W0 (char 2)
        d1 = kdiv(P, T, W1)
        pk2 = apply_delta(P, pk, {(0, 0): d0, (1, 1): d1})
        if pk2 != pk and lib.verify(M1, sig, pk2):
            res['ok']['NR_SCEO'] += 1

        # --- S-DEO: same sig valid for M2 under pk3 ---
        t1, L1 = t_lambda(P, M1, salt)
        t2, L2 = t_lambda(P, M2, salt)
        Ls1, Ls2 = L_eval(P, L1, blocks), L_eval(P, L2, blocks)
        # P*_new(s) = P*_old(s) + D(s); P*_old(s) = t1 + L1(s); need = t2 + L2(s)
        target = [a ^ b ^ c ^ d for a, b, c, d in zip(t1, Ls1, t2, Ls2)]
        d = kdiv(P, target, W0)
        pk3 = apply_delta(P, pk, {(0, 0): d})
        if lib.verify(M2, sig, pk3) and not lib.verify(M2, sig, pk):
            res['ok']['SDEO'] += 1

        # --- MBS: malicious pk4 and sig4 valid for both M1 and M2 ---
        salt4 = os.urandom(P['salt_bytes'])
        ta, La = t_lambda(P, M1, salt4)
        tb, Lb = t_lambda(P, M2, salt4)
        # unknowns x_i (oil coords) with s_i = (0^{n-o}, x_i); constraint L_{La-Lb}(s) = ta - tb
        cols = []
        for i in range(k):
            for c in range(o):
                blk = [[0] * n for _ in range(k)]
                blk[i][n - o + c] = 1
                va, vb = L_eval(P, La, blk), L_eval(P, Lb, blk)
                cols.append([x ^ y for x, y in zip(va, vb)])
        rows = [[cols[j][r] for j in range(k * o)] for r in range(m)]
        rhs = [x ^ y for x, y in zip(ta, tb)]
        xs = solve(rows, rhs, k * o)
        if xs is not None:
            blk = [[0] * (n - o) + xs[i * o:(i + 1) * o] for i in range(k)]
            Wd = W_rc(P, blk, 0, 0)
            T = [x ^ y for x, y in zip(ta, L_eval(P, La, blk))]  # need P*_{P3'}(s) = ta - La(s)
            dd = kdiv(P, T, Wd)
            if dd is not None:
                pk4 = os.urandom(P['pk_seed_bytes']) + bytes(P['P3_bytes'])
                pk4 = apply_delta(P, pk4, {(0, 0): dd})
                sig4 = nib_encode(sum(blk, [])) + salt4
                if lib.verify(M1, sig4, pk4) and lib.verify(M2, sig4, pk4):
                    res['ok']['MBS'] += 1
                # Round-2 contrast: without Lambda the same equations would need ta == tb.
                res['ok']['MBS_round2_style_check'] += int(ta != tb)
    print(json.dumps(res))

if __name__ == '__main__':
    main()
