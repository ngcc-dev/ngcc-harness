#!/usr/bin/env python3
"""hash-05-4: uHash final-block counters can be cancelled by message padding.

The final iteration of uHash-h XORs the counter 15+j into the last key byte of
branch j (Algorithm 1, line 14).  That byte is also the last byte of the final
padded message chunk.  If a valid 10*-padded chunk ends in the byte 15+j, the
counter cancels, and digest branch j equals branch j of the ordinary (untweaked)
compression of the same chunk with last byte 00.

With b chosen queries X || Z_j, one per branch, an attacker who never learns
the common prefix X therefore obtains the complete chaining value after
X || Z, where Z is any chunk ending in 00.  Public evaluation of uBlock-1024
then predicts H(X || Z || W || T) for unqueried continuations.

The script runs the attack against the submitted uHash-512/768/1024 libraries
(through the CryptHash ABI) and, as controls, against an ideal random function
and with non-cancelling final bytes.  The public block cipher is a pure-Python
transcription of the submitted reference code, checked against the libraries
on independent messages before use.
"""

import ctypes
import os
import random
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LIB = ROOT / "hash-05" / "lib"
ID = "hash-05-4"

# --- uBlock-1024 (CryptHash_AlgorithmInstance.c:36-441) ---------------------

SBOX4 = (3, 0, 6, 2, 5, 4, 15, 14, 10, 8, 7, 9, 1, 12, 13, 11)
SBOX = bytes((SBOX4[x >> 4] << 4) | SBOX4[x & 15] for x in range(256))
PL = (28, 18, 16, 26, 30, 22, 29, 23, 27, 17, 24, 31, 25, 19, 21, 20,
      3, 10, 4, 15, 14, 5, 7, 13, 12, 0, 11, 1, 9, 2, 6, 8)
PR = (0, 2, 1, 12, 11, 8, 13, 7, 10, 4, 6, 5, 3, 14, 9, 15,
      22, 18, 27, 23, 24, 31, 20, 28, 29, 17, 16, 19, 25, 30, 26, 21)
PF = (11, 3, 14, 2, 9, 10, 12, 0, 4, 13, 7, 8, 15, 6, 5, 1,
      16, 19, 27, 22, 31, 26, 20, 30, 23, 29, 25, 18, 24, 21, 28, 17)
PG = (23, 3, 31, 2, 0, 20, 21, 15, 13, 29, 14, 30, 12, 28, 22, 1,
      17, 24, 8, 5, 27, 16, 6, 11, 25, 18, 19, 10, 7, 9, 26, 4)
RC = ((0x00, 0x00, 0x00, 0x00), (0x98, 0x8C, 0xC9, 0xDD),
      (0xF0, 0xE4, 0xA1, 0xB5), (0x21, 0x35, 0x70, 0x64),
      (0x83, 0x97, 0xD2, 0xC6), (0xC7, 0xD3, 0x96, 0x82),
      (0x4F, 0x5B, 0x1E, 0x0A), (0x5E, 0x4A, 0x0F, 0x1B),
      (0x7C, 0x68, 0x2D, 0x39), (0x39, 0x2D, 0x68, 0x7C),
      (0xB3, 0xA7, 0xE2, 0xF6), (0xA7, 0xB3, 0xF6, 0xE2),
      (0x8E, 0x9A, 0xDF, 0xCB))
ROUNDS = 52
M32 = 0xFFFFFFFF


def nibble_permute(x, perm):
    nib = []
    for byte in x:
        nib += (byte >> 4, byte & 15)
    out = [nib[i] for i in perm]
    return bytes((out[2 * i] << 4) | out[2 * i + 1] for i in range(16))


def words(x):
    return [int.from_bytes(x[i:i + 4], "big") for i in range(0, 16, 4)]


def rotl(w, r):
    return ((w << r) | (w >> (32 - r))) & M32


def round_function(state, rk):
    pk = bytes(a ^ b for a, b in zip(state, rk))
    x = words(bytes(SBOX[v] for v in pk[:16]))
    y = words(bytes(SBOX[v] for v in pk[16:]))
    y = [a ^ b for a, b in zip(x, y)]
    x = [a ^ rotl(b, 4) for a, b in zip(x, y)]
    y = [a ^ rotl(b, 8) for a, b in zip(y, x)]
    x = [a ^ rotl(b, 8) for a, b in zip(x, y)]
    y = [a ^ rotl(b, 20) for a, b in zip(y, x)]
    x = [a ^ b for a, b in zip(x, y)]
    xb = b"".join(w.to_bytes(4, "big") for w in x)
    yb = b"".join(w.to_bytes(4, "big") for w in y)
    return nibble_permute(xb, PL) + nibble_permute(yb, PR)


