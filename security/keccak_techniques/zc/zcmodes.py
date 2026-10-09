#!/usr/bin/env python3
"""Python models of the three ZC modes as implemented (hash-30/31/32), byte-aligned messages."""
from zc import ZC, RC_IMPL


def pad(msg: bytes, rate_bytes: int):
    bits = []
    for b in msg:
        bits += [(b >> (7 - k)) & 1 for k in range(8)]
    bits += [0, 1, 1]
    r = rate_bytes * 8
    while (len(bits) + 1) % r:
        bits.append(0)
    bits.append(1)
    out = bytearray(len(bits) // 8)
    for i, v in enumerate(bits):
        if v:
            out[i >> 3] |= 1 << (7 - (i & 7))
    return [bytes(out[i:i + rate_bytes]) for i in range(0, len(out), rate_bytes)]


def xor_into(z, A, data, offset):
    s = bytearray(z.to_bytes(A))
    for i, v in enumerate(data):
        s[offset + i] ^= v
    return z.from_bytes(bytes(s))


def zc_hash(mode, width, n, msg, rounds=12):
    z = ZC(width)
    c = n + 64
    rb = (width - c) // 8
    rcs = RC_IMPL[12 - rounds:]
    half = RC_IMPL[6:]
    A = [0] * z.N
    for blk in pad(msg, rb):
        A = xor_into(z, A, blk, 0)
        old = list(A)
        if mode == "DM":
            A = z.perm(A, rcs)
            A = [a ^ b for a, b in zip(A, old)]
        elif mode == "DMC":
            A = z.perm(A, rcs)
            A = xor_into(z, A, z.to_bytes(old)[rb:], rb)
        elif mode == "EDMC":   # implemented: h(h(X) ^ (0||X_c)), h = last 6 rounds
            A = z.perm(A, half)
            A = xor_into(z, A, z.to_bytes(old)[rb:], rb)
            A = z.perm(A, half)
    out_rate = 8 if mode == "DMC" else rb
    out = b""
    while len(out) < n // 8:
        out += z.to_bytes(A)[:out_rate]
        if len(out) < n // 8:
            A = z.perm(A, rcs)
    return out[:n // 8]
