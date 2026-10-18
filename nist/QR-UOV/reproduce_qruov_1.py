#!/usr/bin/env python3
"""Reproduce the submitted-source evidence for QR-UOV-1."""

import argparse
import hashlib
import json
from pathlib import Path


HERE = Path(__file__).resolve().parent
BASE = HERE / "submission/QR_UOV - Round3"
BACKENDS = [
    BASE / "Reference_Implementation/ref",
    BASE / "Optimized_Implementation/opt",
    BASE / "Optimized_Implementation/avx2",
]
LINSYS_SHA256 = "ec4d44a48707c0bdf9212bc5ddb22105068f28fcdbd0467cfde42ebd92b8b5ea"
COMMON_SHA256 = "0a551833d268ea855851a4c3d4f6517d7d70219b88d3873a0f51caaccd5d8117"


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    linsys = [path / "linsys.c" for path in BACKENDS]
    common = [path / "qruov_common.c" for path in BACKENDS]
    gf = [path / "gf.h" for path in BACKENDS]
    lt = [path.read_text() for path in linsys]
    ct = [path.read_text() for path in common]
    gt = [path.read_text() for path in gf]
    checks = {
        "all_three_backends_present": all(path.is_file() for path in linsys + common + gf),
        "linsys_sources_identical": all(sha(path) == LINSYS_SHA256 for path in linsys),
        "common_sources_identical": all(sha(path) == COMMON_SHA256 for path in common),
        "secret_matrix_from_vinegar": all(
            "sign_build_system(eqn, u, sdt, y, seed_pk)" in text for text in ct
        ),
        "data_dependent_pivot_search": all(
            "while (EQN(echelon, j, c) == 0)" in text
            and "row_swap(echelon->eqn, i, j)" in text for text in lt
        ),
        "data_dependent_inverse_lookup": all(
            "uint8_t inv = gf_inv(pivot)" in text for text in lt
        ) and all("return gf_inv_table[a]" in text for text in gt),
        "rank_retry_is_explicit": all(
            "if (rank == QRUOV_m || linsys_check_prepared_consistency" in text
            for text in ct
        ),
        "no_alternate_ct_solver_in_archive": (
            len(list(BASE.rglob("linsys.c"))) == 3
            and not any("mask_eq" in text or "ct_select" in text for text in lt)
        ),
    }
    result = {
        "finding": "QR-UOV-1",
        "evidence": "source-level root-cause witness; no timing measurement or key recovery",
        "linsys_sha256": LINSYS_SHA256,
        "backends": [str(path.relative_to(HERE)) for path in BACKENDS],
        "checks": checks,
        "status": "PASS" if all(checks.values()) else "FAIL",
    }
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded)
    print(encoded, end="")
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
