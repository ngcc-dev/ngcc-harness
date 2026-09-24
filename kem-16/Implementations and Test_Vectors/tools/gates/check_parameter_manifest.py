#!/usr/bin/env python3
"""Validate HARE instance headers, API sizes, formulas, and primitive-prime n values."""
import csv
import math
import re
import sys
from pathlib import Path
from typing import Dict, List

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "PARAMETER_MANIFEST.tsv"

MACRO_MAP = {
    "n": "PARAM_N",
    "k_bytes": "PARAM_K",
    "w": "PARAM_OMEGA",
    "wr": "PARAM_OMEGA_R",
    "we": "PARAM_OMEGA_E",
    "n1": "PARAM_N1",
    "n2": "PARAM_N2",
    "alpha": "PARAM_ALPHA",
    "comp_n": "PARAM_COMP_N",
    "comp_k": "PARAM_COMP_K",
    "comp_r": "PARAM_COMP_R",
    "seed_bytes": "SEED_BYTES",
    "salt_bytes": "SALT_BYTES",
    "ss_bytes": "SHARED_SECRET_BYTES",
    "pk_bytes": "PUBLIC_KEY_BYTES",
    "sk_api_expanded_bytes": "SECRET_KEY_BYTES",
    "ct_bytes": "CIPHERTEXT_BYTES",
}


def read_numeric_macros(path: Path) -> Dict[str, int]:
    text = path.read_text()
    out = {}
    for m in re.finditer(r"^#define\s+([A-Z0-9_]+)\s+([0-9]+)\s*$", text, re.M):
        out[m.group(1)] = int(m.group(2))
    return out


def factors(n: int) -> List[int]:
    result = []
    d = 2
    while d * d <= n:
        if n % d == 0:
            result.append(d)
            while n % d == 0:
                n //= d
        d += 1 if d == 2 else 2
    if n > 1:
        result.append(n)
    return result


def is_prime(n: int) -> bool:
    if n < 2:
        return False
    if n % 2 == 0:
        return n == 2
    d = 3
    while d * d <= n:
        if n % d == 0:
            return False
        d += 2
    return True


def is_two_primitive_root(n: int) -> bool:
    if not is_prime(n):
        return False
    phi = n - 1
    return all(pow(2, phi // q, n) != 1 for q in factors(phi))


def ceil_div(a: int, b: int) -> int:
    return (a + b - 1) // b


def expected_paths(instance: str, backend: str) -> List[Path]:
    level = instance.split("-", 2)[:2]
    hare = "-".join(level)
    paths = [
        ROOT / "Implementations/Reference_Implementation" / hare / backend,
        ROOT / "Implementations/Optimized_Implementation" / hare / backend,
    ]
    if backend == "kr":
        paths.append(ROOT / "Implementations/Additional_Implementation" / hare / "kr-arm-sve")
    return paths


def main() -> int:
    failures = []
    with MANIFEST.open(newline="") as f:
        rows = list(csv.DictReader(f, delimiter="\t"))

    for row in rows:
        instance = row["instance"]
        backend = row["backend"]
        values = {k: int(row[k]) for k in MACRO_MAP}
        n = values["n"]
        n1n2 = values["n1"] * values["n2"]
        comp_q = n1n2 // values["comp_n"]
        l2 = comp_q * values["comp_n"]
        nm = comp_q * values["comp_k"]
        nv2 = n1n2 - l2
        payload_bytes = ceil_div(nm + nv2, 8)
        derived_pk = values["seed_bytes"] + ceil_div(n, 8)
        derived_sk = derived_pk + values["seed_bytes"] + values["ss_bytes"] + values["seed_bytes"]
        derived_ct = ceil_div(n, 8) + payload_bytes + values["salt_bytes"]

        if n1n2 > n:
            failures.append(f"{instance}: n1*n2={n1n2} exceeds n={n}")
        if derived_pk != values["pk_bytes"]:
            failures.append(f"{instance}: pk manifest={values['pk_bytes']} derived={derived_pk}")
        if derived_sk != values["sk_api_expanded_bytes"]:
            failures.append(f"{instance}: sk manifest={values['sk_api_expanded_bytes']} derived={derived_sk}")
        if derived_ct != values["ct_bytes"]:
            failures.append(f"{instance}: ct manifest={values['ct_bytes']} derived={derived_ct}")
        if int(row["sk_seed_reconstruction_bytes"]) != values["seed_bytes"]:
            failures.append(f"{instance}: seed-reconstruction size must equal seed size")
        if not is_two_primitive_root(n):
            failures.append(f"{instance}: n={n} is not a primitive prime for base 2")

        for directory in expected_paths(instance, backend):
            if not directory.is_dir():
                failures.append(f"{instance}: missing implementation directory {directory.relative_to(ROOT)}")
                continue
            params = read_numeric_macros(directory / "parameters.h")
            api = read_numeric_macros(directory / "api.h")
            for field, macro in MACRO_MAP.items():
                expected = values[field]
                actual = params.get(macro)
                if actual != expected:
                    failures.append(f"{directory.relative_to(ROOT)}: {macro}={actual}, expected {expected}")
            api_checks = {
                "CRYPTO_PUBLICKEYBYTES": values["pk_bytes"],
                "CRYPTO_SECRETKEYBYTES": values["sk_api_expanded_bytes"],
                "CRYPTO_CIPHERTEXTBYTES": values["ct_bytes"],
                "CRYPTO_BYTES": values["ss_bytes"],
            }
            for macro, expected in api_checks.items():
                actual = api.get(macro)
                if actual != expected:
                    failures.append(f"{directory.relative_to(ROOT)}: {macro}={actual}, expected {expected}")

    if failures:
        print("parameter manifest check: FAIL")
        for item in failures:
            print(" -", item)
        return 1
    print(f"parameter manifest check: PASS ({len(rows)} algorithm instances)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
