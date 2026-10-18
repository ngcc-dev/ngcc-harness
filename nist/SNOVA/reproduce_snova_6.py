#!/usr/bin/env python3
"""Reproduce SNOVA's fresh-process first-verification failure."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MESSAGE = b"SNOVA fresh-process verification witness"


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


def child_generate(library: Path, directory: Path) -> int:
    lib = load(library)
    meta = lib.nist_sig_get_api().contents
    seed = bytes((19 + 37 * i) & 255 for i in range(48))
    if lib.nist_sig_seed(buf(seed)) != 0:
        return 2
    pk, sk = buf(meta.pk), buf(meta.sk)
    if lib.nist_sig_keypair(pk, sk) != 0:
        return 3
    if lib.nist_sig_seed(buf(bytes((23 + 41 * i) & 255 for i in range(48)))) != 0:
        return 4
    sm = buf(meta.sig + len(MESSAGE))
    smlen = ctypes.c_uint64()
    if lib.nist_sig_sign(
        sm, ctypes.byref(smlen), buf(MESSAGE), len(MESSAGE), sk
    ) != 0:
        return 5
    directory.joinpath("pk.bin").write_bytes(bytes(pk))
    directory.joinpath("sm.bin").write_bytes(bytes(sm[:smlen.value]))
    return 0


def child_verify(library: Path, directory: Path) -> int:
    lib = load(library)
    pk = directory.joinpath("pk.bin").read_bytes()
    signed = directory.joinpath("sm.bin").read_bytes()

    def verify() -> tuple[int, bytes]:
        out = buf(len(signed))
        outlen = ctypes.c_uint64()
        rc = lib.nist_sig_open(
            out, ctypes.byref(outlen), buf(signed), len(signed), buf(pk)
        )
        recovered = bytes(out[:outlen.value]) if rc == 0 else b""
        return rc, recovered

    first_rc, first_message = verify()
    second_rc, second_message = verify()
    directory.joinpath("verify.json").write_text(
        json.dumps({
            "first_rc": first_rc,
            "first_message_matches": first_message == MESSAGE,
            "second_rc": second_rc,
            "second_message_matches": second_message == MESSAGE,
        }) + "\n",
        encoding="utf-8",
    )
    return 0


def run_one(entry: dict, work: Path) -> dict:
    library = ROOT / entry["library"]
    case = work / entry["parameter"]
    case.mkdir()
    command = [sys.executable, str(Path(__file__).resolve())]
    generated = subprocess.run(
        [*command, "--child-generate", str(library), str(case)],
        cwd=ROOT, capture_output=True, text=True,
    )
    if generated.returncode:
        raise RuntimeError(
            f'{entry["parameter"]}: generation failed: '
            f"{generated.stdout}{generated.stderr}"
        )
    verified = subprocess.run(
        [*command, "--child-verify", str(library), str(case)],
        cwd=ROOT, capture_output=True, text=True,
    )
    if verified.returncode:
        raise RuntimeError(
            f'{entry["parameter"]}: verification child failed: '
            f"{verified.stdout}{verified.stderr}"
        )
    observed = json.loads(case.joinpath("verify.json").read_text())
    expected = {
        "first_rc": -1,
        "first_message_matches": False,
        "second_rc": 0,
        "second_message_matches": True,
    }
    if observed != expected:
        raise RuntimeError(
            f'{entry["parameter"]}: expected {expected}, observed {observed}'
        )
    return {
        "parameter": entry["parameter"],
        "library": entry["library"],
        "library_sha256": hashlib.sha256(library.read_bytes()).hexdigest(),
        **observed,
    }


def parent(output: Path | None) -> int:
    entries = json.loads((ROOT / "SNOVA/libraries.json").read_text())
    if len(entries) != 18:
        raise RuntimeError(f"expected 18 SNOVA reference libraries, found {len(entries)}")
    missing = [entry["library"] for entry in entries if not (ROOT / entry["library"]).is_file()]
    if missing:
        built = subprocess.run(
            ["make", "-C", str(ROOT / "SNOVA"), "libs"],
            cwd=ROOT, capture_output=True, text=True,
        )
        if built.returncode:
            raise RuntimeError(
                "could not build required SNOVA libraries:\n"
                + built.stdout + built.stderr
            )
    with tempfile.TemporaryDirectory(prefix="snova-6-") as temporary:
        rows = [run_one(entry, Path(temporary)) for entry in entries]
    result = {
        "format": "SNOVA-6-fresh-process-witness-v1",
        "status": "PASS",
        "scope": "18 reference API variants (nine K/B/S choices times SHAKE/AES)",
        "results": rows,
        "observation": (
            "Each fresh verifier rejected its first valid signed message and "
            "accepted the identical input on its second call."
        ),
        "limitations": [
            "The failure is fail-closed and does not produce a forgery.",
            "This witness does not assess concurrent first use or optimized implementations.",
        ],
    }
    rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if output:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    print("PASS SNOVA-6: reproduced first-call rejection in 18/18 fresh processes")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    parser.add_argument("--child-generate", nargs=2, metavar=("LIBRARY", "DIRECTORY"))
    parser.add_argument("--child-verify", nargs=2, metavar=("LIBRARY", "DIRECTORY"))
    args = parser.parse_args()
    if args.child_generate:
        return child_generate(Path(args.child_generate[0]), Path(args.child_generate[1]))
    if args.child_verify:
        return child_verify(Path(args.child_verify[0]), Path(args.child_verify[1]))
    return parent(args.output)


if __name__ == "__main__":
    raise SystemExit(main())