def key_schedule(key):
    assert len(key) == 128
    rk = [key[32 * r:32 * r + 32] for r in range(4)]
    m = [key[16 * r:16 * r + 16] for r in range(8)]
    for s in range(1, 13):
        k = [m[2], m[5], m[1], m[0], m[3], m[6], m[7], m[4]]
        k[0] = bytes(a ^ b for a, b in zip(k[0], RC[s] + (0,) * 12))
        m = [nibble_permute(k[i], PF if i % 2 == 0 else PG) for i in range(8)]
        rk += [m[2 * r] + m[2 * r + 1] for r in range(4)]
    return rk


def encrypt(plaintext, key):
    state = bytes(plaintext)
    for rk in key_schedule(key)[:ROUNDS]:
        state = round_function(state, rk)
    return state


# --- uHash-h mode (specification Algorithm 1, physical PDF page 4) ----------

def xor_last(block, value):
    return block[:-1] + bytes([block[-1] ^ value])


def compress(state, chunk, final):
    """CF_bn (final=False) or CF'_bn (final=True) on a list of b 32-byte branches."""
    b = len(state)
    key = b"".join(state[1:]) + chunk
    out = []
    for j in range(b):
        k = xor_last(key, 15 + j) if final else key
        c = encrypt(xor_last(state[0], j), k)
        out.append(bytes(p ^ q for p, q in zip(c, state[0])))
    return out


def initial_value(b):
    bn = 256 * b
    iv = []
    for j in range(b):
        block = bytearray(32)
        block[24 - 8 * j:32 - 8 * j] = bn.to_bytes(8, "big")
        iv.append(bytes(block))
    return iv


