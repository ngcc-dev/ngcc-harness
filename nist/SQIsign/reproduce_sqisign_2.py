#!/usr/bin/env python3
"""Source certificate for SQIsign secret-dependent signing control flow."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "SQIsign/submission/SQIsign/Reference_Implementation"
SIGN = BASE / "src/signature/ref/lvlx/sign.c"
MP = BASE / "src/mp/ref/lvlx/mp.c"
SUPPRESSIONS = BASE / "test/ct-quaternion.supp"
README = BASE / "README.md"


REQUIRED = {
    SIGN: [
        "ibz_invmod(&degree_resp_inv, &ideal_skchall.norm, &remain);",
        "ibz_invmod(&degree_resp_inv, &deg_odd, &remain);",
        "quat_response_element(&ideal_skchall,",
        "if (!ibz_is_one(&tmp_int))",
        "if (!ibz_is_one(&tmp_int) || !ibz_is_one(&tmp_int2))",
    ],
    MP: [
        "Variable-time (non-constant-time) modular inverse",
        "if (mp_compare(abs_a.limbs, abs_mod.limbs",
        "while (ulen > 1 && u[ulen - 1] == 0)",
        "while (1)",
        "steps = mp_lehmer_inner",
        "mp_div_unsigned(qbuf, rbuf, u, ulen, v, vlen);",
    ],
    SUPPRESSIONS: [
        "KNOWN to be",
        "non-constant-time in its current state",
        "fun:sqisign_gen_ibz_*",
        "fun:*id2iso_*",
    ],
    README: [
        "Accepted exemptions (the remaining",
        "non-constant-time parts of mp, id2iso and the quaternion layer)",
    ],
}


def euclid_trace(a: int, modulus: int) -> list[dict[str, int]]:
    """Small control showing that value-driven inverse traces differ."""
    u, v = modulus, a
    trace = []
    while v:
        trace.append({"u": u, "v": v, "quotient": u // v})
        u, v = v, u % v
    return trace


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    sources = []
    for path, tokens in REQUIRED.items():
        data = path.read_bytes()
        text = data.decode("utf-8")
        missing = [token for token in tokens if token not in text]
        if missing:
            raise SystemExit(f"{path}: missing source markers {missing}")
        sources.append({
            "path": str(path.relative_to(ROOT)),
            "sha256": hashlib.sha256(data).hexdigest(),
            "markers": tokens,
        })

    libraries = json.loads((ROOT / "SQIsign/libraries.json").read_text())
    parameters = sorted(entry["parameter"] for entry in libraries)
    if parameters != ["p324_3", "p500_27", "p664_17"]:
        raise SystemExit(f"unexpected SQIsign parameter inventory: {parameters}")

    traces = {str(a): euclid_trace(a, 256) for a in (3, 7)}
    if len(traces["3"]) == len(traces["7"]):
        raise SystemExit("inverse trace controls unexpectedly have equal lengths")

    document = {
        "format": "SQIsign-2-source-certificate-v1",
        "status": "PASS",
        "affected_parameters": parameters,
        "source_trace": (
            "secret response norm ideal_skchall.norm -> "
            "ibz_invmod -> value-dependent Lehmer/division control flow"
        ),
        "public_context": (
            "The second inverse operand deg_odd is q_rsp, which is disclosed by "
            "the response representation and is not evidence for this finding."
        ),
        "sources": sources,
        "abstract_inverse_trace_controls": traces,
        "limitations": [
            "The integer controls illustrate the source property; they are not timing measurements.",
            "No secret recovery, signature forgery, or remotely observable channel is demonstrated.",
            "The certificate does not assess the promised constant-time submodules independently.",
        ],
    }
    rendered = json.dumps(document, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    print("PASS SQIsign-2: a secret response norm reaches variable-time multiprecision inversion")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
