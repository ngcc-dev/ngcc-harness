#!/usr/bin/env python3
"""Recompute the rank-failure model and query condition cited by QR-UOV-2."""

import argparse
from decimal import Decimal, getcontext
import hashlib
import json
from pathlib import Path


HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
SPEC_SHA256 = "23100e20c4b442c6e30ee449def82158c7b6ac5e4eb5c77869587075ddba9958"


def rank_failure(q: int, m: int, delta: int) -> Decimal:
    qd = Decimal(q)
    total = Decimal(0)
    for deficiency in range(delta + 1, m + 1):
        numerator = Decimal(1)
        for j in range(deficiency + 1, m + 1):
            numerator *= (1 - qd ** (-j)) ** 2
        denominator = Decimal(1)
        for j in range(1, m - deficiency + 1):
            denominator *= 1 - qd ** (-j)
        total += qd ** (-(deficiency * deficiency)) * numerator / denominator
    return total


def log2_qrom_term(qsign: int, qsign_prime: int, qqro: int, security: int) -> Decimal:
    """Log2 of Theorem 2's first additive QROM term."""
    numerator = Decimal(qsign_prime + 3 * qsign + qqro + 5)
    log2 = Decimal(2).ln()
    return (
        (Decimal(3) / Decimal(2)).ln() / log2
        + Decimal(qsign_prime).ln() / log2
        + (numerator.ln() / log2 - Decimal(security)) / 2
    )


def qrom_threshold(security: int) -> Decimal:
    """Largest log2(qsign) with favorable qsign'=qsign, qqro=0 term below one."""
    low, high = Decimal(0), Decimal(security)
    two = Decimal(2)
    for _ in range(200):
        middle = (low + high) / 2
        qsign = int(two ** middle)
        if log2_qrom_term(qsign, qsign, 0, security) < 0:
            low = middle
        else:
            high = middle
    return low


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    getcontext().prec = 100
    estimates = {}
    for m in (54, 60, 78, 90, 105, 110):
        epsilon = rank_failure(127, m, 2)
        estimates[str(m)] = {
            "epsilon_rank": str(epsilon),
            "log2_epsilon": float(epsilon.ln() / Decimal(2).ln()),
            "product_at_2^61": str(epsilon * Decimal(2) ** 61),
            "product_at_2^64": str(epsilon * Decimal(2) ** 64),
        }
    values = [Decimal(item["epsilon_rank"]) for item in estimates.values()]
    qrom = {}
    for security in (128, 192, 256):
        qrom[str(security)] = {
            "log2_term_at_2^64": float(
                log2_qrom_term(2 ** 64, 2 ** 64, 0, security)
            ),
            "log2_query_threshold": float(qrom_threshold(security)),
            "assumptions": "qsign_prime=qsign and qqro=0 (favorable lower choice)",
        }
    rules = (ROOT / "doc/NIST-rules.md").read_text()
    checks = {
        "pinned_specification": hashlib.sha256((HERE / "QR-UOV-spec.pdf").read_bytes()).hexdigest() == SPEC_SHA256,
        "nist_2^64_baseline_recorded": "2^64 chosen-message signing queries" in " ".join(rules.split()),
        "matches_specification_estimate": all(
            Decimal("1.172e-19") < value < Decimal("1.174e-19") for value in values
        ),
        "theorem_condition_holds_at_2^61": all(value * Decimal(2) ** 61 < Decimal("0.5") for value in values),
        "theorem_condition_fails_at_2^64": all(value * Decimal(2) ** 64 >= Decimal("0.5") for value in values),
        "qrom_term_vacuous_at_2^64_for_lambda_128_192": all(
            Decimal(str(qrom[str(security)]["log2_term_at_2^64"])) > 0
            for security in (128, 192)
        ),
        "qrom_term_nonvacuous_at_2^64_for_lambda_256": (
            Decimal(str(qrom["256"]["log2_term_at_2^64"])) < 0
        ),
        "qrom_query_thresholds_match": (
            abs(Decimal(str(qrom["128"]["log2_query_threshold"])) - Decimal("41.6100")) < Decimal("0.0001")
            and abs(Decimal(str(qrom["192"]["log2_query_threshold"])) - Decimal("62.9433")) < Decimal("0.0001")
        ),
    }
    result = {
        "finding": "QR-UOV-2",
        "evidence": "cost/model check of the rank heuristic and additive QROM term; not an attack",
        "model": "sum of exact rank-deficiency probabilities for a uniform m by m matrix over F_127",
        "estimates": estimates,
        "first_additive_qrom_term": qrom,
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
