#!/usr/bin/env python3
"""Deterministic correctness and repeated-decapsulation sweep for NGCC KEMs."""

import argparse
import ctypes
import hashlib
import json
import pathlib
import statistics
import time


U8P = ctypes.POINTER(ctypes.c_ubyte)
ULLP = ctypes.POINTER(ctypes.c_ulonglong)


def seed_bytes(domain: str, trial: int) -> bytes:
    return hashlib.sha512(f"ngcc-kem-sweep:{domain}:{trial}".encode()).digest()


def array(n: int):
    return (ctypes.c_ubyte * n)()


def configure(lib):
    lib.ngcc_seed.argtypes = [U8P, ctypes.c_ulonglong]
    lib.ngcc_seed.restype = ctypes.c_int
    for name in ("kem_get_pk_len_bytes", "kem_get_sk_len_bytes",
                 "kem_get_ct_len_bytes", "kem_get_ss_len_bytes"):
        fn = getattr(lib, name)
        fn.argtypes = []
        fn.restype = ctypes.c_ulonglong
    lib.kem_keygen.argtypes = [U8P, ULLP, U8P, ULLP]
    lib.kem_keygen.restype = ctypes.c_int
    lib.kem_enc.argtypes = [U8P, ctypes.c_ulonglong, U8P, ULLP, U8P, ULLP]
    lib.kem_enc.restype = ctypes.c_int
    lib.kem_dec.argtypes = [U8P, ctypes.c_ulonglong, U8P, ctypes.c_ulonglong,
                            U8P, ULLP]
    lib.kem_dec.restype = ctypes.c_int


def reseed(lib, domain: str, trial: int):
    seed = seed_bytes(domain, trial)
    value = (ctypes.c_ubyte * len(seed)).from_buffer_copy(seed)
    rc = lib.ngcc_seed(value, len(seed))
    if rc:
        raise RuntimeError(f"ngcc_seed returned {rc}")


def quantiles_ns(values):
    if not values:
        return {}
    ordered = sorted(values)
    def q(frac):
        return ordered[round(frac * (len(ordered) - 1))]
    return {"min": ordered[0], "median": int(statistics.median(ordered)),
            "p90": q(0.90), "p99": q(0.99), "max": ordered[-1]}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("library", type=pathlib.Path)
    ap.add_argument("--trials", type=int, default=100)
    ap.add_argument("--repeats", type=int, default=2,
                    help="decapsulations of each valid ciphertext")
    ap.add_argument("--stop-on-failure", action="store_true")
    args = ap.parse_args()

    lib = ctypes.CDLL(str(args.library.resolve()))
    configure(lib)
    sizes = {name: int(getattr(lib, f"kem_get_{name}_len_bytes")())
             for name in ("pk", "sk", "ct", "ss")}
    failures = []
    return_codes = {}
    timings = {}
    repeat_changes = 0
    completed = 0

    start = time.monotonic()
    for trial in range(args.trials):
        pk, sk, ct, ss = (array(sizes[x]) for x in ("pk", "sk", "ct", "ss"))
        pkn, skn = ctypes.c_ulonglong(sizes["pk"]), ctypes.c_ulonglong(sizes["sk"])
        ctn, ssn = ctypes.c_ulonglong(sizes["ct"]), ctypes.c_ulonglong(sizes["ss"])

        reseed(lib, "keygen", trial)
        key_rc = lib.kem_keygen(pk, ctypes.byref(pkn), sk, ctypes.byref(skn))
        reseed(lib, "enc", trial)
        enc_rc = lib.kem_enc(pk, pkn.value, ss, ctypes.byref(ssn), ct, ctypes.byref(ctn))
        expected = bytes(ss[:ssn.value])
        records = []
        for repeat in range(args.repeats):
            out, outn = array(sizes["ss"]), ctypes.c_ulonglong(sizes["ss"])
            # A distinct deterministic stream per repeat exposes randomized decapsulation
            # while keeping every verdict independently replayable.
            reseed(lib, f"dec:{repeat}", trial)
            t0 = time.perf_counter_ns()
            dec_rc = lib.kem_dec(sk, skn.value, ct, ctn.value, out, ctypes.byref(outn))
            elapsed = time.perf_counter_ns() - t0
            actual = bytes(out[:min(outn.value, sizes["ss"])])
            good = dec_rc == 0 and outn.value == ssn.value and actual == expected
            records.append((dec_rc, outn.value, actual, good, elapsed))
            return_codes[str(dec_rc)] = return_codes.get(str(dec_rc), 0) + 1
            timings.setdefault(str(dec_rc), []).append(elapsed)

        completed += 1
        if len({(r[0], r[1], r[2]) for r in records}) > 1:
            repeat_changes += 1
        if key_rc or enc_rc or not all(r[3] for r in records):
            failures.append({
                "trial": trial, "keygen_rc": key_rc, "enc_rc": enc_rc,
                "dec": [{"repeat": i, "rc": r[0], "ss_len": r[1],
                         "matches": r[3], "time_ns": r[4]}
                        for i, r in enumerate(records)],
                "seed_recipe": "SHA-512('ngcc-kem-sweep:<domain>:<trial>')",
            })
            print(json.dumps(failures[-1], sort_keys=True), flush=True)
            if args.stop_on_failure:
                break

    result = {
        "terminal": "NGCC_KEM_ROUNDTRIP_SWEEP",
        "library": str(args.library), "sizes": sizes,
        "requested_trials": args.trials, "completed_trials": completed,
        "repeats": args.repeats, "failure_count": len(failures),
        "repeat_change_count": repeat_changes, "return_codes": return_codes,
        "timing_ns_by_return_code": {k: quantiles_ns(v) for k, v in timings.items()},
        "elapsed_seconds": time.monotonic() - start,
        "failures": failures,
    }
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
