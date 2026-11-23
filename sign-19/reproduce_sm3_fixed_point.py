#!/usr/bin/env python3
"""sign-19-8: Phoenix-SM3 signed messages can be extended through SM3 fixed points.

Phoenix-SM3 hashes a message as seed = SM3(R || PK.seed || PK.root || M || counter)
and then expands R || PK.seed || seed. SM3 is a Merkle-Damgard hash whose digest
is its last chaining value, and its compression function is V' = E_B(V) xor V
with E_B invertible. For any chosen block B, S = E_B^-1(0) is therefore a
fixed point: CF(S, B) = S. If S equals the seed of a signed message, appending
the old padding and B to that message leaves the seed unchanged, so the old
signature verifies for the longer message. One trial tests every collected
signature at once (Guo, Cheng, Preneel and Wang, ePrint 2026/2624, Section 3.2).

When the signed messages are long, their intermediate chaining states are
targets as well. Strenzke's hourglass attack with state matching (ePrint
2026/2635, Section 3.2) merges the states that follow each signature's prefix
into one state by collisions, appends an expandable message built from a
fixed point, links into any intermediate state of any signed message and
copies that message's tail.

Checks:
  1. the affected source trees compute and use the seed as described;
  2. an independent SM3 matches the standard test vectors, and E_B^-1(0) is a
     fixed point for padding-compatible final blocks;
  3. the forged input layout: SM3(X || pad(X) || D) = CF(SM3(X), D || padding),
     so the forged digest depends on the signed input X only through its seed;
  4. the whole attack on a Davies-Meyer hash with a 20-bit state and the same
     layout: 64 signed messages with distinct R are forged in about 2^20/64
     trials, each forgery keeping its target's R and public key;
  5. the hourglass attack on the same model: 16 long signed messages with
     distinct R, one merged state, and forgeries that keep R, the public key,
     the length and the trailing counter of the message they hit.

The full-size searches are not run. Their costs are 2^256 / signatures inverse
compressions for the fixed-point attack and, from 2026/2635 Equation (4),
2^(132 + k1/2) + 2^129 + 2^(256 - k1 - k2) compressions for 2^k1 signatures on
messages of 2^k2 blocks.
"""
import random
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
BASE = ROOT / "Implementations and Test_Vectors/Implementations"
M32 = 0xFFFFFFFF


def fail(detail):
    print(f"NOT CONFIRMED sign-19-8: {detail}")
    sys.exit(1)


# --- 1. source structure ----------------------------------------------------
def check_sources():
    n = 0
    for family, directory, suffix, expander in (
        ("reference", "Reference_Implementation", "", "mgf1_sm3"),
        ("AVX2", "Optimized_Implementation/avx2", "-avx2", "sm3_xof"),
    ):
        for level in (192, 256, 384, 512):
            for speed in ("s", "f"):
                name = f"Phoenix-SM3-{level}{speed}"
                inst = BASE / directory / (name + suffix)
                h = (inst / "hash_sm3.c").read_text().replace("\r", "")
                sig = (inst / "sign.c").read_text().replace("\r", "")
                params = (inst / "params" / f"params-phoenix-sm3-{level}{speed}.h").read_text()
                header = (inst / "hash_sm3.h").read_text()
                core = (inst / "sm3_drng.c").read_text().replace("\r", "")
                for cond, what in (
                    ("A ^= a;" in core and "H ^= h;" in core, "SM3 feed-forward"),
                    ("padded[inlen] = 0x80;" in core and "out[i] = state[i];" in core,
                     "SM3 padding and unmodified final state"),
                    (f"#define SPX_N {level // 8}" in params, "N"),
                    ("#define COUNTER_SIZE 4" in params, "counter size"),
                    ("#define SPX_SM3_OUTPUT_BYTES 32" in header, "SM3 width"),
                    (h.count("sm3_inc_finalize(seed + 2*SPX_N") == 2, "inner digest"),
                    (f"{expander}(bufp, SPX_DGST_BYTES, seed," in h, "expansion of the seed"),
                    ("memcpy(seed, R, SPX_N);" in h, "R in the expansion"),
                    ("counter = get_fors_counter(sig);" in sig, "counter read from the signature"),
                    ("ull_to_bytes(mtmp + mlen, COUNTER_SIZE, counter);" in sig, "counter appended"),
                    ("hash_message(mhash, &tree, &idx_leaf, sig, pk, mtmp, mlen + COUNTER_SIZE, &ctx);" in sig,
                     "verifier hashes M || counter with R from the signature"),
                ):
                    if not cond:
                        fail(f"{family} {name}: {what} differs from the archive")
                n += 1
    print(f"source: {n} trees take a 32-byte SM3 digest of R || PK || M || counter with the candidate's own SM3 "
          "(xor feed-forward, digest = last chaining value); R and counter come from the signature")


