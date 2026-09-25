#!/usr/bin/env python3
"""Show that a zero DKEM public vector collapses every sender's key to KDF(0^n)."""

import argparse
import ctypes
import hashlib


U8 = ctypes.c_ubyte
U8P = ctypes.POINTER(U8)
ULL = ctypes.c_ulonglong
ULLP = ctypes.POINTER(ULL)
SEEDBYTES = 32  # trailing rho in the public key (Algorithm 14, line 1)


def array(data):
    return (U8 * len(data)).from_buffer_copy(data)


def sm3_zero_key(length):
    """KDF(0^n) for 32-byte keys (sm3hash); None when hashlib lacks SM3."""
    if length != 32:
        return None  # DKEM-512 uses pseudohash(512), not available in hashlib
    try:
        return hashlib.new("sm3", bytes(length)).digest()
    except ValueError:
        return None


def check(path, trials):
    lib = ctypes.CDLL(path)
    lib.ngcc_seed.argtypes = [U8P, ULL]
    lib.ngcc_seed.restype = ctypes.c_int
    for field in ("pk", "sk", "ct", "ss"):
        getattr(lib, f"kem_get_{field}_len_bytes").restype = ULL
    lib.kem_keygen.argtypes = [U8P, ULLP, U8P, ULLP]
    lib.kem_keygen.restype = ctypes.c_int
    lib.kem_enc.argtypes = [U8P, ULL, U8P, ULLP, U8P, ULLP]
    lib.kem_enc.restype = ctypes.c_int

    sizes = {name: int(getattr(lib, f"kem_get_{name}_len_bytes")())
             for name in ("pk", "sk", "ct", "ss")}
    seed = hashlib.sha512(b"DKEM malicious public key witness").digest()
    assert lib.ngcc_seed(array(seed), len(seed)) == 0
    pk, sk = (U8 * sizes["pk"])(), (U8 * sizes["sk"])()
    pk_len, sk_len = ULL(sizes["pk"]), ULL(sizes["sk"])
    assert lib.kem_keygen(pk, ctypes.byref(pk_len), sk, ctypes.byref(sk_len)) == 0
    honest = bytes(pk)
    # Malicious key: zero polynomial vector u_A, honest-looking trailing rho.
    malicious = bytes(sizes["pk"] - SEEDBYTES) + honest[-SEEDBYTES:]

    def encapsulate(public_key):
        ct, ss = (U8 * sizes["ct"])(), (U8 * sizes["ss"])()
        ct_len, ss_len = ULL(sizes["ct"]), ULL(sizes["ss"])
        assert lib.kem_enc(array(public_key), len(public_key), ss,
                           ctypes.byref(ss_len), ct, ctypes.byref(ct_len)) == 0
        return bytes(ct), bytes(ss)

    control = [encapsulate(honest) for _ in range(trials)]
    attack = [encapsulate(malicious) for _ in range(trials)]
    assert len({ct for ct, _ in control}) == len({k for _, k in control}) == trials
    assert len({ct for ct, _ in attack}) == trials
    assert len({ct[-sizes["ss"]:] for ct, _ in attack}) == trials  # fresh coins in c2
    keys = {k for _, k in attack}
    assert len(keys) == 1
    key = keys.pop()

    print(f"{path}: {trials} encapsulations each")
    print(f"CONTROL: honest public key gives {trials} distinct ciphertexts and keys")
    print(f"CONFIRMED: zero public vector gives {trials} distinct ciphertexts "
          f"but one key {key.hex()[:32]}...")
    expected = sm3_zero_key(sizes["ss"])
    if expected is not None:
        assert key == expected
        print("CONFIRMED: repeated key equals sm3hash(256, 0^32), so the raw secret is zero")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("libraries", nargs="+")
    parser.add_argument("--trials", type=int, default=64)
    args = parser.parse_args()
    for path in args.libraries:
        check(path, args.trials)


if __name__ == "__main__":
    main()
