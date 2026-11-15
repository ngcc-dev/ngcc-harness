#!/usr/bin/env python3
"""Reproduce MQOM-5: the concrete bound of MQOM v3 Theorem 5.1 reaches one
5.1 to 7.5 bits below 2^lambda oracle queries.

Theorem 5.1 (specification Version 3.0, physical PDF page 74, printed page 70)
bounds the EUF-KO advantage of an adversary that makes Q_RO random-oracle
queries and Q_IC ideal-cipher queries, with Q_Sign <= 2^64, Q_RO <= 2^lambda,
Q_IC <= 2^(lambda-1) and (2/N)^tau <= 2^-72/lambda, by

    eps_mq + Q_RO/|K|^eta + 2^-w * (2/N)^tau * 4*Q_IC/3
           + 3*Q_RO^2/2^(2*lambda) + (4N+1)*Q_RO*Q_IC/2^(2*lambda)
           + 2^-6 * Q_IC/2^lambda + 2^-68 * Q_RO/2^lambda + 2^-lambda.

The term (4N+1)*Q_RO*Q_IC/2^(2*lambda) is 4N*Q_RO*Q_IC/2^(2*lambda) from
Eq. (15)-(16) (physical p.76: collisions of sampled cipher outputs with the
N*Q_7 listed seed commitments) plus 2^-4*Q_IC*Q_RO/2^(2*lambda) from Eq. (18),
as combined in Eq. (19) (physical p.77).

This script evaluates the bound without eps_mq, in exact rational arithmetic,
for all 18 parameter sets, with lambda, tau, N, w, eta and |K| read from the
submitted parameter headers, and finds the number of queries Q_RO = Q_IC at
which it reaches one.  The control drops the (4N+1) term.

The queries are public random-oracle and ideal-cipher queries of a key-only
adversary, not signing queries.  The result is a boundary of the proof: no
attack or forgery follows from it.  Python standard library only; if
pdftotext is installed the transcribed terms are also looked up in the
specification text.
"""

import argparse
import hashlib
import json
import math
import re
import shutil
import subprocess
from fractions import Fraction
from pathlib import Path


HERE = Path(__file__).resolve().parent
PACKAGE = HERE / "submission/submission_package_v3"
SPEC = HERE / "MQOM-spec.pdf"
SPEC_SHA256 = "489942de99a8001b77f8abc46bf93524aeb4719c46f23f8f6cb139d9eaeaac45"

CANDIDATE = "MQOM-5"
EVIDENCE = (
    "Exact-rational evaluation of the bound printed in Theorem 5.1 (specification "
    "physical p.74; the term comes from Eq. 15-16 on p.76 and Eq. 19 on p.77), with "
    "lambda, tau, N, w and eta read from the 18 submitted parameter headers, shows "
    "that at Q_RO = Q_IC the bound reaches one at 2^(lambda-7.5) to 2^(lambda-5.1) "
    "queries, almost entirely through (4N+1)*Q_RO*Q_IC/2^(2*lambda); these are public "
    "random-oracle and ideal-cipher queries of a key-only adversary, not signing "
    "queries, and this is a boundary of the proof, not an attack or a forgery."
)
TERM = "(4N+1)*Q_RO*Q_IC/2^(2*lambda)"
BOUND = (
    "Adv_EUF-KO <= eps_mq + Q_RO/|K|^eta + 2^-w*(2/N)^tau*4*Q_IC/3 + 3*Q_RO^2/2^(2*lambda) "
    "+ (4N+1)*Q_RO*Q_IC/2^(2*lambda) + 2^-6*Q_IC/2^lambda + 2^-68*Q_RO/2^lambda + 2^-lambda"
)

