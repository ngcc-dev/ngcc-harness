#!/usr/bin/env python3
"""Independent Python model of the ZC-m (ZuD) permutations used by hash-30/31/32.

State: list of Y*X 64-bit lanes, index y*X + x (plane y, lane x), identical to the
reference `ZuD{1280,1536}_plain64_state.A[]` layout and byte order (little endian).
Round: iota -> chi -> rho -> theta -> rho.  Verified against the reference C in
check_ref.py.
"""
import os
import random

M64 = (1 << 64) - 1

# Implementation (KAT) constant order, round 0 .. 11  (rc12 .. rc1 in ZuD*.h)
RC_IMPL = [0x58, 0x38, 0x3C0, 0xD0, 0x120, 0x14, 0x60, 0x2C, 0x380, 0xF0, 0x1A0, 0x12]
# Specification Table 1.1 order c_{-11..0}
RC_SPEC = [0x58, 0x38, 0x3C0, 0xD0, 0x60, 0x120, 0x14, 0x2C, 0xF0, 0x1A0, 0x380, 0x12]


def rotl(v, n):
    n &= 63
    return ((v << n) | (v >> (64 - n))) & M64 if n else v


class ZC:
    def __init__(self, width):
        self.width = width
        if width == 1280:
            self.Y, self.X = 5, 4
            self.rho = [(0, 0), (1, 5), (3, 24), (0, 1), (2, 3)]   # (lane shift s, bit rot r)
            self.th = (5, 14)
        elif width == 1536:
            self.Y, self.X = 3, 8
            self.rho = [(0, 0), (3, 3), (5, 0)]
            self.th = (20, 56)
        else:
            raise ValueError(width)
        self.N = self.Y * self.X

    # --- steps -------------------------------------------------------------
    def iota(self, A, rc):
        A = list(A)
        A[0] ^= rc
        return A

    def chi(self, A):
        Y, X = self.Y, self.X
        B = list(A)
        for x in range(X):
            col = [A[y * X + x] for y in range(Y)]
            for y in range(Y):
                B[y * X + x] = col[y] ^ ((~col[(y + 1) % Y]) & M64 & col[(y + 2) % Y])
        return B

    def rhof(self, A):
        Y, X = self.Y, self.X
        B = [0] * self.N
        for y in range(Y):
            s, r = self.rho[y]
            for x in range(X):
                B[y * X + x] = rotl(A[y * X + (x - s) % X], r)
        return B

    def rhoinv(self, A):
        Y, X = self.Y, self.X
        B = [0] * self.N
        for y in range(Y):
            s, r = self.rho[y]
            for x in range(X):
                B[y * X + (x - s) % X] = rotl(A[y * X + x], 64 - r)
        return B

    def theta(self, A):
        Y, X = self.Y, self.X
        P = [0] * X
        for x in range(X):
            for y in range(Y):
                P[x] ^= A[y * X + x]
        t1, t2 = self.th
        E = [rotl(P[(x - 1) % X], t1) ^ rotl(P[(x - 1) % X], t2) for x in range(X)]
        return [A[y * X + x] ^ E[x] for y in range(Y) for x in range(X)]

    def lin(self, A):
        return self.rhof(self.theta(self.rhof(A)))

    def round(self, A, rc):
        return self.lin(self.chi(self.iota(A, rc)))

    def perm(self, A, rcs=RC_IMPL):
        for rc in rcs:
            A = self.round(A, rc)
        return A

    # --- symmetries -------------------------------------------------------
    def zrot(self, A, t):
        return [rotl(a, t) for a in A]

    def xtr(self, A, d):
        Y, X = self.Y, self.X
        return [A[y * X + (x - d) % X] for y in range(Y) for x in range(X)]

    def rand(self, rng=random):
        return [rng.getrandbits(64) for _ in range(self.N)]

    # --- byte helpers (little-endian lanes, like the reference) -------------
    def to_bytes(self, A):
        return b"".join(a.to_bytes(8, "little") for a in A)

    def from_bytes(self, b):
        return [int.from_bytes(b[8 * i:8 * i + 8], "little") for i in range(self.N)]


def hw(A):
    return sum(bin(a).count("1") for a in A)
