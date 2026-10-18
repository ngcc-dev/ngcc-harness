#!/usr/bin/env python3
"""Pin and verify the submitted source trace underlying MAYO-1."""

import hashlib
import json
import pathlib
import sys


ROOT = pathlib.Path(__file__).resolve().parent
FILES = {
    "mayo.c": (
        ROOT / "submission/MAYO/Reference_Implementation/src/mayo.c",
        "d57c77dff3bafad3e063e000544eedb4390197e56762cb8b10d5758cede54645",
    ),
    "arithmetic.c": (
        ROOT / "submission/MAYO/Reference_Implementation/src/arithmetic.c",
        "5823524f2d75d2f7dc29b403547748f2b27d03dc503a9a76d1d1acba716f07b7",
    ),
    "echelon_form.h": (
        ROOT / "submission/MAYO/Reference_Implementation/src/generic/echelon_form.h",
        "98fab8fb6ac03ff1bb530f3d5fb7702912710577e5d7f10d7ebbe3a8d06382b8",
    ),
    "MAYO-spec.pdf": (
        ROOT / "MAYO-spec.pdf",
        "1fc7561dc8ffdbe36c101b985266e4d3c8aa88fa580e18ead215a80ac7bb371f",
    ),
}


def ordered(text, needles):
    at = -1
    positions = []
    for needle in needles:
        at = text.find(needle, at + 1)
        if at < 0:
            raise ValueError(f"missing source step: {needle}")
        positions.append(at)
    return positions


def rank_check_retries(last_row):
    """Model arithmetic.c's OR reduction and `full_rank == 0` branch."""
    full_rank = 0
    for coefficient in last_row:
        full_rank |= coefficient
    return full_rank == 0


def main():
    hashes = {}
    failures = []
    for name, (path, expected) in FILES.items():
        actual = hashlib.sha256(path.read_bytes()).hexdigest()
        hashes[name] = actual
        if actual != expected:
            failures.append(f"{name}: expected {expected}, got {actual}")

    mayo = FILES["mayo.c"][0].read_text(encoding="utf-8")
    arithmetic = FILES["arithmetic.c"][0].read_text(encoding="utf-8")
    try:
        caller_trace = ordered(mayo, [
            "seed_sk = sk->sk_seed;",
            "memcpy(tmp + param_digest_bytes + param_salt_bytes, seed_sk,",
            "shake256(V, param_k * param_v_bytes + param_r_bytes, tmp,",
            "m_calculate_M_and_VPV(p, P1, P2_or_L, sk->O, Vdec, lambda, use_l,",
            "if (sample_solution(p, A, y, r, x, param_k, param_o, param_m, param_A_cols))",
            "break;",
        ])
        callee_trace = ordered(arithmetic, [
            "unsigned char full_rank = 0;",
            "full_rank |= A[(m - 1) * A_cols + i];",
            "VALGRIND_MAKE_MEM_DEFINED(&full_rank, 1);",
            "if (full_rank == 0)",
            "return 0;",
        ])
    except ValueError as error:
        failures.append(str(error))
        caller_trace = []
        callee_trace = []

    # Exact branch model: zero last-row coefficients retry; any nonzero one succeeds.
    model = {
        "all_zero_last_row": rank_check_retries(bytes(3)),
        "nonzero_last_row": rank_check_retries(bytes((0, 5, 0))),
    }
    if model != {"all_zero_last_row": True, "nonzero_last_row": False}:
        failures.append("unexpected branch-model result")

    result = {
        "id": "MAYO-1",
        "status": "FAIL" if failures else "PASS",
        "evidence": "pinned-source data-flow/root-cause witness",
        "hashes": hashes,
        "caller_trace_offsets": caller_trace,
        "callee_trace_offsets": callee_trace,
        "branch_model_retry": model,
        "limitations": [
            "does not measure a timing distribution",
            "does not recover a key or produce a forgery",
            "does not show that the rank bit is statistically correlated with the long-term key",
        ],
        "failures": failures,
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())

