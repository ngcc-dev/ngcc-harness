#!/usr/bin/env python3
"""HAWK-1: deterministic algebra/model controls and cached source verification.

This does not solve SVP or recover a HAWK key. The toy identity control starts
with a known triangular basis; it only checks the stated public constraints.
"""
import argparse
from decimal import Decimal, ROUND_HALF_UP, ROUND_CEILING
import hashlib
import json
from pathlib import Path
import shutil
import sys

ROOT = Path(__file__).resolve().parents[1]
COMMIT = "fa01c398b42bb7d94eadff75a42dfb45da484457"
PAPER_SHA256 = "056faead316ea9a8eb01000cb9c548e79e21ad09a0684c3d52bbc1093183ac89"
ARCHIVE_SHA256 = "3f318c7124e1fe93e6450e77d3702c3c4555b3cd24470dbc93006ed6b7364fed"


def toy_identity(n):
    """Check cocycle identities in Z[x]/(x^n+1), using a known basis B."""
    zero = (0,) * n
    one = (1,) + (0,) * (n - 1)
    def add(a, b): return tuple(x + y for x, y in zip(a, b))
    def neg(a): return tuple(-x for x in a)
    def mul(a, b):
        result = [0] * n
        for i, x in enumerate(a):
            for j, y in enumerate(b):
                result[(i + j) % n] += x * y * (-1 if i + j >= n else 1)
        return tuple(result)
    def tau(a): return tuple(x * (-1 if i % 2 else 1) for i, x in enumerate(a))
    def conjugate(a): return (a[0],) + tuple(-a[n - i] for i in range(1, n))
    def mm(a, b):
        return tuple(tuple(add(mul(a[i][0], b[0][j]), mul(a[i][1], b[1][j]))
                           for j in range(2)) for i in range(2))
    def mapped(a, function): return tuple(tuple(function(x) for x in row) for row in a)
    def transpose(a): return tuple(tuple(a[j][i] for j in range(2)) for i in range(2))
    identity = ((one, zero), (zero, one))
    a = tuple(i % 3 - 1 for i in range(n))
    basis = ((one, a), (zero, one))
    inverse = ((one, neg(a)), (zero, one))
    public = mm(transpose(mapped(basis, conjugate)), basis)
    cocycle = mm(inverse, mapped(basis, tau))
    adjugate = ((cocycle[1][1], neg(cocycle[0][1])),
                (neg(cocycle[1][0]), cocycle[0][0]))
    sigma_transpose = transpose(mapped(cocycle, lambda x: conjugate(tau(x))))
    checks = [mm(basis, inverse) == identity, cocycle != identity,
              mm(cocycle, mapped(cocycle, tau)) == identity,
              mapped(cocycle, tau) == adjugate,
              mm(public, cocycle) == mm(sigma_transpose, mapped(public, tau))]
    if not all(checks):
        raise ValueError(f"toy cocycle constraint failure at n={n}")
    return {"n": n, "status": "PASS", "known_secret_control": True,
            "recovered_unknown_key": False}


def models():
    output = []
    for n, old_beta in [(256, 211), (512, 452), (1024, 940)]:
        dimension = n // 2 + 1
        core = lambda beta: int((Decimal("0.292") * beta).to_integral_value(rounding=ROUND_HALF_UP))
        row = {"n": n, "key_lattice_rank": 2 * n, "cocycle_rank": n,
               "published_original_beta": old_beta, "exact_svp_oracle_dimension": dimension,
               "core_svp_original_log2": core(old_beta), "core_svp_attack_log2": core(dimension)}
        if n in {512, 1024}:
            per_call, calls, original = ("86.0", "21.9", 150) if n == 512 else ("157.8", "24.0", 288)
            total = (Decimal(per_call) + Decimal(calls)).to_integral_value(rounding=ROUND_CEILING)
            row.update(published_agps20_per_call_log2=per_call,
                       published_oracle_call_bound_log2=calls,
                       reproduced_agps20_total_ceiling_log2=int(total),
                       published_original_agps20_total_log2=original)
            if int(total) != (108 if n == 512 else 182):
                raise ValueError("gate-cost arithmetic mismatch")
        output.append(row)
    if [row["exact_svp_oracle_dimension"] for row in output] != [129, 257, 513]:
        raise ValueError("dimension arithmetic mismatch")
    return output


def evidence():
    records = []
    for path in (
        ROOT / "security/harvests.jsonl",
        ROOT / ".cache/security/harvests.jsonl",
    ):
        if path.exists():
            records.extend(json.loads(line) for line in path.read_text().splitlines() if line)
    requirements = {
        "nist-round3": (None, ["HAWK", "withdrawn"]),
        "hawk-pqc-forum": (None, ["withdraw our candidate HAWK", "Dustin Moody"]),
        "hawk-key-recovery-paper": (PAPER_SHA256, ["Theorem 6.1", "HAWK-512"]),
        "hawk-demo-archive": (ARCHIVE_SHA256, [])
    }
    checks = []
    for source_id, (pinned, tokens) in requirements.items():
        candidates = [r for r in records if r["source_id"] == source_id and r["status"] == "ok"]
        if not candidates:
            checks.append({"source": source_id, "status": "SKIP", "reason": "no successful logged harvest"})
            continue
        record = candidates[-1]
        raw = ROOT / record["cache_path"]
        text = ROOT / record.get("text_cache_path", record["cache_path"])
        if not raw.is_file() or not text.is_file():
            checks.append({"source": source_id, "status": "SKIP", "reason": "local evidence cache absent; run harvest.py"})
            continue
        digest = hashlib.sha256(raw.read_bytes()).hexdigest()
        valid = digest == record["sha256"] and (not pinned or digest == pinned)
        content = text.read_text(errors="replace") if tokens else ""
        valid = valid and all(token.casefold() in content.casefold() for token in tokens)
        if tokens:
            valid = valid and hashlib.sha256(text.read_bytes()).hexdigest() == record["text_sha256"]
        checks.append({"source": source_id, "status": "PASS" if valid else "FAIL",
                       "sha256": digest, "retrieved_at": record["retrieved_at"]})
    return checks


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--require-cache", action="store_true", help="return SKIP (77) if local source artifacts are absent")
    args = parser.parse_args()
    try:
        result = {"finding": "HAWK-1", "verification": "algebra-and-cost-model-control",
                  "upstream_commit": COMMIT, "toy_controls": [toy_identity(4), toy_identity(8)],
                  "model_checks": models(), "source_checks": evidence(),
                  "end_to_end_key_recovery": {"status": "SKIP", "executed": False,
                    "reason": "upstream SVP key-recovery pipeline has not been run locally",
                    "sage_available": bool(shutil.which("sage")),
                    "nix_shell_available": bool(shutil.which("nix-shell"))}}
    except (OSError, ValueError, KeyError) as error:
        print(json.dumps({"finding": "HAWK-1", "status": "FAIL", "error": str(error)}))
        return 1
    failed = any(check["status"] == "FAIL" for check in result["source_checks"])
    skipped = any(check["status"] == "SKIP" for check in result["source_checks"])
    result["status"] = "FAIL" if failed else "SKIP"
    print(json.dumps(result, indent=2))
    return 1 if failed else 77


if __name__ == "__main__":
    sys.exit(main())