# --- 2. SM3 and its fixed points --------------------------------------------
IV = (0x7380166F, 0x4914B2B9, 0x172442D7, 0xDA8A0600, 0xA96F30BC, 0x163138AA, 0xE38DEE4D, 0xB0FB0E4E)


def rol(x, r):
    r %= 32
    return ((x << r) | (x >> (32 - r))) & M32 if r else x


def p0(x):
    return x ^ rol(x, 9) ^ rol(x, 17)


def p0_inv(y):
    for _ in range(31):  # P0 has order 32: (1 + x^9 + x^17)^32 = 1 mod x^32 + 1
        y = p0(y)
    return y


def p1(x):
    return x ^ rol(x, 15) ^ rol(x, 23)


def expand(block):
    w = list(struct.unpack(">16I", block))
    for j in range(16, 68):
        w.append(p1(w[j - 16] ^ w[j - 9] ^ rol(w[j - 3], 15)) ^ rol(w[j - 13], 7) ^ w[j - 6])
    return w, [w[j] ^ w[j + 4] for j in range(64)]


def ff(j, x, y, z):
    return x ^ y ^ z if j < 16 else (x & y) | (x & z) | (y & z)


def gg(j, x, y, z):
    return x ^ y ^ z if j < 16 else (x & y) | (~x & z & M32)


def tj(j):
    return rol(0x79CC4519 if j < 16 else 0x7A879D8A, j)


def cipher(state, block):
    """E_B: the 64 rounds without the feed-forward."""
    a, b, c, d, e, f, g, h = state
    w, w1 = expand(block)
    for j in range(64):
        ss1 = rol((rol(a, 12) + e + tj(j)) & M32, 7)
        ss2 = ss1 ^ rol(a, 12)
        tt1 = (ff(j, a, b, c) + d + ss2 + w1[j]) & M32
        tt2 = (gg(j, e, f, g) + h + ss1 + w[j]) & M32
        a, b, c, d, e, f, g, h = tt1, a, rol(b, 9), c, p0(tt2), e, rol(f, 19), g
    return (a, b, c, d, e, f, g, h)


def cipher_inv(state, block):
    a2, b2, c2, d2, e2, f2, g2, h2 = state
    w, w1 = expand(block)
    for j in range(63, -1, -1):
        a, b, c, e, f, g = b2, rol(c2, 23), d2, f2, rol(g2, 13), h2
        ss1 = rol((rol(a, 12) + e + tj(j)) & M32, 7)
        ss2 = ss1 ^ rol(a, 12)
        d = (a2 - ff(j, a, b, c) - ss2 - w1[j]) & M32
        h = (p0_inv(e2) - gg(j, e, f, g) - ss1 - w[j]) & M32
        a2, b2, c2, d2, e2, f2, g2, h2 = a, b, c, d, e, f, g, h
    return (a2, b2, c2, d2, e2, f2, g2, h2)


def cf(state, block):
    return tuple(x ^ y for x, y in zip(cipher(state, block), state))


def pad(nbytes):
    return b"\x80" + b"\x00" * ((55 - nbytes) % 64) + struct.pack(">Q", 8 * nbytes)


def sm3(data):
    state = IV
    data = data + pad(len(data))
    for i in range(0, len(data), 64):
        state = cf(state, data[i:i + 64])
    return state


