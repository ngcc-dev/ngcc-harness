#!/usr/bin/env python3
"""Recover MORNING-ATLAS s1 from public signatures using the submitted code.

Run with any Python that can import sage.all, for example:
    sage -python sign-15/reproduce_mask_key_recovery.py
or  mamba run -n sage python sign-15/reproduce_mask_key_recovery.py
The temporary C driver only generates a test key, four official signatures,
and their public challenge/response encodings. The first two signatures alone
are used for recovery; the secret key is read only for the final control.
"""
from __future__ import annotations

import argparse
import subprocess
import tempfile
from pathlib import Path

from sage.all import GF, matrix, vector


ROOT = Path(__file__).resolve().parent
FIELD = GF(2147483647)


def multiplication_row(challenge: list[int], degree: int) -> list[int]:
    n = len(challenge)
    return [challenge[(degree - k) % n] * (1 if degree >= k else -1)
            for k in range(n)]


def paired_equations(challenge: list[int]) -> list[list[int]]:
    n = len(challenge)
    return [[a - b for a, b in zip(multiplication_row(challenge, 2 * i),
                                   multiplication_row(challenge, 2 * i + 1))]
            for i in range(n // 2)]


def parse_lines(output: str):
    lines = [line.split() for line in output.splitlines()]
    if len(lines) != 11 or lines[0][0] != "PARAM" or lines[1][0] != "PK" or lines[-1][0] != "SECRET":
        raise ValueError("unexpected generator output")
    n, length, modulus = map(int, lines[0][1:])
    if modulus != 1 << 23 or n not in (128, 256) or length not in range(1, 16):
        raise ValueError("unexpected candidate parameters")
    public_key = lines[1][1]
    if len(public_key) % 2 or not all(ch in "0123456789abcdef" for ch in public_key):
        raise ValueError("invalid public key encoding")
    samples = []
    for i in range(4):
        c_line, z_line = lines[2 + 2 * i], lines[3 + 2 * i]
        if c_line[0] != "C" or z_line[0] != "Z":
            raise ValueError("missing challenge or response")
        c, z = list(map(int, c_line[1:])), list(map(int, z_line[1:]))
        if len(c) != n or len(z) != length * n:
            raise ValueError("wrong challenge or response length")
        samples.append((c, z))
    secret = list(map(int, lines[-1][1:]))
    if len(secret) != length * n:
        raise ValueError("wrong secret length")
    return n, length, public_key, samples, secret


def compile_and_sample(level: int, variant: str, binary: Path) -> tuple:
    source = ROOT / "Implementation" / variant / f"lwrdsa{level}"
    files = [source / f"SIG_lwrdsa{level}.c"] + [source / f"{name}.c" for name in
        ("poly", "polyvec", "packing", "rounding", "auxfunc", "drng")]
    if not all(path.is_file() for path in files):
        raise FileNotFoundError(f"missing archived sources in {source}")
    flags = ["-mavx2", "-DATLAS_OPTIMIZED"] if variant == "Optimized_Implementation" else []
    subprocess.run(["cc", "-O2", "-std=gnu11", "-fwrapv", "-fno-strict-aliasing",
                    *flags,
                    "-Wno-unused", "-I", str(source), str(ROOT / "reproduce_mask_key_recovery.c"),
                    *map(str, files), "-lm", "-o", str(binary)], check=True,
                   stdout=subprocess.DEVNULL)
    output = subprocess.run([str(binary)], check=True, capture_output=True,
                            text=True, timeout=120).stdout
    return parse_lines(output)


def recover(level: int, variant: str) -> None:
    with tempfile.TemporaryDirectory(prefix="atlas-mask-") as temporary:
        binary = Path(temporary) / "sample"
        n, length, public_key, samples, secret = compile_and_sample(level, variant, binary)
        rows = []
        responses = []
        for challenge, z in samples[:2]:
            rows.extend(paired_equations(challenge))
            responses.extend([[z[j * n + 2 * i] - z[j * n + 2 * i + 1]
                               for j in range(length)] for i in range(n // 2)])
        coefficients = matrix(FIELD, rows)
        if coefficients.rank() != n:
            raise AssertionError(f"{variant} {level}: two-signature system not full rank")
        solution = coefficients.solve_right(matrix(FIELD, responses))
        recovered = [[int(value) if int(value) <= FIELD.order() // 2
                      else int(value) - FIELD.order() for value in solution.column(j)]
                     for j in range(length)]
        expected = [secret[j * n:(j + 1) * n] for j in range(length)]
        if recovered != expected:
            raise AssertionError(f"{variant} {level}: recovered key differs from s1")
        for challenge, z in samples[2:]:
            equations = paired_equations(challenge)
            for j in range(length):
                for i, row in enumerate(equations):
                    observed = z[j * n + 2 * i] - z[j * n + 2 * i + 1]
                    if sum(a * b for a, b in zip(row, recovered[j])) != observed:
                        raise AssertionError(f"{variant} {level}: fresh-signature residual")
        recovered_flat = [value for polynomial in recovered for value in polynomial]
        forge_input = public_key + "\n" + " ".join(map(str, recovered_flat)) + "\n"
        forgery = subprocess.run([str(binary), "--forge"], input=forge_input,
                                 capture_output=True, text=True, check=True, timeout=120)
        if not forgery.stdout.startswith("FORGED:"):
            raise AssertionError(f"{variant} {level}: no full-scheme forgery")
    print(f"CONFIRMED sign-15-4 {variant} ATLAS-{level}: "
          f"two verified signatures recover all {length * n} s1 coefficients; "
          "two further signatures have zero residual; new-message forgery verifies")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--level", type=int, choices=(128, 192, 256), action="append")
    parser.add_argument("--variant", choices=("Reference_Implementation", "Optimized_Implementation"),
                        action="append")
    args = parser.parse_args()
    for variant in args.variant or ("Reference_Implementation", "Optimized_Implementation"):
        for level in args.level or (128, 192, 256):
            recover(level, variant)


if __name__ == "__main__":
    main()
