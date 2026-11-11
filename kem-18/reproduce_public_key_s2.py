#!/usr/bin/env python3
"""kem-18-5: recover the LoongKEM s2 block from a fresh public key.

Each key pair comes from the prebuilt submitted library (kem-18/lib) under a
fresh seed. The attack reads only pk: it expands A4 from the public seed,
decompresses B2, cancels the negacyclic Block(A3*s1) term between rows of each
block, and solves the remaining small-error linear system in s2 with a Kannan
embedding and BKZ (fpylll). sk is parsed only afterwards, for the comparison.

Default: Loong128 on two seeds. NGCC_LONG=1 adds Loong256 (several minutes).
"""
import ctypes
import hashlib
import os
import re
import sys
import time
from pathlib import Path

try:
    import numpy as np
    from fpylll import BKZ, GSO, LLL, IntegerMatrix
    from fpylll.algorithms.bkz2 import BKZReduction
except ImportError as exc:
    print(f"SKIP kem-18-5: {exc}")
    sys.exit(77)
if "sm3" not in hashlib.algorithms_available:
    print("SKIP kem-18-5: hashlib lacks SM3")
    sys.exit(77)

ROOT = Path(__file__).resolve().parents[1]
Q = 8191
# level, seed tag, equations used, final BKZ block size, GSO float type
RUNS = [(128, "a", 90, 30, "double"), (128, "b", 90, 30, "double")]
if os.environ.get("NGCC_LONG") == "1":
    RUNS.append((256, "a", 240, 40, "dd"))


def xof(seed, size):
    out, i = b"", 1
    while len(out) < size:
        out += hashlib.new("sm3", seed + i.to_bytes(4, "big")).digest()
        i += 1
    return out[:size]


def unpack(data, count, bits):
    v = np.unpackbits(np.frombuffer(data, np.uint8), bitorder="little")[:count * bits]
    return v.reshape(count, bits).astype(np.int64) @ (1 << np.arange(bits))


def keypair(level, tag):
    U8, ULL = ctypes.c_uint8, ctypes.c_ulonglong
    lib = ctypes.CDLL(str(ROOT / f"kem-18/lib/libLoong{level}.so"))
    for f in ("pk", "sk"):
        getattr(lib, f"kem_get_{f}_len_bytes").restype = ULL
    pl, sl = lib.kem_get_pk_len_bytes(), lib.kem_get_sk_len_bytes()
    seed = hashlib.sha512(f"kem-18-5-{level}-{tag}".encode()).digest()
    lib.ngcc_seed((U8 * 64).from_buffer_copy(seed), ULL(64))
    pk, sk, a, b = (U8 * pl)(), (U8 * sl)(), ULL(pl), ULL(sl)
    if lib.kem_keygen(pk, ctypes.byref(a), sk, ctypes.byref(b)):
        raise RuntimeError("kem_keygen failed")
    return bytes(pk)[:a.value], bytes(sk)[:b.value], seed


def public_system(pk, p):
    """Rows F and right-hand side y with F s2 = y + small error (mod q)."""
    n, k1, k2, db, sb = p["N"], p["K1"], p["K2"], p["DB"], p["SEED_BYTES"]
    xlen = k1 * k1 * n + 2 * k1 * k2 * n + k2 * k2 * n * n
    raw = np.frombuffer(xof(pk[:sb], 3 * xlen), np.uint8).astype(np.int64)
    w = (raw[0::2] << 8) | raw[1::2]                  # sample_vector, poly.c
    w = w[w < 8 * Q][:xlen] % Q
    a4 = w[k1 * k1 * n + 2 * k1 * k2 * n:].reshape(k2 * n, k2 * n)
    step = 1 << (13 - db)
    b2 = unpack(pk[sb + k1 * n * db // 8:], k2 * n * n, db).reshape(k2 * n, n)
    b2 = np.minimum(b2 * step + step // 2, Q - 1)      # decompress, poly.c

    def coeffs(r, j):                                  # (A4 S2)[r][j] in s2
        row = np.zeros(k2 * n, np.int64)
        for blk in range(k2):
            for i in range(n):
                row[blk * n + (j - i) % n] += a4[r, blk * n + i] * (1 if j >= i else -1)
        return row

    rows, rhs = [], []
    for blk in range(k2):
        for r in range(1, n):
            for j in range(n):
                sg, jj = (1 if j >= r else -1), (j - r) % n
                rows.append((coeffs(blk * n + r, j) - sg * coeffs(blk * n, jj)) % Q)
                rhs.append((b2[blk * n + r, j] - sg * b2[blk * n, jj]) % Q)
    return np.array(rows), np.array(rhs), 2 * p["ETA"] + step


def recover(f, y, bound, eta, beta_max, ftype):
    n, m = f.shape[1], f.shape[0]
    dim = n + m + 1
    basis = IntegerMatrix(dim, dim)
    for i in range(n):
        basis[i, i] = 1
        for k in range(m):
            basis[i, n + k] = int(f[k, i])
    for k in range(m):
        basis[n + k, n + k] = Q
        basis[dim - 1, n + k] = int(-y[k] % Q)
    basis[dim - 1, dim - 1] = 1

    def candidates():
        found = set()
        for r in range(dim):
            v = [basis[r, c] for c in range(dim)]
            if abs(v[-1]) != 1:
                continue
            x = np.array(v[:n]) * v[-1]
            res = (f @ x - y) % Q
            res = np.where(res > Q // 2, res - Q, res)
            if np.max(np.abs(x)) <= eta and np.max(np.abs(res)) <= bound:
                found.add(tuple(int(t) for t in x))
        return sorted(found)

    gso = GSO.Mat(basis, float_type=ftype)
    LLL.Reduction(gso)()
    bkz = BKZReduction(gso)
    for beta in range(20, beta_max + 1, 10):
        bkz(BKZ.EasyParam(beta, max_loops=8))
        found = candidates()
        if found:
            return found, beta, dim
    return [], beta_max, dim


def main():
    good = 0
    for level, tag, m, beta_max, ftype in RUNS:
        text = (ROOT / f"kem-18/Implementations/Reference_Implementation/Loong{level}/params.h").read_text()
        p = {k: int(v) for k, v in re.findall(r"#define\s+(\w+)\s+(\d+)\b", text)}
        pk, sk, seed = keypair(level, tag)
        f, y, bound = public_system(pk, p)
        pick = np.random.default_rng(int.from_bytes(seed[:4], "little")).choice(len(f), m, replace=False)
        start = time.time()
        found, beta, dim = recover(f[pick], y[pick], bound, p["ETA"], beta_max, ftype)
        elapsed = time.time() - start
        n1, n2 = p["K1"] * p["N"], p["K2"] * p["N"]
        s2 = tuple(int(t) for t in unpack(sk, n1 + n2, 4)[n1:] - p["ETA"])  # only now read sk
        ok = found == [s2]
        good += ok
        print(f"Loong{level} key {tag}: {len(f)} public equations, {m} used, dimension {dim}, "
              f"BKZ-{beta} {elapsed:.0f} s; candidates={len(found)}; equals sk s2={ok} "
              f"({n2} of {n1 + n2} secret coefficients)", flush=True)
    if good == len(RUNS):
        print(f"CONFIRMED kem-18-5: s2 recovered from the public key alone ({good}/{len(RUNS)} fresh keys)")
        return 0
    print(f"NOT CONFIRMED kem-18-5: {good}/{len(RUNS)} recoveries matched")
    return 1


if __name__ == "__main__":
    sys.exit(main())