def check_sm3(rng):
    vectors = (
        (b"abc", "66c7f0f462eeedd9d1f2d46bdc10e4e24167c4875cf2f7a2297da02b8f4ba8e0"),
        (b"abcd" * 16, "debe9ff92275b8a138604889c18e5a4d6fdb70e5387e5765293dcba39c0c5732"),
    )
    for msg, want in vectors:
        if struct.pack(">8I", *sm3(msg)).hex() != want:
            fail("the SM3 model does not match the standard test vectors")
    zero = (0,) * 8
    for _ in range(64):
        blocks_before = rng.randrange(2, 9)
        d = rng.randbytes(51) + rng.randbytes(4)  # 51 free bytes and a 4-byte counter
        block = d + b"\x80" + struct.pack(">Q", 8 * (64 * blocks_before + 55))
        s = cipher_inv(zero, block)
        if cipher(s, block) != zero or cf(s, block) != s or cf(cf(s, block), block) != s:
            fail("E_B^-1(0) is not a fixed point of the SM3 compression function")
    print("SM3: test vectors match; E_B^-1(0) is a fixed point for 64 padding-compatible final blocks")


def check_layout(rng):
    for _ in range(32):
        n = 32
        x = rng.randbytes(n) + rng.randbytes(2 * n) + rng.randbytes(rng.randrange(0, 300)) + rng.randbytes(4)
        seed = sm3(x)
        padded = x + pad(len(x))
        d = rng.randbytes(51) + rng.randbytes(4)
        forged = padded + d
        last = d + b"\x80" + struct.pack(">Q", 8 * len(forged))
        if sm3(forged) != cf(seed, last) or not forged.startswith(x[:3 * n]) or forged == x:
            fail("the forged input layout does not reduce to one compression call on the seed")
    print("layout: SM3(X || pad(X) || D) = CF(SM3(X), D || padding) for 32 Phoenix-format inputs X")


# --- 4. the attack at reduced scale ----------------------------------------
BITS = 20
MASK = (1 << BITS) - 1
MUL = 0x9E375
MUL_INV = pow(MUL, -1, 1 << BITS)


def toy_keys(block):
    v = int.from_bytes(block, "big")
    return [((v >> (4 * r)) ^ (v >> (BITS + r)) ^ (0xA5A5A * (r + 1))) & MASK for r in range(12)]


def toy_cipher(s, block):
    for k in toy_keys(block):
        s = (s + k) & MASK
        s = (s * MUL) & MASK
        s ^= s >> 11
    return s


def toy_cipher_inv(s, block):
    for k in reversed(toy_keys(block)):
        s ^= s >> 11
        s = (s * MUL_INV) & MASK
        s = (s - k) & MASK
    return s


def toy_pad(nbytes):
    return b"\x80" + b"\x00" * ((5 - nbytes) % 8) + struct.pack(">H", nbytes)


def toy_hash(data):
    s = 0x5A5A5
    data = data + toy_pad(len(data))
    for i in range(0, len(data), 8):
        s = toy_cipher(s, data[i:i + 8]) ^ s
    return s


