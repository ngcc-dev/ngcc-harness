#!/usr/bin/env python3
"""Check Pébereau's public-only Origami-128 forgery against our source-built ABI.

The attack implementation is loaded from a separately cloned, pinned
UnfoldOrigami tree. Its bundled shared library is never loaded here.
"""

import argparse
import ctypes as c
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys
import tempfile
import time


ATTACK_COMMIT = "defa3405d66e763580729b14d6f81c1300fbb219"
ATTACK_URL = "https://github.com/pi-r2/UnfoldOrigami.git"
ATTACK_SOURCE_SHA256 = {
    "forge.sage": "1fe0c4ca114efd217a1e42291f60450dd3a4edd2606b8ef56e411ebc46594917",
    "technical.py": "b287be01bf244ca7cdc749b2bb7c9ca00d756e0eee895bb117d5f722352e9cb2",
}
PK_BYTES = 2996
SK_BYTES = 16


def reproduce(attack_dir: pathlib.Path, lib_path: pathlib.Path) -> None:
    commit = subprocess.check_output(
        ["git", "-C", str(attack_dir), "rev-parse", "HEAD"], text=True
    ).strip()
    if commit != ATTACK_COMMIT:
        raise SystemExit(f"expected UnfoldOrigami commit {ATTACK_COMMIT}, got {commit}")
    for name, expected in ATTACK_SOURCE_SHA256.items():
        source = attack_dir / name
        if not source.is_file() or hashlib.sha256(source.read_bytes()).hexdigest() != expected:
            raise SystemExit(f"UnfoldOrigami source hash mismatch: {name}")
    if not lib_path.is_file():
        raise SystemExit(f"build {lib_path} from the archived Origami source first")

    sys.path.insert(0, str(attack_dir))
    forge = runpy.run_path(str(attack_dir / "forge.sage"))["forge"]

    library = c.CDLL(str(lib_path))
    seed_drng = library.ngcc_seed
    seed_drng.argtypes = [c.c_void_p, c.c_ulonglong]
    seed_drng.restype = c.c_int
    seed = c.create_string_buffer(os.urandom(32))
    if seed_drng(seed, 32) != 0:
        raise SystemExit("official test DRNG initialization failed")
    keygen = library.sig_keygen
    keygen.argtypes = [c.c_void_p, c.c_void_p, c.c_void_p, c.c_void_p]
    keygen.restype = c.c_int
    verify = library.sig_verify
    verify.argtypes = [
        c.c_void_p, c.c_ulonglong, c.c_void_p, c.c_ulonglong,
        c.c_void_p, c.c_ulonglong,
    ]
    verify.restype = c.c_int

    pk, sk = c.create_string_buffer(PK_BYTES), c.create_string_buffer(SK_BYTES)
    pk_len, sk_len = c.c_ulonglong(), c.c_ulonglong()
    if keygen(pk, c.byref(pk_len), sk, c.byref(sk_len)) != 0:
        raise SystemExit("official key generation failed")
    if (pk_len.value, sk_len.value) != (PK_BYTES, SK_BYTES):
        raise SystemExit("unexpected Origami-128 key lengths")
    c.memset(c.addressof(sk), 0, SK_BYTES)

    message = b"NGCC public-only Origami forgery"
    started = time.monotonic()
    forged = forge(pk.raw.hex(), message.hex())
    elapsed = time.monotonic() - started
    signature = c.create_string_buffer(forged)
    message_buf = c.create_string_buffer(message)
    altered = c.create_string_buffer(b"X" + message[1:])
    valid = verify(pk, pk_len.value, signature, len(forged), message_buf, len(message))
    control = verify(pk, pk_len.value, signature, len(forged), altered, len(message))
    if valid != 0 or control == 0:
        raise SystemExit(f"NOT-CONFIRMED: forged verdict={valid}, changed-message control={control}")
    print(
        f"CONFIRMED: public-key-only Origami-128 forgery accepted; "
        f"changed-message control rejected; inversion {elapsed:.2f}s"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("attack_dir", nargs="?", type=pathlib.Path,
                        help="optional local checkout of the pinned attack code")
    parser.add_argument(
        "--lib",
        type=pathlib.Path,
        default=pathlib.Path("sign-18/lib/libOrigami-128.so"),
        help="Origami-128 library built from the archived source",
    )
    args = parser.parse_args()
    lib_path = args.lib.resolve()
    if args.attack_dir:
        reproduce(args.attack_dir.resolve(), lib_path)
        return
    with tempfile.TemporaryDirectory(prefix="ngcc-origami-attack-") as temp:
        attack_dir = pathlib.Path(temp) / "UnfoldOrigami"
        subprocess.run(["git", "clone", "--quiet", ATTACK_URL, str(attack_dir)], check=True)
        subprocess.run(["git", "-C", str(attack_dir), "checkout", "--quiet", "--detach",
                        ATTACK_COMMIT], check=True)
        reproduce(attack_dir, lib_path)


if __name__ == "__main__":
    main()