# mqom3_parameters_<set>.h in Reference_Implementation/mqom3_<set>/.
HEADER_SHA256 = {
    "cat1_gf16_fast_ct": "dc7418364b44c290ae4d2d35df808616c5c8bbd7a403f0a682a1c606e7b6bdeb",
    "cat1_gf16_fast_ot": "c7ee50b17401a0bd302e83a5b80e5f717a53ddae7d38c71e585ecd548328bc8a",
    "cat1_gf16_short_ct": "9311cbc1986b2f087d037c3c77cfcd41d17be8776377cd0e93a7ebe783f9af8a",
    "cat1_gf16_short_ot": "4d74325fb1052da91dbc73017dc9dd391d4b14d2d2d29bee92a1874148ba84db",
    "cat1_gf2_shorter_ct": "3a39c856b16fbb6c64c311ad0eeb5999a0247e1fdcf0e4baeeeb7dee39806281",
    "cat1_gf2_shorter_ot": "f9dd49402020203bb5fb54aa44a64410fdba56cb7f8540db81e5d8eaed7d8da1",
    "cat3_gf16_fast_ct": "6fa3003147c155f43b35ba1b22bf6cb01ce33840560dc92a97a33706f4c81f9b",
    "cat3_gf16_fast_ot": "458101730a518e8b1896a5d9bbbe8abc150f70b83bbd290df44696ca9f01a9c6",
    "cat3_gf16_short_ct": "d54e91168ae5bcb903fa03948271701f4cfc0dff479d6bee4d5201d882a9f404",
    "cat3_gf16_short_ot": "a0c658e52254c054dc1267a01f5f4cbdaab56316dd26ddc5acee0d4d4c14c87c",
    "cat3_gf2_shorter_ct": "37c9e579dd875cbbe79e00a4a5f3fdf3dd3ab5d9f67fc8afcdf7a57ca7aae67a",
    "cat3_gf2_shorter_ot": "f7c6549d014eaa01da58f29a231b5d8807adaea94b4c59535e9abb55c8364280",
    "cat5_gf16_fast_ct": "b3ca0d0f9cc7923f7650ed49eaa20d9cfbb16323c97c424f7577fc5eddc3ec2b",
    "cat5_gf16_fast_ot": "e4c35607ce6d54a68d6ada4e21f8ab2dfae3a6a886751bbd0b537331246d425d",
    "cat5_gf16_short_ct": "6f04fff1a052f7cac2e873f7cab731eafa88271d5c34e70e657c1ef2f99f6aca",
    "cat5_gf16_short_ot": "a5a486c3e7a7f9042dffc542e206ddbfaf902e0db2bd45868fee0cc57015eeca",
    "cat5_gf2_shorter_ct": "0a129fcef1310037b80c62764a6ea46a41ad37624f241e09d33ebf03c876725a",
    "cat5_gf2_shorter_ot": "7702beaee0be6a39448b3e9a6c5a82bb49aa2e5495c5cb497131bd142c629a47",
}

# What the review states.
REVIEW_OFFSET_BITS = {8192: 7.5, 4096: 7.0, 2048: 6.5, 256: 5.1}   # lambda - log2(queries)
REVIEW_CAT1_LOG2_QUERIES = (120.5, 122.9)
REVIEW_CAT1_LOG2_GATES = (135.5, 137.9)
# NIST comparison levels as quoted by the specification (physical p.83), and
# the cost of one cipher call: 2^15 at Category 1 (specification physical p.91:
# "T_AES is 2^15 or 2^16, depending on the security level"; 2^143 / 2^128).
NIST_LOG2_GATES = {128: 143, 192: 207, 256: 272}
LOG2_GATES_PER_QUERY = {128: 15, 192: 15, 256: 16}

