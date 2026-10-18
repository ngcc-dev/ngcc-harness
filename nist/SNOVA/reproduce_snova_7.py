#!/usr/bin/env python3
"""Reproduce omitted public-key hashing in SNOVA's l=2 alternatives."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import resource
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "SNOVA/submission/SNOVA_Round3/Additional_Implementations/generic"
MESSAGE = b"SNOVA l=2 public-key binding witness"
PARAMETERS = (
    ("cat1", 50, 17, 17, 9842, 150),
    ("cat3", 76, 25, 25, 31266, 218),
    ("cat5", 104, 33, 33, 71890, 290),
)
PINNED = {
    "snova_ref.c": "ef5dc33896f799c19631f888dd8c9192565e219df54e7932849188ae2e7e117c",
    "snova.h": "91b38ca19ec6f12d34fb87a1cf5e299c5e71d4c96199f47e8714472579e5274c",
    "sign.c": "d79c85f77529a7f9d3787ca909c931c76dce14ba09ccf75e6b956bb1b7be8ddc",
}


class Meta(ctypes.Structure):
    _fields_ = [
        ("abi", ctypes.c_uint32),
        ("scheme", ctypes.c_char_p),
        ("parameter", ctypes.c_char_p),
        ("implementation", ctypes.c_char_p),
        ("pk", ctypes.c_uint64),
        ("sk", ctypes.c_uint64),
        ("sig", ctypes.c_uint64),
    ]


def buffer(data: bytes | bytearray | int):
    if isinstance(data, int):
        return (ctypes.c_ubyte * data)()
    return (ctypes.c_ubyte * len(data)).from_buffer_copy(data)


def load(path: Path):
    lib = ctypes.CDLL(str(path))
    bytep = ctypes.POINTER(ctypes.c_ubyte)
    lib.nist_sig_get_api.restype = ctypes.POINTER(Meta)
    lib.nist_sig_seed.argtypes = [bytep]
    lib.nist_sig_seed.restype = ctypes.c_int
    lib.nist_sig_keypair.argtypes = [bytep, bytep]
    lib.nist_sig_keypair.restype = ctypes.c_int
    lib.nist_sig_sign.argtypes = [
        bytep, ctypes.POINTER(ctypes.c_uint64), bytep, ctypes.c_uint64, bytep
    ]
    lib.nist_sig_sign.restype = ctypes.c_int
    lib.nist_sig_open.argtypes = [
        bytep, ctypes.POINTER(ctypes.c_uint64), bytep, ctypes.c_uint64, bytep
    ]
    lib.nist_sig_open.restype = ctypes.c_int
    return lib


def compile_library(cc: str, output: Path, parameter: tuple, hash_pk: bool) -> None:
    name, v, o, m1, _pk, _sig = parameter
    definitions = [
        "-DSNOVA_REF=1", f"-DSNOVA_v={v}", f"-DSNOVA_o={o}",
        "-DSNOVA_q=16", "-DSNOVA_l=2", "-DSNOVA_r=2",
        f"-DSNOVA_m1={m1}", "-DSNOVA_NAME=snova_alt_l2",
        '-DNIST_SIG_SCHEME="SNOVA"',
        f'-DNIST_SIG_PARAMETER="alt-l2-{name}"',
    ]
    if hash_pk:
        definitions.append("-DHASH_PK=1")
    instrumented = output.with_suffix(".c")
    source_text = (SOURCE / "snova_ref.c").read_text(encoding="utf-8")
    marker = "\n\t/**\n\t * Check hashes\n\t */"
    if source_text.count(marker) != 1:
        raise RuntimeError("could not locate SNOVA verification result")
    source_text = source_text.replace(
        marker,
        "\n\tmemcpy(snova_witness_hash, hash_in_GF, GF16_HASH);" + marker,
    )
    source_text = source_text.replace(
        '#include "symmetric.h"',
        '#include "symmetric.h"\nuint8_t snova_witness_hash[GF16_HASH];',
        1,
    )
    source_text += (
        "\nvoid snova_witness_get_hash(uint8_t *out) { "
        "memcpy(out, snova_witness_hash, GF16_HASH); }\n"
    )
    instrumented.write_text(source_text, encoding="utf-8")
    sources = ["symmetric_ref.c", "rng.c", "sign.c", "aes.c"]
    command = [
        cc, "-O2", "-std=c11", "-fPIC", "-shared", "-Wl,-Bsymbolic",
        *definitions, f"-I{SOURCE}", f"-I{ROOT / 'api'}",
        str(instrumented), *(str(SOURCE / source) for source in sources),
        str(ROOT / "api/nist_sig_shim.c"), "-o", str(output),
    ]
    completed = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    if completed.returncode:
        raise RuntimeError(completed.stdout + completed.stderr)


def verify(lib, signed: bytes, public_key: bytes, message: bytes) -> tuple[int, bool]:
    out = buffer(len(signed))
    outlen = ctypes.c_uint64()
    rc = lib.nist_sig_open(
        out, ctypes.byref(outlen), buffer(signed), len(signed), buffer(public_key)
    )
    return rc, rc == 0 and bytes(out[:outlen.value]) == message


def gf16_mul(a: int, b: int) -> int:
    product = 0
    for _ in range(4):
        if b & 1:
            product ^= a
        b >>= 1
        a <<= 1
        if a & 0x10:
            a ^= 0x13
    return product


def gf16_inv(a: int) -> int:
    if not a:
        raise ZeroDivisionError
    for candidate in range(1, 16):
        if gf16_mul(a, candidate) == 1:
            return candidate
    raise AssertionError("GF(16) inverse missing")


def null_vector(columns: list[bytes]) -> list[int]:
    rows = [[column[row] for column in columns] for row in range(len(columns[0]))]
    pivots = []
    pivot_row = 0
    for column in range(len(columns)):
        selected = next((r for r in range(pivot_row, len(rows)) if rows[r][column]), None)
        if selected is None:
            continue
        rows[pivot_row], rows[selected] = rows[selected], rows[pivot_row]
        inverse = gf16_inv(rows[pivot_row][column])
        rows[pivot_row] = [gf16_mul(value, inverse) for value in rows[pivot_row]]
        for row in range(len(rows)):
            if row == pivot_row or not rows[row][column]:
                continue
            factor = rows[row][column]
            rows[row] = [
                value ^ gf16_mul(factor, pivot)
                for value, pivot in zip(rows[row], rows[pivot_row])
            ]
        pivots.append(column)
        pivot_row += 1
        if pivot_row == len(rows):
            break
    free = next(column for column in range(len(columns)) if column not in pivots)
    solution = [0] * len(columns)
    solution[free] = 1
    for row, pivot in reversed(list(enumerate(pivots))):
        solution[pivot] = rows[row][free]
    for row in range(len(rows)):
        total = 0
        for coefficient, value in zip(solution, rows[row]):
            total ^= gf16_mul(coefficient, value)
        if total:
            raise AssertionError("computed vector is not in the kernel")
    return solution


def map_hash(lib, signed: bytes, public_key: bytes, output_elements: int) -> bytes:
    verify(lib, signed, public_key, MESSAGE)
    out = buffer(output_elements)
    lib.snova_witness_get_hash(out)
    return bytes(out)


def substitute_key(lib, signed: bytes, public_key: bytes, o: int) -> bytes:
    """Find a nonzero public-key delta that fixes this signature's map value."""
    output_elements = 4 * o
    baseline = map_hash(lib, signed, public_key, output_elements)
    indexes = list(range(output_elements + 1))
    columns = []
    for index in indexes:
        changed = bytearray(public_key)
        changed[16 + index // 2] ^= 1 << (4 * (index & 1))
        value = map_hash(lib, signed, bytes(changed), output_elements)
        columns.append(bytes(a ^ b for a, b in zip(value, baseline)))
    coefficients = null_vector(columns)
    changed = bytearray(public_key)
    for index, coefficient in zip(indexes, coefficients):
        if not coefficient:
            continue
        byte_index = 16 + index // 2
        changed[byte_index] ^= coefficient << (4 * (index & 1))
    if bytes(changed) == public_key:
        raise AssertionError("public-key substitution made no change")
    if map_hash(lib, signed, bytes(changed), output_elements) != baseline:
        raise AssertionError("substitution changed the evaluated public map")
    return bytes(changed)


def run_case(default_path: Path, hashed_path: Path, parameter: tuple) -> dict:
    name, _v, o, _m1, expected_pk, expected_sig = parameter
    observations = {}
    for mode, path, expected_sk in (
        ("submitted", default_path, 48), ("hash_pk_control", hashed_path, 96)
    ):
        lib = load(path)
        meta = lib.nist_sig_get_api().contents
        sizes = [meta.pk, meta.sk, meta.sig]
        if sizes != [expected_pk, expected_sk, expected_sig]:
            raise RuntimeError(f"{name}/{mode}: unexpected API sizes {sizes}")
        seed = bytes((17 + 29 * i) & 255 for i in range(48))
        if lib.nist_sig_seed(buffer(seed)) != 0:
            raise RuntimeError(f"{name}/{mode}: seeding failed")
        pk, sk = buffer(meta.pk), buffer(meta.sk)
        if lib.nist_sig_keypair(pk, sk) != 0:
            raise RuntimeError(f"{name}/{mode}: key generation failed")
        signing_seed = bytes((31 + 43 * i) & 255 for i in range(48))
        if lib.nist_sig_seed(buffer(signing_seed)) != 0:
            raise RuntimeError(f"{name}/{mode}: signing seed failed")
        signed = buffer(meta.sig + len(MESSAGE))
        signed_len = ctypes.c_uint64()
        if lib.nist_sig_sign(
            signed, ctypes.byref(signed_len), buffer(MESSAGE), len(MESSAGE), sk
        ) != 0:
            raise RuntimeError(f"{name}/{mode}: signing failed")
        public_key = bytes(pk)
        signed_bytes = bytes(signed[:signed_len.value])
        lib.snova_witness_get_hash.argtypes = [ctypes.POINTER(ctypes.c_ubyte)]
        lib.snova_witness_get_hash.restype = None
        substituted = substitute_key(lib, signed_bytes, public_key, o)
        original_rc, original_matches = verify(lib, signed_bytes, public_key, MESSAGE)
        substitute_rc, substitute_matches = verify(lib, signed_bytes, substituted, MESSAGE)
        changed_message = signed_bytes[:meta.sig] + b"X" + signed_bytes[meta.sig + 1:]
        message_rc, _ = verify(lib, changed_message, public_key, MESSAGE)
        observations[mode] = {
            "pk_bytes": meta.pk,
            "sk_bytes": meta.sk,
            "signature_bytes": meta.sig,
            "original_rc": original_rc,
            "original_message_matches": original_matches,
            "substituted_key_rc": substitute_rc,
            "substituted_key_message_matches": substitute_matches,
            "changed_message_rc": message_rc,
            "public_key_changed_bytes": sum(a != b for a, b in zip(public_key, substituted)),
            "library_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        }
    submitted = observations["submitted"]
    control = observations["hash_pk_control"]
    if not (
        submitted["original_rc"] == 0
        and submitted["original_message_matches"]
        and submitted["substituted_key_rc"] == 0
        and submitted["substituted_key_message_matches"]
        and submitted["changed_message_rc"] != 0
        and control["original_rc"] == 0
        and control["original_message_matches"]
        and control["substituted_key_rc"] != 0
        and control["changed_message_rc"] != 0
        and submitted["public_key_changed_bytes"] > 0
    ):
        raise RuntimeError(f"{name}: control or witness failed: {observations}")
    return {"parameter": name, **observations}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    cc = shutil.which("cc")
    if cc is None:
        print("SKIP SNOVA-7: C compiler unavailable")
        return 77
    soft, hard = resource.getrlimit(resource.RLIMIT_STACK)
    wanted = 64 * 1024 * 1024
    if soft != resource.RLIM_INFINITY and soft < wanted:
        resource.setrlimit(
            resource.RLIMIT_STACK,
            (wanted if hard == resource.RLIM_INFINITY else min(wanted, hard), hard),
        )
    source_hashes = {
        name: hashlib.sha256((SOURCE / name).read_bytes()).hexdigest() for name in PINNED
    }
    if source_hashes != PINNED:
        raise RuntimeError(f"submitted source hashes changed: {source_hashes}")
    rows = []
    with tempfile.TemporaryDirectory(prefix="snova-7-") as temporary:
        directory = Path(temporary)
        for parameter in PARAMETERS:
            default = directory / f"{parameter[0]}-submitted.so"
            hashed = directory / f"{parameter[0]}-hash-pk.so"
            compile_library(cc, default, parameter, False)
            compile_library(cc, hashed, parameter, True)
            rows.append(run_case(default, hashed, parameter))
    result = {
        "format": "SNOVA-7-public-key-binding-witness-v1",
        "status": "PASS",
        "source_sha256": source_hashes,
        "results": rows,
        "observation": (
            "All three submitted l=2 configurations accepted one signature "
            "under two distinct public keys with the same evaluation at that "
            "signed input; HASH_PK=1 rejected it."
        ),
        "limitations": [
            "The three l=2 sets are alternatives, not the nine recommended sets.",
            "The witness demonstrates the claimed public-key-binding property, not an EUF-CMA forgery under a challenger-supplied key.",
        ],
    }
    rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    print("PASS SNOVA-7: public-key substitution accepted in 3/3 l=2 alternatives")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
