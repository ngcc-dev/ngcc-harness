#!/usr/bin/env python3
"""Pin and verify UOV's secret-derived rank/retry trace."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EXPECTED_OV = "863e59ecf3838fa8643c3a65e0bfa8cb86eeefaec7a31f1260767b2a7444affc"
EXPECTED_SPEC = "ce9044adf1b4e407467968438fa66794882bcaa6411d3509bed72ff54380181a"
TRACE = [
    "VALGRIND_MAKE_MEM_UNDEFINED(sk, OV_SECRETKEYBYTES )",
    "hash_update(&h_vinegar_copy, sk->sk_seed, LEN_SKSEED )",
    "while ( MAX_ATTEMPT_VINEGAR > n_attempt )",
    "n_attempt++;",
    "hash_final_digest( vinegar, _V_BYTE, &h_vinegar)",
    "ov_calc_l1_mat( mat_l1, sk, vinegar )",
    "unsigned l1_succ = gf",
    "VALGRIND_MAKE_MEM_DEFINED(&l1_succ, sizeof(unsigned) )",
    "if ( !l1_succ )",
    "continue;",
    "break;",
    "if ( MAX_ATTEMPT_VINEGAR <= n_attempt )",
]


def ordered(text: str, needles: list[str]) -> list[int]:
    at = -1
    positions = []
    for needle in needles:
        at = text.find(needle, at + 1)
        if at < 0:
            raise ValueError(f"missing ordered source step: {needle}")
        positions.append(at)
    return positions


def retry_trace(results: list[int]) -> list[str]:
    trace = []
    for attempt, success in enumerate(results):
        trace.append(f"attempt[{attempt}]={'success' if success else 'reject'}")
        if success:
            break
    return trace


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    failures = []
    instances = []
    for entry in json.loads((ROOT / "UOV/libraries.json").read_text()):
        path = ROOT / entry["source"] / "ov.c"
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        try:
            positions = ordered(path.read_text(encoding="utf-8"), TRACE)
        except ValueError as error:
            failures.append(f"{entry['parameter']}: {error}")
            positions = []
        if digest != EXPECTED_OV:
            failures.append(f"{entry['parameter']}: unexpected ov.c hash {digest}")
        instances.append({
            "parameter": entry["parameter"],
            "source": str(path.relative_to(ROOT)),
            "sha256": digest,
            "trace_offsets": positions,
        })

    spec = ROOT / "UOV/UOV-spec.pdf"
    spec_digest = hashlib.sha256(spec.read_bytes()).hexdigest()
    if spec_digest != EXPECTED_SPEC:
        failures.append(f"unexpected specification hash {spec_digest}")

    first = retry_trace([1])
    later = retry_trace([0, 0, 1])
    if first != ["attempt[0]=success"]:
        failures.append("unexpected illustrative first-attempt trace")
    if later != ["attempt[0]=reject", "attempt[1]=reject", "attempt[2]=success"]:
        failures.append("unexpected illustrative third-attempt trace")

    document = {
        "format": "UOV-1-source-certificate-v1",
        "status": "FAIL" if failures else "PASS",
        "instances": instances,
        "specification_sha256": spec_digest,
        "illustrative_branch_traces": {"first_attempt": first, "third_attempt": later},
        "source_trace": (
            "secret key and secret-derived vinegar -> hidden linear system -> "
            "rank result -> declassified continue/break and attempt count"
        ),
        "limitations": [
            "This proves a source-level execution-trace dependency, not a measured timing channel.",
            "It does not show correlation with the long-term key, key recovery, or forgery.",
            "It does not contradict the specification's explicit decision to reveal rejected matrices.",
        ],
        "failures": failures,
    }
    rendered = json.dumps(document, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    print(f"{document['status']} UOV-1: checked {len(instances)} secret-derived retry traces")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
