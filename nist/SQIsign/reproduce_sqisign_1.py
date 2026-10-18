#!/usr/bin/env python3
"""Reproduce equivalent accepted SQIsign response-matrix encodings."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MESSAGE = b"SQIsign response encoding witness"
PARAMETERS = {
    "p324_3": {"fp": 41, "response_bits": 196, "response_bytes": 25},
    "p500_27": {"fp": 64, "response_bits": 302, "response_bytes": 38},
    "p664_17": {"fp": 84, "response_bits": 400, "response_bytes": 51},
}


class Meta(ctypes.Structure):
    _fields_ = [
        ("abi", ctypes.c_uint32), ("scheme", ctypes.c_char_p),
        ("parameter", ctypes.c_char_p), ("implementation", ctypes.c_char_p),
        ("pk", ctypes.c_uint64), ("sk", ctypes.c_uint64),
        ("sig", ctypes.c_uint64),
    ]


def buf(data: bytes | int):
    if isinstance(data, int):
        return (ctypes.c_ubyte * data)()
    return (ctypes.c_ubyte * len(data)).from_buffer_copy(data)


def run(parameter: str) -> dict:
    path = ROOT / "SQIsign/lib" / f"{parameter}.so"
    lib = ctypes.CDLL(str(path))
    lib.nist_sig_get_api.restype = ctypes.POINTER(Meta)
    bytep = ctypes.POINTER(ctypes.c_ubyte)
    lib.nist_sig_seed.argtypes = [bytep]
    lib.nist_sig_seed.restype = ctypes.c_int
    lib.nist_sig_keypair.argtypes = [bytep, bytep]
    lib.nist_sig_keypair.restype = ctypes.c_int
    lib.nist_sig_sign.argtypes = [bytep, ctypes.POINTER(ctypes.c_uint64),
                                  bytep, ctypes.c_uint64, bytep]
    lib.nist_sig_sign.restype = ctypes.c_int
    lib.nist_sig_open.argtypes = [bytep, ctypes.POINTER(ctypes.c_uint64),
                                  bytep, ctypes.c_uint64, bytep]
    lib.nist_sig_open.restype = ctypes.c_int
    meta = lib.nist_sig_get_api().contents
    cfg = PARAMETERS[parameter]
    assert meta.scheme == b"SQIsign" and meta.parameter.decode() == parameter

    seed = bytes((7 + 29 * i) & 255 for i in range(48))
    assert lib.nist_sig_seed(buf(seed)) == 0
    pk, sk = buf(meta.pk), buf(meta.sk)
    assert lib.nist_sig_keypair(pk, sk) == 0
    assert lib.nist_sig_seed(buf(bytes((11 + 29 * i) & 255 for i in range(48)))) == 0
    sm = buf(meta.sig + len(MESSAGE))
    smlen = ctypes.c_uint64()
    assert lib.nist_sig_sign(sm, ctypes.byref(smlen), buf(MESSAGE), len(MESSAGE), sk) == 0
    signed = bytes(sm[:smlen.value])

    def opened(candidate: bytes) -> tuple[bool, bytes]:
        out = buf(len(candidate))
        outlen = ctypes.c_uint64()
        rc = lib.nist_sig_open(out, ctypes.byref(outlen), buf(candidate), len(candidate), pk)
        recovered = bytes(out[:outlen.value]) if rc == 0 and outlen.value <= len(candidate) else b""
        return rc == 0, recovered

    baseline_ok, baseline_message = opened(signed)
    if not baseline_ok or baseline_message != MESSAGE:
        raise RuntimeError(f"{parameter}: baseline rejected")

    matrix_offset = 2 * cfg["fp"]
    response_bits = cfg["response_bits"]
    response_bytes = cfg["response_bytes"]
    matrix_bits = response_bits + 2
    modulus = 1 << matrix_bits
    midpoint = 1 << (matrix_bits - 1)

    def get_matrix(candidate: bytes) -> list[int]:
        return [
            int.from_bytes(
                candidate[matrix_offset + field * response_bytes:
                          matrix_offset + (field + 1) * response_bytes],
                "little",
            )
            for field in range(4)
        ]

    def put_matrix(candidate: bytes, matrix: list[int]) -> bytes:
        changed = bytearray(candidate)
        for field, value in enumerate(matrix):
            start = matrix_offset + field * response_bytes
            changed[start:start + response_bytes] = (value % modulus).to_bytes(
                response_bytes, "little"
            )
        return bytes(changed)

    def is_normalized(matrix: list[int]) -> bool:
        for value in matrix:
            if value > midpoint:
                return False
            if 0 < value < midpoint:
                return True
        return True

    def normalize(matrix: list[int]) -> list[int]:
        reduced = [value % modulus for value in matrix]
        if is_normalized(reduced):
            return reduced
        return [(-value) % modulus for value in reduced]

    # The verified alias group has 64 elements: sixteen independent top-bit
    # choices, a sign choice for one column (global sign is normalized away),
    # and multiplication of column 0 by 1+2^RESPONSE_BITS.
    matrix = get_matrix(signed)
    aliases: dict[bytes, dict[str, int | bool]] = {}
    scale = 1 + (1 << response_bits)
    for negate_column_zero in (False, True):
        for scale_column_zero in (1, scale):
            base = list(matrix)
            for field in (0, 2):
                base[field] = base[field] * scale_column_zero
                if negate_column_zero:
                    base[field] = -base[field]
            for top_bit_mask in range(16):
                transformed = [
                    value ^ (midpoint if top_bit_mask & (1 << field) else 0)
                    for field, value in enumerate(normalize(base))
                ]
                transformed = normalize(transformed)
                candidate = put_matrix(signed, transformed)
                aliases[candidate] = {
                    "negated_column_zero": negate_column_zero,
                    "scaled_column_zero": scale_column_zero != 1,
                    "top_bit_mask": top_bit_mask,
                }

    if len(aliases) != 64:
        raise RuntimeError(f"{parameter}: expected 64 distinct matrix encodings, got {len(aliases)}")
    rejected_on_changed_message = 0
    for candidate in aliases:
        accepts, recovered = opened(candidate)
        if not accepts or recovered != MESSAGE:
            raise RuntimeError(f"{parameter}: matrix alias rejected")
        changed_message_candidate = bytearray(candidate)
        changed_message_candidate[-1] ^= 1
        changed_accepts, _ = opened(bytes(changed_message_candidate))
        rejected_on_changed_message += not changed_accepts
    if rejected_on_changed_message != 64:
        raise RuntimeError(f"{parameter}: a matrix alias accepted a changed message")

    # A lower scalar bit is an ordinary response change and must be rejected.
    control_bit = 8 * matrix_offset + cfg["response_bits"] - 1
    control = bytearray(signed)
    control[control_bit // 8] ^= 1 << (control_bit & 7)
    lower_control_ok, lower_control_message = opened(bytes(control))
    lower_control_accepted = lower_control_ok and lower_control_message == MESSAGE
    if lower_control_accepted:
        raise RuntimeError(f"{parameter}: lower-bit control accepted")

    return {
        "parameter": parameter,
        "library": str(path.relative_to(ROOT)),
        "library_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        "response_bits": response_bits,
        "response_bytes": response_bytes,
        "matrix_offset": matrix_offset,
        "distinct_matrix_encodings": len(aliases),
        "matrix_encodings_accepted": 64,
        "matrix_encodings_rejected_on_changed_message": rejected_on_changed_message,
        "transform_families": {
            "independent_entry_top_bits": 16,
            "column_zero_signs_modulo_global_normalization": 2,
            "column_zero_scales": [1, scale],
        },
        "lower_bit_control": {"signed_message_bit": control_bit,
                              "accepted_same_message": lower_control_accepted},
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    results = [run(parameter) for parameter in PARAMETERS]
    document = {
        "format": "SQIsign-1-runtime-witness-v1",
        "status": "PASS",
        "results": results,
        "limitations": [
            "This demonstrates same-message signature malleability, not a fresh-message forgery.",
            "SQIsign and the NIST call claim/require EUF-CMA, not strong unforgeability.",
            "No key recovery, entropy failure, or side-channel attack is demonstrated.",
        ],
    }
    rendered = json.dumps(document, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    print("PASS SQIsign-1: all 64 response-matrix encodings accepted for every parameter set")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
