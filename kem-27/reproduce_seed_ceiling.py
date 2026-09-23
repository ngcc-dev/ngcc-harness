#!/usr/bin/env python3
"""Scaled NTRE-512 secret-seed search against a public key.

The submitted NTRE-512 samples its private polynomial f from a 32-byte seed.
This witness restricts that seed to a small subspace, then identifies f using
only the public h = g/f and the specified small-coefficient distribution of g.
It also decapsulates fresh, honest KEM ciphertexts with the recovered f.

This is a scaled search, not an execution of the full 2^256 attack.
"""

from __future__ import annotations

import argparse
import ctypes as C
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile
import time


N = 2304
Q = 3457
SEED_BYTES = 32
SAMPLE_BYTES = N // 4
PK_BYTES = 3456
SK_BYTES = 6944
SS_BYTES = 64

ROOT = Path(__file__).resolve().parents[1]
IMPL = ROOT / "kem-27/Implementations/Reference_Implementation/NTRE-512"


class Poly(C.Structure):
    _fields_ = [("coeffs", C.c_int16 * N)]


def check_parameters() -> None:
    params = (IMPL / "src/params.h").read_text()
    expected = {"NTRE_N": N, "NTRE_Q": Q, "NTRE_SYMBYTES": SEED_BYTES,
                "NTRE_POLYBYTES": PK_BYTES, "NTRE_SSBYTES": SS_BYTES}
    for name, value in expected.items():
        match = re.search(r"^#define\s+" + name + r"\s+(\d+)\b", params, re.M)
        if match is None or int(match.group(1)) != value:
            raise RuntimeError(f"unexpected {name}; witness needs review")


def build_library(output: Path, compiler: str) -> None:
    compiler_command = shlex.split(compiler)
    if not compiler_command or shutil.which(compiler_command[0]) is None:
        raise RuntimeError(f"C compiler not found: {compiler}")
    files = [
        IMPL / "src/poly.c",
        IMPL / "src/ntt.c",
        IMPL / "src/symmetric.c",
        IMPL / "KEM_AlgorithmInstance.c",
        IMPL / "auxfunc.c",
        IMPL / "drng.c",
        ROOT / "api/link_shim.c",
    ]
    command = [
        *compiler_command, "-O2", "-std=gnu11", "-fPIC", "-shared", "-fwrapv",
        "-fno-strict-aliasing", "-DNGCC_BUILD_KEM",
        "-I", str(IMPL), "-I", str(IMPL / "src"), "-I", str(ROOT / "api"),
        "-o", str(output), *(str(path) for path in files), "-lm",
    ]
    result = subprocess.run(command, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"temporary candidate build failed:\n{result.stderr}")


def configure_library(path: Path) -> C.CDLL:
    lib = C.CDLL(str(path))
    byte_ptr = C.POINTER(C.c_ubyte)
    poly_ptr = C.POINTER(Poly)
    u64_ptr = C.POINTER(C.c_ulonglong)
    signatures = {
        "sample_psi1": ([byte_ptr, byte_ptr], None),
        "poly_cbd1": ([poly_ptr, byte_ptr], None),
        "poly_double_add_one": ([poly_ptr, poly_ptr], None),
        "poly_double": ([poly_ptr, poly_ptr], None),
        "poly_ntt": ([poly_ptr], None),
        "poly_invntt": ([poly_ptr], None),
        "poly_baseinv": ([poly_ptr, poly_ptr], C.c_int),
        "poly_basemul": ([poly_ptr, poly_ptr, poly_ptr], None),
        "poly_tobytes": ([byte_ptr, poly_ptr], None),
        "poly_frombytes": ([poly_ptr, byte_ptr], None),
        "hash_G": ([byte_ptr, byte_ptr], None),
        "ngcc_seed": ([byte_ptr, C.c_ulonglong], C.c_int),
        "kem_enc": ([byte_ptr, C.c_ulonglong, byte_ptr, u64_ptr,
                     byte_ptr, u64_ptr], C.c_int),
        "kem_dec": ([byte_ptr, C.c_ulonglong, byte_ptr, C.c_ulonglong,
                     byte_ptr, u64_ptr], C.c_int),
    }
    for name, (argtypes, restype) in signatures.items():
        function = getattr(lib, name)
        function.argtypes = argtypes
        function.restype = restype
    for name, expected in (("kem_get_pk_len_bytes", PK_BYTES),
                           ("kem_get_sk_len_bytes", SK_BYTES),
                           ("kem_get_ss_len_bytes", SS_BYTES),
                           ("kem_get_ct_len_bytes", PK_BYTES)):
        function = getattr(lib, name)
        function.restype = C.c_ulonglong
        if function() != expected:
            raise RuntimeError(f"unexpected {name} from compiled candidate")
    return lib


