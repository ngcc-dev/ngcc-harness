#!/usr/bin/env python3
"""Source certificate for the SNOVA reference signer's address/branch trace."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REF = ROOT / "SNOVA/submission/SNOVA_Round3/Reference_Implementation"
OPT = ROOT / "SNOVA/submission/SNOVA_Round3/Additional_Implementations/AVX2/SNOVA_I_K"

REQUIRED = {
    "secret_indexed_multiply": "return gf_multtab[a * SNOVA_q + b];",
    "secret_indexed_inverse": "return gf_invtab[a];",
    "secret_indexed_add": "return gf_addtab[a * SNOVA_q + b];",
    "secret_pivot_branch": "if (gauss[i][i] == 0)",
    "secret_row_branch": "if (gauss[j][i] != 0)",
    "secret_retry": "} while (flag_redo);",
}


def trace(matrix: list[list[int]]) -> list[str]:
    """Model only the submitted pivot/row branch decisions over GF(16)."""
    a = [row[:] for row in matrix]
    events: list[str] = []
    n = len(a)
    for i in range(n):
        events.append(f"pivot[{i}]={'zero' if a[i][i] == 0 else 'nonzero'}")
        if a[i][i] == 0:
            for j in range(i + 1, n):
                events.append(f"candidate[{j},{i}]={'zero' if a[j][i] == 0 else 'nonzero'}")
                if a[j][i] != 0:
                    a[i], a[j] = a[j], a[i]
                    events.append(f"swap[{i},{j}]")
                    break
        if a[i][i] == 0:
            events.append("redo")
            break
        for j in range(i + 1, n):
            events.append(f"eliminate[{j},{i}]={'skip' if a[j][i] == 0 else 'run'}")
    return events


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    files = sorted(REF.glob("*/snova_ref.c"))
    if len(files) != 18:
        raise SystemExit(f"expected 18 reference instances, found {len(files)}")

    checked = []
    for path in files:
        raw = path.read_bytes()
        text = raw.decode("utf-8")
        missing = [name for name, token in REQUIRED.items() if token not in text]
        if missing:
            raise SystemExit(f"{path}: missing {', '.join(missing)}")
        params = (path.parent / "snova_params.h").read_text(encoding="utf-8")
        if "#define SNOVA_q 16" not in params:
            raise SystemExit(f"{path.parent}: expected recommended q=16 parameter set")
        checked.append({
            "path": str(path.relative_to(ROOT)),
            "sha256": hashlib.sha256(raw).hexdigest(),
            "markers": sorted(REQUIRED),
        })

    opt_file = OPT / "src/platforms/x86_avx2/rct_sign_gfni.h"
    opt_text = opt_file.read_text(encoding="utf-8")
    opt_markers = [
        "ct_gf_nz",
        "rct_gauss_row_scale_sec",
        "rct_gauss_row_axpy_sec",
        "SNOVA_CT_DECLASSIFY(&flag_redo",
    ]
    missing_opt = [token for token in opt_markers if token not in opt_text]
    if missing_opt:
        raise SystemExit(f"optimized control source missing markers: {missing_opt}")

    direct = [[1, 1], [0, 1]]
    swap = [[0, 1], [1, 1]]
    direct_trace, swap_trace = trace(direct), trace(swap)
    if direct_trace == swap_trace:
        raise SystemExit("branch-trace controls unexpectedly equal")

    # These are table byte offsets for q=16.  The differing addresses are the
    # cache-address property; the script does not claim to measure a cache.
    table_address_controls = {
        "gf_mult_a1_b7": 1 * 16 + 7,
        "gf_mult_a9_b7": 9 * 16 + 7,
        "gf_inv_a1": 1,
        "gf_inv_a9": 9,
    }
    result = {
        "format": "SNOVA-5-source-certificate-v1",
        "status": "PASS",
        "scope": "18 submitted On-Ramp Round 3 reference q=16 implementations",
        "reference_files": checked,
        "table_address_controls": table_address_controls,
        "branch_trace_controls": {"direct_pivot": direct_trace, "row_swap": swap_trace},
        "optimized_control": {
            "path": str(opt_file.relative_to(ROOT)),
            "markers": opt_markers,
            "meaning": "separate submitted optimized path has masked/secure helpers",
        },
        "limitations": [
            "This is a source and abstract trace certificate, not a timing or cache measurement.",
            "It does not recover a secret key or produce a forgery.",
            "It does not assess the optimized implementation as a whole.",
        ],
    }
    rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    print("PASS SNOVA-5: all reference signers contain the traced secret-dependent operations")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