# Whitespace-free strings expected in the pdftotext output of each physical page.
SPEC_MARKERS = {
    74: ["Theorem5.1", "EUF-KO", "(4N+1)·QRO·QIC", "QRO≤2λ", "QIC≤2λ−1", "QSign≤264"],
    76: ["4N·QRO·QIC", "(2λ−2λ−1)2", "(15)", "(16)"],
    77: ["4N·QRO·QIC", "2−4·", "(18)", "(19)"],
    91: ["TAESis215or216"],
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_header(text: str) -> dict:
    """Read the proof-system parameters of one submitted parameter header."""
    def macro(name: str) -> str:
        found = re.findall(rf"^#define MQOM3_PARAM_{name}[ \t]+(.+?)[ \t]*(?:/\*.*)?$", text, re.M)
        if len(found) != 1:
            raise ValueError(f"MQOM3_PARAM_{name} defined {len(found)} times")
        return found[0]

    lam = int(macro("SECURITY"))
    ext = int(macro("EXT_FIELD"))
    eta = macro("ETA")
    if eta == "(MQOM3_PARAM_SECURITY/MQOM3_PARAM_EXT_FIELD)":
        if lam % ext:
            raise ValueError("eta is not an integer")
        eta_value = lam // ext
    else:
        eta_value = int(eta)
    return {
        "lambda": lam,
        "one_tree": int(macro("OT_VARIANT")) == 1,
        "tau": int(macro("TAU")),
        "N": 1 << int(macro("NB_EVALS_LOG")),
        "w": int(macro("W")),
        "eta": eta_value,
        "log2_K": ext,
    }


def bound(p: dict, q_ro: int, q_ic: int, with_term: bool = True):
    """Theorem 5.1 without eps_mq, as (total, value of the (4N+1) term)."""
    lam = p["lambda"]
    two_lam = Fraction(1, 1 << lam)
    term = Fraction((4 * p["N"] + 1) * q_ro * q_ic, 1 << (2 * lam))
    total = (
        Fraction(q_ro, 1 << (p["log2_K"] * p["eta"]))
        + Fraction(1, 1 << p["w"]) * Fraction(2, p["N"]) ** p["tau"] * Fraction(4 * q_ic, 3)
        + Fraction(3 * q_ro * q_ro, 1 << (2 * lam))
        + (term if with_term else 0)
        + Fraction(1, 64) * q_ic * two_lam
        + Fraction(1, 1 << 68) * q_ro * two_lam
        + two_lam
    )
    return total, term


def last_query_count_below_one(p: dict, with_term: bool = True) -> int:
    """Largest Q with bound(Q, Q) < 1; the bound is increasing in Q."""
    low, high = 0, 1 << p["lambda"]            # bound(0) = 2^-lambda < 1 <= bound(2^lambda)
    assert bound(p, low, low, with_term)[0] < 1 <= bound(p, high, high, with_term)[0]
    while high - low > 1:
        middle = (low + high) // 2
        if bound(p, middle, middle, with_term)[0] < 1:
            low = middle
        else:
            high = middle
    return low


def spec_text_check(pdftotext: str):
    """Look up the transcribed terms in the specification text, page by page."""
    missing = []
    for page, markers in SPEC_MARKERS.items():
        try:
            done = subprocess.run(
                [pdftotext, "-f", str(page), "-l", str(page), str(SPEC), "-"],
                capture_output=True, text=True, timeout=120, check=False,
            )
        except (OSError, subprocess.TimeoutExpired):
            return None
        if done.returncode != 0:
            return None
        flat = re.sub(r"\s+", "", done.stdout)
        missing += [f"p.{page}: {marker}" for marker in markers if marker not in flat]
    return missing


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    checks, measurements = {}, {}
    reference = PACKAGE / "Reference_Implementation"
    optimized = PACKAGE / "Optimized_Implementation"
    headers = {
        name: reference / f"mqom3_{name}" / f"mqom3_parameters_{name}.h" for name in HEADER_SHA256
    }
    found = sorted(path.name for path in reference.glob("mqom3_cat*"))
    checks["eighteen_parameter_set_directories"] = found == sorted(f"mqom3_{name}" for name in HEADER_SHA256)
    checks["parameter_headers_match_pinned_sha256"] = all(
        path.is_file() and sha256(path) == HEADER_SHA256[name] for name, path in headers.items()
    )
    checks["optimized_tree_has_the_same_parameter_headers"] = all(
        (optimized / f"mqom3_{name}" / f"mqom3_parameters_{name}.h").is_file()
        and sha256(optimized / f"mqom3_{name}" / f"mqom3_parameters_{name}.h") == HEADER_SHA256[name]
        for name in HEADER_SHA256
    )
    checks["specification_pdf_matches_pinned_sha256"] = SPEC.is_file() and sha256(SPEC) == SPEC_SHA256

    pdftotext = shutil.which("pdftotext")
    missing = spec_text_check(pdftotext) if pdftotext and checks["specification_pdf_matches_pinned_sha256"] else None
    if missing is None:
        measurements["specification_text_lookup"] = "not run: pdftotext not available"
    else:
        measurements["specification_text_lookup"] = "run with pdftotext on physical pages 74, 76, 77, 91"
        measurements["specification_text_markers_missing"] = missing
        checks["transcribed_terms_found_in_specification_text"] = not missing

    rows = []
    try:
        parameters = {name: parse_header(path.read_text()) for name, path in headers.items()}
    except (OSError, ValueError) as error:
        parameters = {}
        measurements["header_parse_error"] = str(error)
    checks["parameters_read_from_18_headers"] = len(parameters) == 18

    for name, p in sorted(parameters.items()):
        lam, n = p["lambda"], p["N"]
        q_last = last_query_count_below_one(p)                 # bound < 1 here ...
        total, term = bound(p, q_last + 1, q_last + 1)         # ... and >= 1 here
        without_term_there, _ = bound(p, q_last + 1, q_last + 1, with_term=False)
        q_last_without = last_query_count_below_one(p, with_term=False)
        # Smallest Q at which the term alone is at least one.
        q_term = math.isqrt((1 << (2 * lam)) // (4 * n + 1))
        while (4 * n + 1) * q_term * q_term < (1 << (2 * lam)):
            q_term += 1
        log2_q = math.log2(q_last + 1)
        # As the query count approaches the boundary the bound approaches one,
        # so queries/bound approaches the boundary query count itself.  The
        # review report's "lambda - 7 to lambda - 4.2" is this ratio at the last
        # power-of-two query count below the boundary, which is larger.
        power = (q_last + 1).bit_length() - 1
        ratio_at_power_of_two = power - math.log2(bound(p, 1 << power, 1 << power)[0])
        rows.append({
            "parameter_set": f"mqom3_{name}",
            "lambda": lam,
            "variant": "one-tree" if p["one_tree"] else "correlated-tree",
            "tau": p["tau"],
            "N": n,
            "w": p["w"],
            "eta": p["eta"],
            "log2_K": p["log2_K"],
            "premise_2_over_N_to_tau_at_most_2^-72_over_lambda":
                Fraction(2, n) ** p["tau"] <= Fraction(1, (1 << 72) * lam),
            "K_to_eta_is_2^lambda": p["log2_K"] * p["eta"] == lam,
            "tau_log2_N_over_2_plus_w_is_lambda": p["tau"] * (n.bit_length() - 2) + p["w"] == lam,
            "log2_queries_where_bound_reaches_one": round(log2_q, 3),
            "bits_below_2^lambda": round(lam - log2_q, 3),
            "inside_theorem_range_Q_IC_at_most_2^(lambda-1)": q_last + 1 <= 1 << (lam - 1),
            "share_of_the_4N_plus_1_term_there": round(float(term / total), 4),
            "bits_below_2^lambda_term_alone": round(lam - math.log2(q_term), 3),
            "control_bound_without_term_at_same_queries": round(float(without_term_there), 4),
            "control_bits_below_2^lambda_without_term": round(lam - math.log2(q_last_without + 1), 3),
            "log2_gates_at_bound_one": round(log2_q + LOG2_GATES_PER_QUERY[lam], 3),
            "log2_queries_over_bound_at_last_power_of_two_below": round(ratio_at_power_of_two, 3),
            "nist_log2_gates": NIST_LOG2_GATES[lam],
            "_exact": (total, without_term_there, term),
        })

    if rows:
        def one_decimal(value):
            return round(value + 1e-12, 1)

        by_n = {}
        for row in rows:
            by_n.setdefault(row["N"], set()).add(row["bits_below_2^lambda"])
        cat1 = [row for row in rows if row["lambda"] == 128]
        cat1_queries = sorted(row["log2_queries_where_bound_reaches_one"] for row in cat1)
        cat1_gates = sorted(row["log2_gates_at_bound_one"] for row in cat1)

        checks["N_values_are_8192_4096_2048_256"] = sorted(by_n) == sorted(REVIEW_OFFSET_BITS)
        checks["theorem_premise_holds_for_18_sets"] = all(
            row["premise_2_over_N_to_tau_at_most_2^-72_over_lambda"] for row in rows
        )
        checks["bound_reaches_one_inside_the_theorem_query_range"] = all(
            row["inside_theorem_range_Q_IC_at_most_2^(lambda-1)"] for row in rows
        )
        checks["bound_reaches_one_7.5_7.0_6.5_5.1_bits_below_2^lambda"] = all(
            {one_decimal(value) for value in by_n.get(n, set())} == {offset}
            for n, offset in REVIEW_OFFSET_BITS.items()
        )
        checks["category_1_bound_reaches_one_at_2^120.5_to_2^122.9_queries"] = bool(cat1) and (
            one_decimal(cat1_queries[0]), one_decimal(cat1_queries[-1])
        ) == REVIEW_CAT1_LOG2_QUERIES
        checks["category_1_that_is_2^135.5_to_2^137.9_gates_below_2^143"] = bool(cat1) and (
            one_decimal(cat1_gates[0]), one_decimal(cat1_gates[-1])
        ) == REVIEW_CAT1_LOG2_GATES and cat1_gates[-1] < NIST_LOG2_GATES[128]
        checks["the_4N_plus_1_term_is_over_90_percent_of_the_bound_there"] = all(
            row["_exact"][2] > Fraction(9, 10) * row["_exact"][0] for row in rows
        )
        checks["the_term_alone_reaches_one_at_2^(lambda-1-log2(N)/2)"] = all(
            abs(row["bits_below_2^lambda_term_alone"] - (1 + (row["N"].bit_length() - 1) / 2)) < 0.002
            for row in rows
        )
        # Control: the same bound without the term is far below one at those
        # query counts, and reaches one only 1.7 bits below 2^lambda.
        checks["control_without_the_term_bound_is_below_0.1_at_the_same_queries"] = all(
            row["_exact"][1] < Fraction(1, 10) for row in rows
        )
        checks["control_without_the_term_bound_reaches_one_1.7_bits_below_2^lambda"] = all(
            one_decimal(row["control_bits_below_2^lambda_without_term"]) == 1.7 for row in rows
        )

        def single(values):
            """One number when all parameter sets of a group agree, else the list."""
            distinct = sorted(set(values))
            return distinct[0] if len(distinct) == 1 else distinct

        # The offsets below 2^lambda depend on N only (to the printed precision).
        measurements["by_N"] = {
            str(n): {
                "bits_below_2^lambda": single(r["bits_below_2^lambda"] for r in rows if r["N"] == n),
                "review_bits_below_2^lambda": REVIEW_OFFSET_BITS.get(n),
                "bits_below_2^lambda_term_alone": single(
                    r["bits_below_2^lambda_term_alone"] for r in rows if r["N"] == n),
                "share_of_the_term_at_bound_one": single(
                    r["share_of_the_4N_plus_1_term_there"] for r in rows if r["N"] == n),
                "control_bound_without_term_at_same_queries": single(
                    r["control_bound_without_term_at_same_queries"] for r in rows if r["N"] == n),
                "control_bits_below_2^lambda_without_term": single(
                    r["control_bits_below_2^lambda_without_term"] for r in rows if r["N"] == n),
            }
            for n in sorted(by_n, reverse=True)
        }
        measurements["by_category_and_N"] = {
            f"category_{category}": {
                str(n): {
                    "parameter_sets": [r["parameter_set"] for r in rows if (r["lambda"], r["N"]) == (lam, n)],
                    "log2_queries_at_bound_one": single(
                        r["log2_queries_where_bound_reaches_one"]
                        for r in rows if (r["lambda"], r["N"]) == (lam, n)),
                    "log2_gates_at_bound_one": single(
                        r["log2_gates_at_bound_one"] for r in rows if (r["lambda"], r["N"]) == (lam, n)),
                    "nist_log2_gates": NIST_LOG2_GATES[lam],
                }
                for n in sorted({r["N"] for r in rows if r["lambda"] == lam}, reverse=True)
            }
            for category, lam in ((1, 128), (3, 192), (5, 256))
        }
        measurements["sets_with_K_to_eta_equal_to_2^lambda"] = sum(r["K_to_eta_is_2^lambda"] for r in rows)
        measurements["sets_with_tau_log2_N_over_2_plus_w_equal_to_lambda"] = sum(
            r["tau_log2_N_over_2_plus_w_is_lambda"] for r in rows
        )
        measurements["category_1_log2_queries_min_max"] = [cat1_queries[0], cat1_queries[-1]] if cat1 else []
        measurements["category_1_log2_gates_min_max"] = [cat1_gates[0], cat1_gates[-1]] if cat1 else []
        measurements["category_1_log2_gates_min_max_counting_both_oracles"] = (
            [round(cat1_gates[0] + 1, 3), round(cat1_gates[-1] + 1, 3)] if cat1 else []
        )
        measurements["log2_gates_per_query_assumed"] = {
            str(lam): bits for lam, bits in LOG2_GATES_PER_QUERY.items()
        }
        measurements["min_share_of_the_term_at_bound_one"] = min(
            row["share_of_the_4N_plus_1_term_there"] for row in rows
        )
        measurements["max_control_bound_without_term_at_same_queries"] = max(
            row["control_bound_without_term_at_same_queries"] for row in rows
        )
        # Stated for completeness: without the term the bound would still
        # reach one below the NIST comparison, by 1.7 bits instead of 5.1-7.5.
        measurements["category_1_log2_gates_at_bound_one_without_term"] = sorted(
            {round(128 - row["control_bits_below_2^lambda_without_term"] + LOG2_GATES_PER_QUERY[128], 3)
             for row in cat1}
        )
        measurements["category_1_log2_queries_over_bound_at_last_power_of_two_below_by_N"] = {
            str(row["N"]): row["log2_queries_over_bound_at_last_power_of_two_below"]
            for row in sorted(cat1, key=lambda row: -row["N"])
        }
        measurements["query_model"] = (
            "Q_RO = Q_IC, each counted once; random-oracle and ideal-cipher queries of a "
            "key-only adversary (Q_Sign does not appear in the bound)"
        )
        measurements["parameter_sets_evaluated"] = len(rows)

    result = {
        "candidate": CANDIDATE,
        "evidence": EVIDENCE,
        "bound": BOUND,
        "term": TERM,
        "term_source": (
            "Theorem 5.1, specification physical p.74 (printed p.70); derived in Eq. (15)-(16), "
            "physical p.76, and combined with 2^-4*Q_IC*Q_RO/2^(2*lambda) of Eq. (18) in Eq. (19), "
            "physical p.77"
        ),
        "checks": checks,
        "measurements": measurements,
        "source_sha256": {
            "MQOM-spec.pdf": SPEC_SHA256,
            **{
                f"submission/submission_package_v3/Reference_Implementation/mqom3_{name}/"
                f"mqom3_parameters_{name}.h": digest
                for name, digest in HEADER_SHA256.items()
            },
        },
        "status": "PASS" if checks and all(checks.values()) and rows else "FAIL",
    }
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded)
    print(encoded, end="")
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
