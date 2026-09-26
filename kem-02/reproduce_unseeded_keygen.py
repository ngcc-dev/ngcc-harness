#!/usr/bin/env python3
"""Demonstrate the effect of omitting caller-required DRNG seeding."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import subprocess
import sys
from pathlib import Path


def child(library: Path, seed_hex: str | None) -> int:
    lib = ctypes.CDLL(str(library.resolve()))
    u64 = ctypes.c_ulonglong
    lib.kem_get_pk_len_bytes.restype = u64
    lib.kem_get_sk_len_bytes.restype = u64
    if seed_hex is not None:
        seed = bytes.fromhex(seed_hex)
        seed_buf = (ctypes.c_ubyte * len(seed)).from_buffer_copy(seed)
        if lib.ngcc_seed(seed_buf, len(seed)) != 0:
            raise RuntimeError("ngcc_seed failed")

    pk = (ctypes.c_ubyte * lib.kem_get_pk_len_bytes())()
    sk = (ctypes.c_ubyte * lib.kem_get_sk_len_bytes())()
    pk_len = u64()
    sk_len = u64()
    rc = lib.kem_keygen(pk, ctypes.byref(pk_len), sk, ctypes.byref(sk_len))
    if rc != 0:
        raise RuntimeError(f"kem_keygen returned {rc}")
    digest = hashlib.sha256(bytes(pk) + bytes(sk)).hexdigest()
    print(json.dumps({"digest": digest, "pk": pk_len.value, "sk": sk_len.value}))
    return 0


def launch(script: Path, library: Path, seed_hex: str | None = None) -> dict[str, object]:
    command = [sys.executable, str(script), "--child", str(library)]
    if seed_hex is not None:
        command += ["--seed", seed_hex]
    return json.loads(subprocess.check_output(command, text=True))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--child", action="store_true")
    parser.add_argument("library", nargs="?", type=Path)
    parser.add_argument("--seed")
    args = parser.parse_args()
    if args.child:
        if args.library is None:
            parser.error("--child requires a library")
        return child(args.library, args.seed)

    root = Path(__file__).resolve().parent
    libraries = sorted(
        path for path in (root / "lib").glob("libAmoeba*.so")
        if "attack" not in path.name
    )
    if len(libraries) != 5:
        print("expected five Amoeba libraries; run `make -C kem-02`", file=sys.stderr)
        return 2

    for library in libraries:
        first = launch(Path(__file__).resolve(), library)
        second = launch(Path(__file__).resolve(), library)
        seeded_a = launch(Path(__file__).resolve(), library, "01" * 48)
        seeded_b = launch(Path(__file__).resolve(), library, "a5" * 48)
        if first["digest"] != second["digest"]:
            print(f"{library.name}: fresh unseeded keys unexpectedly differ", file=sys.stderr)
            return 1
        if seeded_a["digest"] == seeded_b["digest"]:
            print(f"{library.name}: seeded control keys unexpectedly match", file=sys.stderr)
            return 1
        print(
            f"OBSERVATION kem-02-5 {library.stem.removeprefix('lib')}: "
            f"fresh unseeded key digest {first['digest']}; seeded control differs"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