def seed_from_small_integer(value: int) -> C.Array:
    return (C.c_ubyte * SEED_BYTES).from_buffer_copy(
        value.to_bytes(SEED_BYTES, "little"))


def sample_poly(lib: C.CDLL, seed: C.Array) -> Poly:
    stream = (C.c_ubyte * SAMPLE_BYTES)()
    result = Poly()
    lib.sample_psi1(stream, seed)
    lib.poly_cbd1(C.byref(result), stream)
    return result


def derive_f(lib: C.CDLL, seed: C.Array) -> Poly:
    f = Poly()
    sampled = sample_poly(lib, seed)
    lib.poly_double_add_one(C.byref(f), C.byref(sampled))
    lib.poly_ntt(C.byref(f))
    return f


def derive_g(lib: C.CDLL, seed: C.Array) -> Poly:
    g = Poly()
    sampled = sample_poly(lib, seed)
    lib.poly_double(C.byref(g), C.byref(sampled))
    lib.poly_ntt(C.byref(g))
    return g


def public_key_from_seeds(lib: C.CDLL, f_seed: C.Array,
                          g_seed: C.Array) -> tuple[C.Array, Poly]:
    f = derive_f(lib, f_seed)
    f_inverse = Poly()
    if lib.poly_baseinv(C.byref(f_inverse), C.byref(f)) != 0:
        raise RuntimeError("planted f is not invertible")
    g = derive_g(lib, g_seed)
    h = Poly()
    lib.poly_basemul(C.byref(h), C.byref(g), C.byref(f_inverse))
    pk = (C.c_ubyte * PK_BYTES)()
    lib.poly_tobytes(pk, C.byref(h))
    return pk, f


def is_small_even_g(lib: C.CDLL, h: Poly, f: Poly, scratch: Poly) -> bool:
    lib.poly_basemul(C.byref(scratch), C.byref(h), C.byref(f))
    lib.poly_invntt(C.byref(scratch))
    return all(coefficient in (-2, 0, 2) for coefficient in scratch.coeffs)


def reconstructed_secret_key(lib: C.CDLL, pk: C.Array, f: Poly) -> C.Array:
    sk = (C.c_ubyte * SK_BYTES)()
    f_bytes = (C.c_ubyte * PK_BYTES)()
    h_pk = (C.c_ubyte * 32)()
    lib.poly_tobytes(f_bytes, C.byref(f))
    lib.hash_G(h_pk, pk)
    sk[:PK_BYTES] = f_bytes[:]
    sk[PK_BYTES:2 * PK_BYTES] = pk[:]
    sk[2 * PK_BYTES:] = h_pk[:]
    return sk