def check_toy(rng):
    pk = rng.randbytes(8)
    targets = {}
    max_blocks = 0
    for _ in range(64):
        x = rng.randbytes(8) + pk + rng.randbytes(rng.randrange(3, 20)) + rng.randbytes(1)
        targets.setdefault(toy_hash(x), x)
        max_blocks = max(max_blocks, len(x + toy_pad(len(x))) // 8)
    trials = forgeries = 0
    first = None
    while trials < (1 << 18) and forgeries < 4:
        trials += 1
        d = rng.randbytes(5)  # 4 free bytes and a 1-byte counter
        block = d + b"\x80" + struct.pack(">H", 8 * max_blocks + 5)
        s = toy_cipher_inv(0, block)
        x = targets.get(s)
        if x is None:
            continue
        padded = x + toy_pad(len(x))
        forged = padded + block * (max_blocks - len(padded) // 8) + d
        if toy_hash(forged) != s or forged == x or not forged.startswith(x[:16]):
            fail("a reduced-scale hit did not give a valid extension")
        forgeries += 1
        first = first or trials
    if forgeries < 4:
        fail("the reduced-scale attack found fewer than four forgeries in 2^18 trials")
    print(f"reduced scale: 20-bit state, {len(targets)} signed inputs with distinct R; first forgery after "
          f"{first} trials and four after {trials} (2^20/64 = 16384 expected for each)")


def toy_cf(s, block):
    return toy_cipher(s, block) ^ s


def check_hourglass(rng):
    k1 = 4
    pk = rng.randbytes(8)
    inputs = [rng.randbytes(8) + pk + rng.randbytes(rng.randrange(250, 330)) + rng.randbytes(1)
              for _ in range(1 << k1)]
    table = {}
    leaves = []
    for i, x in enumerate(inputs):
        s = 0x5A5A5
        for j in range(1, (len(x) - 1) // 8 + 1):  # keep a non-empty tail, which holds the counter
            s = toy_cf(s, x[8 * (j - 1):8 * j])
            if j == 2:
                leaves.append(s)  # state after R || PK
            if j >= k1 + 4:
                table.setdefault(s, (i, j))
    if len(set(leaves)) != len(leaves):
        fail("reduced-scale prefix states are not distinct")
    # narrowing half: merge the prefix states pairwise by collisions
    paths = [[] for _ in inputs]
    groups = [([i], leaves[i]) for i in range(len(inputs))]
    work = 0
    while len(groups) > 1:
        merged = []
        for (ia, sa), (ib, sb) in zip(groups[0::2], groups[1::2]):
            seen = {}
            for _ in range(1 << 11):
                b = rng.randbytes(8)
                seen[toy_cf(sa, b)] = b
            while True:
                work += 1
                b = rng.randbytes(8)
                t = toy_cf(sb, b)
                if t in seen:
                    break
            for i in ia:
                paths[i].append(seen[t])
            for i in ib:
                paths[i].append(b)
            merged.append((ia + ib, t))
            work += 1 << 11
        groups = merged
    hstar = groups[0][1]
    # expandable message: one block from h* to a fixed point of another block
    fixed = {}
    for _ in range(1 << 11):
        b = rng.randbytes(8)
        fixed[toy_cipher_inv(0, b)] = b
    while True:
        xblk = rng.randbytes(8)
        s_e = toy_cf(hstar, xblk)
        if s_e in fixed:
            break
    bfp = fixed[s_e]
    # widening half: one search against every eligible state of every message
    trials = forgeries = 0
    while trials < (1 << 17) and forgeries < 3:
        trials += 1
        b = rng.randbytes(8)
        hit = table.get(toy_cf(s_e, b))
        if hit is None:
            continue
        i, j = hit
        x = inputs[i]
        forged = x[:16] + b"".join(paths[i]) + xblk + bfp * (j - k1 - 4) + b + x[8 * j:]
        if (toy_hash(forged) != toy_hash(x) or forged == x or len(forged) != len(x)
                or forged[:16] != x[:16] or forged[-1:] != x[-1:]):
            fail("a reduced-scale hourglass hit did not give a valid forgery")
        forgeries += 1
    if forgeries < 3:
        fail("the reduced-scale hourglass attack found fewer than three forgeries in 2^17 trials")
    print(f"reduced scale, long messages: {len(inputs)} signed inputs with distinct R merged into one state "
          f"({work} compressions), {len(table)} target states; three forgeries after {trials} linking trials "
          f"(2^20/{len(table)} = {(1 << 20) // len(table)} expected for each)")


def hourglass_cost(k1, k2):
    from math import log2
    return log2(2.0 ** (132 + k1 / 2) + 2.0 ** 129 + 2.0 ** (256 - k1 - k2))


def main():
    rng = random.Random(0x19_08)
    check_sources()
    check_sm3(rng)
    check_layout(rng)
    check_toy(rng)
    check_hourglass(rng)
    print("cost: 2^256 / (number of signatures) inverse SM3 compressions; 2^192 at the 2^64 signatures of one key")
    c20, c26 = hourglass_cost(64, 20), hourglass_cost(64, 26)
    if not (171.9 < c20 < 172.1 and 166.2 < c26 < 166.4):
        fail("the hourglass cost formula does not give the published figures")
    print(f"cost, long messages (2026/2635 Eq. 4): 2^{c20:.1f} for 2^64 signatures on 64 MiB messages, "
          f"2^{c26:.1f} on 4 GiB messages; both below 192")
    print("ATTACK sign-19-8: a fixed point of SM3 extends any signed Phoenix-SM3 message under its own signature: CONFIRMED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
