#!/usr/bin/env python3
"""Replayable sampled-ciphertext decapsulation return/timing probe."""

import argparse
import ctypes
import hashlib
import json
import pathlib
import time


U8P = ctypes.POINTER(ctypes.c_ubyte)
ULLP = ctypes.POINTER(ctypes.c_ulonglong)


def arr(n, data=None):
    value = (ctypes.c_ubyte * n)()
    if data is not None:
        value[:] = data
    return value


def seed(lib, label):
    raw = hashlib.sha512(("ngcc-kem-oracle:" + label).encode()).digest()
    value = arr(len(raw), raw)
    if lib.ngcc_seed(value, len(raw)):
        raise RuntimeError("ngcc_seed failed")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("library", type=pathlib.Path)
    ap.add_argument("--samples", type=int, default=32)
    ap.add_argument("--bits", help="comma-separated bit positions (overrides --samples)")
    args = ap.parse_args()
    lib = ctypes.CDLL(str(args.library.resolve()))
    lib.ngcc_seed.argtypes, lib.ngcc_seed.restype = [U8P, ctypes.c_ulonglong], ctypes.c_int
    for kind in ("pk", "sk", "ct", "ss"):
        fn = getattr(lib, f"kem_get_{kind}_len_bytes")
        fn.restype = ctypes.c_ulonglong
    lib.kem_keygen.argtypes = [U8P, ULLP, U8P, ULLP]
    lib.kem_enc.argtypes = [U8P, ctypes.c_ulonglong, U8P, ULLP, U8P, ULLP]
    lib.kem_dec.argtypes = [U8P, ctypes.c_ulonglong, U8P, ctypes.c_ulonglong, U8P, ULLP]
    sizes = {x: int(getattr(lib, f"kem_get_{x}_len_bytes")()) for x in ("pk", "sk", "ct", "ss")}
    pk, sk, ct, ss = (arr(sizes[x]) for x in ("pk", "sk", "ct", "ss"))
    pkn, skn, ctn, ssn = (ctypes.c_ulonglong(sizes[x]) for x in ("pk", "sk", "ct", "ss"))
    seed(lib, "keygen")
    kg = lib.kem_keygen(pk, ctypes.byref(pkn), sk, ctypes.byref(skn))
    seed(lib, "enc")
    enc = lib.kem_enc(pk, pkn.value, ss, ctypes.byref(ssn), ct, ctypes.byref(ctn))
    records = []
    total_bits = ctn.value * 8
    bits = ([int(value) for value in args.bits.split(",")]
            if args.bits else
            [sample * (total_bits - 1) // max(args.samples - 1, 1)
             for sample in range(args.samples)])
    if any(bit < 0 or bit >= total_bits for bit in bits):
        raise ValueError(f"bit position outside [0,{total_bits})")
    for sample, bit in enumerate(bits):
        changed = bytearray(ct[:ctn.value])
        changed[bit // 8] ^= 1 << (bit % 8)
        changed_ct, out, outn = arr(ctn.value, changed), arr(sizes["ss"]), ctypes.c_ulonglong(sizes["ss"])
        seed(lib, f"dec:{sample}")
        t0 = time.perf_counter_ns()
        rc = lib.kem_dec(sk, skn.value, changed_ct, ctn.value, out, ctypes.byref(outn))
        elapsed = time.perf_counter_ns() - t0
        records.append({"sample": sample, "bit": bit, "rc": rc,
                        "time_ns": elapsed, "matches_valid_ss": bytes(out[:outn.value]) == bytes(ss[:ssn.value])})
        print(json.dumps(records[-1], sort_keys=True), flush=True)
    counts = {}
    for record in records:
        counts[str(record["rc"])] = counts.get(str(record["rc"]), 0) + 1
    print(json.dumps({"terminal": "NGCC_KEM_MUTATION_ORACLE", "library": str(args.library),
                      "keygen_rc": kg, "enc_rc": enc, "sizes": sizes,
                      "return_codes": counts, "records": records}, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