def check_shared_secrets(lib: C.CDLL, pk: C.Array, sk: C.Array,
                         wrong_sk: C.Array,
                         count: int) -> None:
    drng_seed = (C.c_ubyte * 48).from_buffer_copy(bytes(range(48)))
    if lib.ngcc_seed(drng_seed, len(drng_seed)) != 0:
        raise RuntimeError("DRNG initialization failed")
    for _ in range(count):
        ct = (C.c_ubyte * PK_BYTES)()
        honest_ss = (C.c_ubyte * SS_BYTES)()
        recovered_ss = (C.c_ubyte * SS_BYTES)()
        ss_len = C.c_ulonglong()
        ct_len = C.c_ulonglong()
        if lib.kem_enc(pk, PK_BYTES, honest_ss, C.byref(ss_len),
                       ct, C.byref(ct_len)) != 0:
            raise RuntimeError("honest encapsulation failed")
        if (ss_len.value, ct_len.value) != (SS_BYTES, PK_BYTES):
            raise RuntimeError("unexpected honest encapsulation lengths")
        recovered_len = C.c_ulonglong()
        if lib.kem_dec(sk, SK_BYTES, ct, PK_BYTES, recovered_ss,
                       C.byref(recovered_len)) != 0:
            raise RuntimeError("recovered-key decapsulation rejected")
        if recovered_len.value != SS_BYTES or bytes(honest_ss) != bytes(recovered_ss):
            raise RuntimeError("recovered-key shared secret mismatch")
        wrong_ss = (C.c_ubyte * SS_BYTES)()
        wrong_len = C.c_ulonglong()
        wrong_rc = lib.kem_dec(wrong_sk, SK_BYTES, ct, PK_BYTES,
                               wrong_ss, C.byref(wrong_len))
        if wrong_rc == 0 or bytes(wrong_ss) == bytes(honest_ss):
            raise RuntimeError("wrong-secret-key control unexpectedly succeeded")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed-bits", type=int, default=12,
                        help="scaled seed-search width, 1..16 (default: 12)")
    parser.add_argument("--target", type=int, default=0xABC,
                        help="planted f-seed integer (default: 0xabc)")
    parser.add_argument("--g-seed", type=int, default=77,
                        help="independent planted g-seed integer (default: 77)")
    parser.add_argument("--encapsulations", type=int, default=10,
                        help="fresh KEM shared-secret checks (default: 10)")
    parser.add_argument("--cc", default=os.environ.get("CC", "cc"),
                        help="C compiler (default: CC or cc)")
    args = parser.parse_args()
    if not 1 <= args.seed_bits <= 16:
        parser.error("--seed-bits must be in 1..16")
    if not 0 <= args.target < 1 << args.seed_bits:
        parser.error("--target must fit in --seed-bits")
    if not 0 <= args.g_seed < 1 << 256:
        parser.error("--g-seed must fit in 256 bits")
    if not 1 <= args.encapsulations <= 100:
        parser.error("--encapsulations must be in 1..100")

    check_parameters()
    with tempfile.TemporaryDirectory(prefix="ntre-seed-ceiling-") as temporary:
        library = Path(temporary) / "libntre512.so"
        build_library(library, args.cc)
        lib = configure_library(library)
        pk, planted_f = public_key_from_seeds(
            lib, seed_from_small_integer(args.target),
            seed_from_small_integer(args.g_seed))
        h = Poly()
        lib.poly_frombytes(C.byref(h), pk)
        scratch = Poly()
        if not is_small_even_g(lib, h, planted_f, scratch):
            raise RuntimeError("planted f does not pass the public verification")

        started = time.monotonic()
        hits: list[tuple[int, Poly]] = []
        for candidate in range(1 << args.seed_bits):
            f = derive_f(lib, seed_from_small_integer(candidate))
            if is_small_even_g(lib, h, f, scratch):
                hits.append((candidate, f))
        seconds = time.monotonic() - started
        if len(hits) != 1 or hits[0][0] != args.target:
            raise RuntimeError(f"unexpected search result: {[hit[0] for hit in hits]}")
        recovered_seed, recovered_f = hits[0]
        if bytes(recovered_f) != bytes(planted_f):
            raise RuntimeError("the recovered polynomial differs from planted f")
        sk = reconstructed_secret_key(lib, pk, recovered_f)
        wrong_f = derive_f(lib, seed_from_small_integer(args.target ^ 1))
        wrong_sk = reconstructed_secret_key(lib, pk, wrong_f)
        check_shared_secrets(lib, pk, sk, wrong_sk, args.encapsulations)

    print("CONFIRMED kem-27-1 NTRE-512 scaled public-key seed search: "
          f"space=2^{args.seed_bits} recovered_seed={recovered_seed} "
          f"unique_match=1 honest_shared_secrets={args.encapsulations}/"
          f"{args.encapsulations} wrong_key_rejections={args.encapsulations}/"
          f"{args.encapsulations} search_seconds={seconds:.3f}")
    print("LIMITATION: the actual 32-byte f-seed space is 2^256; "
          "this witness restricts only its low bits and does not execute "
          "the full search.")


if __name__ == "__main__":
    main()
