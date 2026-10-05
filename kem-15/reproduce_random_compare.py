#!/usr/bin/env python3
"""Exercise the submitted AVX2 and reference ciphertext comparisons."""

from __future__ import annotations

import ctypes
import random
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SOURCES = ROOT / "Implementations"


def load_verify(source: Path, output: Path, optimized: bool):
    flags = ["-O3", "-shared", "-fPIC"]
    if optimized:
        flags.extend(("-mavx2", "-msse4.1"))
    subprocess.run(
        ["cc", *flags, str(source), "-o", str(output)], check=True, capture_output=True
    )
    lib = ctypes.CDLL(str(output))
    verify = getattr(lib, "flit128_avx2_verify" if optimized else "flit128_ref_verify")
    verify.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
    verify.restype = ctypes.c_int
    return verify


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="ngcc-flit-compare-") as temporary:
        directory = Path(temporary)
        try:
            optimized = load_verify(
                SOURCES / "Optimized_Implementation/FLIT128/verify.c",
                directory / "optimized.so",
                True,
            )
            reference = load_verify(
                SOURCES / "Reference_Implementation/FLIT128/verify.c",
                directory / "reference.so",
                False,
            )
        except (FileNotFoundError, subprocess.CalledProcessError) as exc:
            print(f"SKIP kem-15-2: C compiler or AVX2 build unavailable: {exc}")
            return 77

        rng = random.Random(0xF1152026)
        equal_when_different = 0
        reference_rejects = 0
        for _ in range(2000):
            a = ctypes.create_string_buffer(rng.randbytes(1024))
            b = ctypes.create_string_buffer(rng.randbytes(1024))
            equal_when_different += optimized(a, b, 1024) == 0
            reference_rejects += reference(a, b, 1024) != 0
        assert equal_when_different == 2000
        assert reference_rejects == 2000
        print(
            "CONFIRMED kem-15-2: AVX2 verify falsely accepted 2000/2000 "
            "unrelated inputs; reference rejected 2000/2000"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