def pad(message, bits, rate):
    out = bytearray(message[:(bits + 7) // 8])
    if bits % 8 == 0:
        out.append(0x80)
    else:
        out[-1] = (out[-1] & (0xFF << (8 - bits % 8)) & 0xFF) | (0x80 >> (bits % 8))
    total = (bits // (8 * rate) + 1) * rate
    return bytes(out) + bytes(total - len(out))


def model_hash(b, message, bits):
    rate = 32 * (5 - b)
    padded = pad(message, bits, rate)
    chunks = [padded[i:i + rate] for i in range(0, len(padded), rate)]
    state = initial_value(b)
    for chunk in chunks[:-1]:
        state = compress(state, chunk, False)
    return b"".join(compress(state, chunks[-1], True))


# --- submitted libraries ----------------------------------------------------

def library_oracle(b):
    path = LIB / f"libuHash-{256 * b}.so"
    function = ctypes.CDLL(str(path)).CryptHash
    function.argtypes = (ctypes.c_int, ctypes.c_void_p, ctypes.c_ulonglong, ctypes.c_void_p)
    function.restype = ctypes.c_int

    def digest(message, bits):
        data = bytes(message[:(bits + 7) // 8]) + bytes(8)
        source = (ctypes.c_ubyte * len(data)).from_buffer_copy(data)
        output = (ctypes.c_ubyte * (32 * b))()
        if function(256 * b, source, bits, output) != 0:
            raise RuntimeError(f"uHash-{256 * b}: CryptHash failed")
        return bytes(output)

    return digest


def ideal_oracle(b, rng):
    table = {}

    def digest(message, bits):
        key = (bytes(message[:(bits + 7) // 8]), bits)
        if key not in table:
            table[key] = bytes(rng.getrandbits(8) for _ in range(32 * b))
        return table[key]

    return digest


# --- the attack ----------------------------------------------------------

def cancelling_suffix(z, tweak):
    """Z without its last byte, followed by the shortest bit string that
    10*-pads to the byte `tweak` (clear its lowest set bit; padding restores it)."""
    low = (tweak & -tweak).bit_length() - 1
    return z[:-1] + bytes([tweak ^ (1 << low)]), 8 * (len(z) - 1) + 7 - low


def recover_state(query, b, z, cancel=True):
    """b chosen queries on suffixes of the unknown prefix; returns the
    recovered chaining value after prefix || z."""
    state = []
    for j in range(b):
        if cancel:
            suffix, bits = cancelling_suffix(z, 15 + j)
        else:
            # Mutated control: the final byte pads to 80, so the counter remains.
            suffix, bits = z[:-1], 8 * (len(z) - 1)
        state.append(query(suffix, bits)[32 * j:32 * j + 32])
    return state


def predict(state, rate, continuation, bits):
    padded = pad(continuation, bits, rate)
    chunks = [padded[i:i + rate] for i in range(0, len(padded), rate)]
    for chunk in chunks[:-1]:
        state = compress(state, chunk, False)
    return b"".join(compress(state, chunks[-1], True))


def run_attack(oracle, b, rng, cancel=True):
    rate = 32 * (5 - b)
    prefix = bytes(rng.getrandbits(8) for _ in range(rate * rng.randint(1, 2)))

    def query(suffix, bits):          # the attacker sees only digests
        return oracle(prefix + suffix, 8 * len(prefix) + bits)

    z = bytes(rng.getrandbits(8) for _ in range(rate - 1)) + b"\x00"
    state = recover_state(query, b, z, cancel)
    results = []
    # Target 1: X || Z || T for a short tail T; target 2: X || Z || W || T.
    for chunks in (0, 1):
        tail_bits = rng.randrange(0, 8 * rate)
        cont = bytes(rng.getrandbits(8) for _ in range(rate * chunks + (tail_bits + 7) // 8))
        cont_bits = 8 * rate * chunks + tail_bits
        if cont_bits % 8:
            cont = cont[:-1] + bytes([cont[-1] & (0xFF << (8 - cont_bits % 8)) & 0xFF])
        guess = predict(state, rate, cont, cont_bits)
        actual = query(z + cont, 8 * rate + cont_bits)    # the single check query
        results.append(guess == actual)
    return results


def main():
    missing = [p for p in (LIB / f"libuHash-{h}.so" for h in (512, 768, 1024)) if not p.is_file()]
    if missing:
        print(f"SKIP {ID}: missing {', '.join(str(p) for p in missing)} (build: make -C hash-05)")
        return 77
    rng = random.Random(int.from_bytes(os.urandom(8), "big"))
    ok = True
    for b in (2, 3, 4):
        h = 256 * b
        rate = 32 * (5 - b)
        lib = library_oracle(b)

        # Model sanity: the Python transcription equals the library on
        # independent lengths (empty, partial-byte, one and several chunks).
        lengths = (0, 5, 8 * rate - 1, 8 * rate, 8 * rate + 13, 16 * rate + 8)
        for bits in lengths:
            msg = bytes(rng.getrandbits(8) for _ in range((bits + 7) // 8))
            if bits % 8:
                msg = msg[:-1] + bytes([msg[-1] & (0xFF << (8 - bits % 8)) & 0xFF])
            if model_hash(b, msg, bits) != lib(msg, bits):
                print(f"NOT CONFIRMED {ID}: Python uBlock-1024 model disagrees with uHash-{h}")
                return 1
        print(f"[model]   uHash-{h}: Python model equals library on {len(lengths)} lengths")

        attack = run_attack(lib, b, rng)
        ideal = run_attack(ideal_oracle(b, rng), b, rng)
        mutated = run_attack(lib, b, rng, cancel=False)
        print(f"[attack]  uHash-{h}: {b} queries recover the chaining value; "
              f"predicted X||Z||T: {'MATCH' if attack[0] else 'FAIL'}, "
              f"X||Z||W||T: {'MATCH' if attack[1] else 'FAIL'}")
        print(f"[control] uHash-{h}: ideal random function: "
              f"{'MATCH' if any(ideal) else 'mismatch'}; non-cancelling final byte 80: "
              f"{'MATCH' if any(mutated) else 'mismatch'}")
        ok &= all(attack) and not any(ideal) and not any(mutated)
    if ok:
        print(f"CONFIRMED {ID}: b chosen queries (b=2,3,4) predict unqueried uHash-512/768/1024 "
              "continuations; ideal-function and non-cancelling controls fail as expected")
        return 0
    print(f"NOT CONFIRMED {ID}: prediction or control did not behave as expected")
    return 1


if __name__ == "__main__":
    sys.exit(main())
