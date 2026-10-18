#!/usr/bin/env python3
"""Deterministic entropy, parser, and one-bit malleability screen for local libraries."""

import argparse
import ctypes
import hashlib
import json
import pathlib
import sys


class Metadata(ctypes.Structure):
    _fields_ = [
        ("abi_version", ctypes.c_uint32),
        ("scheme", ctypes.c_char_p),
        ("parameter", ctypes.c_char_p),
        ("implementation", ctypes.c_char_p),
        ("public_key_bytes", ctypes.c_uint64),
        ("secret_key_bytes", ctypes.c_uint64),
        ("signature_bytes", ctypes.c_uint64),
    ]


def load(path):
    lib = ctypes.CDLL(str(path))
    lib.nist_sig_get_api.restype = ctypes.POINTER(Metadata)
    lib.nist_sig_seed.argtypes = [ctypes.POINTER(ctypes.c_uint8)]
    lib.nist_sig_keypair.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
    lib.nist_sig_sign.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint64), ctypes.c_void_p,
                                  ctypes.c_uint64, ctypes.c_void_p]
    lib.nist_sig_open.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint64), ctypes.c_void_p,
                                  ctypes.c_uint64, ctypes.c_void_p]
    return lib, lib.nist_sig_get_api().contents


def seed(lib, value):
    raw = hashlib.shake_256(value).digest(48)
    buf = (ctypes.c_uint8 * 48).from_buffer_copy(raw)
    assert lib.nist_sig_seed(buf) == 0


def generate(lib, md, seed_value, message, signatures=8):
    seed(lib, seed_value)
    pk = (ctypes.c_uint8 * md.public_key_bytes)()
    sk = (ctypes.c_uint8 * md.secret_key_bytes)()
    if lib.nist_sig_keypair(pk, sk) != 0:
        raise RuntimeError("keypair failed")
    outputs = []
    msg = (ctypes.c_uint8 * len(message)).from_buffer_copy(message)
    for _ in range(signatures):
        sm = (ctypes.c_uint8 * (len(message) + md.signature_bytes))()
        smlen = ctypes.c_uint64()
        if lib.nist_sig_sign(sm, ctypes.byref(smlen), msg, len(message), sk) != 0:
            raise RuntimeError("sign failed")
        outputs.append(bytes(sm[:smlen.value]))
    return bytes(pk), bytes(sk), outputs


def opens(lib, md, pk, sm):
    pkbuf = (ctypes.c_uint8 * len(pk)).from_buffer_copy(pk)
    smbuf = (ctypes.c_uint8 * len(sm)).from_buffer_copy(sm)
    out = (ctypes.c_uint8 * max(1, len(sm)))()
    outlen = ctypes.c_uint64(0xDEADBEEF)
    rc = lib.nist_sig_open(out, ctypes.byref(outlen), smbuf, len(sm), pkbuf)
    if rc != 0 or outlen.value > len(sm):
        return rc, None
    return rc, bytes(out[:outlen.value])


def survey(path, exhaustive, position_cap, signatures):
    lib, md = load(path)
    message = b"nist-sig3 runtime survey\x00message"
    tag = md.parameter or b"unknown"
    first = generate(lib, md, b"replay:" + tag, message, signatures)
    replay = generate(lib, md, b"replay:" + tag, message, signatures)
    other = generate(lib, md, b"other:" + tag, message, signatures)
    pk, sk, signed = first
    base = signed[0]
    rc, recovered = opens(lib, md, pk, base)
    accepted_same_message = []
    accepted_other_message = []
    stride = 1 if exhaustive else max(1, len(base) // position_cap)
    positions = list(range(0, len(base), stride))
    if len(base) - 1 not in positions:
        positions.append(len(base) - 1)
    for pos in positions:
        changed = bytearray(base)
        changed[pos] ^= 1
        mrc, mout = opens(lib, md, pk, changed)
        if mrc == 0:
            (accepted_same_message if mout == message else accepted_other_message).append(pos)
    malformed = {}
    for name, value in (("truncated", base[:-1]), ("appended_zero", base + b"\x00"),
                        ("all_zero", bytes(len(base)))):
        mrc, mout = opens(lib, md, pk, value)
        malformed[name] = {"accepted": mrc == 0, "same_message": mout == message}
    return {
        "library": str(path),
        "parameter": tag.decode(),
        "sizes": {"pk": md.public_key_bytes, "sk": md.secret_key_bytes,
                  "signature_overhead": md.signature_bytes, "signed_message": len(base)},
        "valid_open": rc == 0 and recovered == message,
        "deterministic_replay": first == replay,
        "different_seed_changes_keypair": (first[0], first[1]) != (other[0], other[1]),
        "consecutive_signature_count": len(signed),
        "consecutive_signatures_unique": len(set(signed)) == len(signed),
        "mutation_positions_tested": len(positions),
        "mutation_stride": stride,
        "accepted_same_message_mutations": accepted_same_message,
        "accepted_other_message_mutations": accepted_other_message,
        "malformed": malformed,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exhaustive", action="store_true", help="flip bit 0 at every signed-message byte")
    parser.add_argument("--positions", type=int, default=512,
                        help="minimum evenly spaced mutation positions per library (default: 512)")
    parser.add_argument("--signatures", type=int, default=8,
                        help="consecutive signatures generated in each replay (default: 8)")
    parser.add_argument("libraries", nargs="*")
    args = parser.parse_args()
    here = pathlib.Path(__file__).resolve().parent
    root = here.parent
    inventory = json.loads((here / "libraries.json").read_text())
    selected = set(args.libraries)
    paths = [root / row["library"] for row in inventory
             if not selected or row["parameter"] in selected]
    if args.positions < 1 or args.signatures < 2:
        parser.error("--positions must be positive and --signatures must be at least 2")
    results = [survey(path, args.exhaustive, args.positions, args.signatures) for path in paths]
    print(json.dumps({"schema": 1, "exhaustive": args.exhaustive, "results": results}, indent=2))
    bad = any(not r["valid_open"] or not r["deterministic_replay"] or
              not r["different_seed_changes_keypair"] or
              not r["consecutive_signatures_unique"] or
              r["accepted_same_message_mutations"] or
              r["accepted_other_message_mutations"] or
              any(v["accepted"] for v in r["malformed"].values()) for r in results)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
