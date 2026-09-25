#!/usr/bin/env python3
"""Show that DKEM's rejected-ciphertext key ignores the c1 component."""

import argparse
import ctypes
import hashlib


U8 = ctypes.c_ubyte
U8P = ctypes.POINTER(U8)
ULL = ctypes.c_ulonglong
ULLP = ctypes.POINTER(ULL)


def array(data):
    return (U8 * len(data)).from_buffer_copy(data)


def flip(data, offset):
    out = bytearray(data)
    out[offset] ^= 1
    return bytes(out)


def check(path):
    lib = ctypes.CDLL(path)
    lib.ngcc_seed.argtypes = [U8P, ULL]
    lib.ngcc_seed.restype = ctypes.c_int
    for field in ("pk", "sk", "ct", "ss"):
        getattr(lib, f"kem_get_{field}_len_bytes").restype = ULL
    lib.kem_keygen.argtypes = [U8P, ULLP, U8P, ULLP]
    lib.kem_keygen.restype = ctypes.c_int
    lib.kem_enc.argtypes = [U8P, ULL, U8P, ULLP, U8P, ULLP]
    lib.kem_enc.restype = ctypes.c_int
    lib.kem_dec.argtypes = [U8P, ULL, U8P, ULL, U8P, ULLP]
    lib.kem_dec.restype = ctypes.c_int

    sizes = {name: int(getattr(lib, f"kem_get_{name}_len_bytes")())
             for name in ("pk", "sk", "ct", "ss")}
    seed = hashlib.sha512(b"DKEM rejected-ciphertext binding witness").digest()
    assert lib.ngcc_seed(array(seed), len(seed)) == 0
    pk, sk = (U8 * sizes["pk"])(), (U8 * sizes["sk"])()
    pk_len, sk_len = ULL(sizes["pk"]), ULL(sizes["sk"])
    assert lib.kem_keygen(pk, ctypes.byref(pk_len), sk, ctypes.byref(sk_len)) == 0
    assert (pk_len.value, sk_len.value) == (sizes["pk"], sizes["sk"])
    ct, expected = (U8 * sizes["ct"])(), (U8 * sizes["ss"])()
    ct_len, ss_len = ULL(sizes["ct"]), ULL(sizes["ss"])
    assert lib.kem_enc(pk, pk_len, expected, ctypes.byref(ss_len),
                       ct, ctypes.byref(ct_len)) == 0
    assert (ct_len.value, ss_len.value) == (sizes["ct"], sizes["ss"])
    sk_bytes, ct_bytes = bytes(sk), bytes(ct)

    def dec(ciphertext):
        result, result_len = (U8 * sizes["ss"])(), ULL(sizes["ss"])
        rc = lib.kem_dec(array(sk_bytes), sizes["sk"], array(ciphertext),
                         sizes["ct"], result, ctypes.byref(result_len))
        assert result_len.value == sizes["ss"]
        return rc, bytes(result)

    honest = dec(ct_bytes)
    first = dec(flip(ct_bytes, 0))
    second = dec(flip(ct_bytes, 1))
    changed_tag = dec(flip(flip(ct_bytes, 0), -1))
    assert honest == (0, bytes(expected))
    assert first[0] == second[0] == changed_tag[0] == 0
    assert first[1] == second[1] != honest[1]
    assert changed_tag[1] != first[1]
    print(f"{path}: ct={sizes['ct']} c1={sizes['ct']-sizes['ss']} c2={sizes['ss']}")
    print("CONFIRMED: distinct rejected c1 values with the same c2 yield the same key")
    print("CONTROL: honest ciphertext and changed c2 yield different keys")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("libraries", nargs="+")
    args = parser.parse_args()
    for path in args.libraries:
        check(path)


if __name__ == "__main__":
    main()
