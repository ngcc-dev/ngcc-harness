#!/usr/bin/env python3
"""Generate/check per-instance README files from parameters.h and api.h."""
import argparse
import difflib
import re
from pathlib import Path
from typing import Dict, List, Tuple

ROOT = Path(__file__).resolve().parents[2]


def macros(path: Path) -> Dict[str, str]:
    text = path.read_text(encoding="utf-8")
    return {
        m.group(1): m.group(2)
        for m in re.finditer(r"^#define\s+([A-Z0-9_]+)\s+([^\s/]+)", text, re.MULTILINE)
    }


def unquote(value: str) -> str:
    return value[1:-1] if len(value) >= 2 and value[0] == value[-1] == '"' else value


def role_for(path: Path) -> Tuple[str, str]:
    parts = path.parts
    if "Reference_Implementation" in parts:
        return "Reference implementation", "Portable ISO C99 implementation using the shared Reference core and API_PKC helper interface."
    if "Optimized_Implementation" in parts:
        return "x86 optimized implementation", "x86 AVX2/PCLMUL performance implementation using the shared optimized kernels and the same API/KAT format as Reference."
    if "Additional_Implementation" in parts:
        return "ARM/SVE Additional implementation", "AArch64 ARMv8.2-A/SVE implementation using the shared ARM/SVE kernels; the GF2X base multiply uses NEON PMULL when enabled."
    raise ValueError(path)


def status_for(backend: str) -> str:
    if backend != "kr":
        raise ValueError(f"unsupported backend in KR-only package: {backend}")
    return "Active HARE v1.5 KR `[51,41]_2` covering-code backend."


def generate_leaf(path: Path) -> str:
    p = macros(path / "parameters.h")
    a = macros(path / "api.h")
    role, role_text = role_for(path)
    backend = "kr" if path.name in ("kr", "kr-arm-sve") else path.name
    security = path.parent.name[len("HARE-"):] if path.parent.name.startswith("HARE-") else path.parent.name
    title_backend = "KR"
    algname = unquote(a["CRYPTO_ALGNAME"])
    comp_r = "2"
    return f"""# HARE-{security} / {title_backend} / {role}

{status_for(backend)}

## Role

{role_text}

API algorithm name: `{algname}`.

## Public API sizes

| Field | Bytes |
|---|---:|
| Public key | {a['CRYPTO_PUBLICKEYBYTES']} |
| Expanded secret key | {a['CRYPTO_SECRETKEYBYTES']} |
| Ciphertext | {a['CRYPTO_CIPHERTEXTBYTES']} |
| Shared secret | {a['CRYPTO_BYTES']} |

The expanded secret key layout is `ekKEM || dkPKE || sigma || seedKEM`.  The
seed-only reconstruction size is `{p['SEED_BYTES']}` bytes, but this implementation
uses the expanded API layout above.

## Main parameters

| Parameter | Value |
|---|---:|
| `PARAM_N` | {p['PARAM_N']} |
| `PARAM_K` (bytes) | {p['PARAM_K']} |
| `PARAM_OMEGA = PARAM_OMEGA_R` | {p['PARAM_OMEGA']} |
| `PARAM_OMEGA_E` | {p['PARAM_OMEGA_E']} |
| `PARAM_N1` | {p['PARAM_N1']} |
| `PARAM_N2` | {p['PARAM_N2']} |
| `PARAM_L1 = PARAM_N1 * PARAM_N2` | {p['PARAM_N1N2']} |
| `PARAM_ALPHA` | {p['PARAM_ALPHA']} |
| `PARAM_COMP_N` | {p['PARAM_COMP_N']} |
| `PARAM_COMP_K` | {p['PARAM_COMP_K']} |
| covering radius | {comp_r} |

## Files

```text
api.h                         public KEM byte sizes and instance name
parameters.h                  HARE instance parameters and derived sizes
reed_solomon.h                parameter-derived Reed-Solomon declarations
KEM_AlgorithmInstance.c/.h    API_PKC-facing wrapper functions
```
"""


def targets() -> List[Path]:
    out = []
    for base in (
        ROOT / "Implementations/Reference_Implementation",
        ROOT / "Implementations/Optimized_Implementation",
        ROOT / "Implementations/Additional_Implementation",
    ):
        for p in base.glob("HARE-*/*"):
            if (p / "parameters.h").is_file() and (p / "api.h").is_file():
                out.append(p)
    return sorted(out)


def main() -> int:
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--write", action="store_true")
    group.add_argument("--check", action="store_true")
    args = parser.parse_args()

    failures = 0
    for path in targets():
        dest = path / "README.md"
        generated = generate_leaf(path)
        current = dest.read_text(encoding="utf-8") if dest.exists() else ""
        if args.check:
            if current != generated:
                failures += 1
                print(f"instance README differs: {dest.relative_to(ROOT)}")
                for line in difflib.unified_diff(
                    current.splitlines(), generated.splitlines(),
                    fromfile="current", tofile="generated", n=2,
                ):
                    print(line)
        else:
            dest.write_text(generated, encoding="utf-8")
            print(dest.relative_to(ROOT))
    if failures:
        return 1
    if args.check:
        print("instance README check: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
