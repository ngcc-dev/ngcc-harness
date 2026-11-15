#!/usr/bin/env python3
"""QR-UOV-6: the only rank threshold the specification states is Delta = 2; the
code uses 3 (q = 31) and 4 (q = 7), and Theorem 2's premise depends on which.

Section 4.5 of the QR-UOV specification (page 23; printed and physical page
numbers agree) says "we set Delta = 2 for all parameter sets".  Appendix B
(page 68) adds nine parameter sets with q = 7 and q = 31 and states no Delta.
The submitted qruov_param.h sets QRUOV_delta to 2 for q = 127, 3 for q = 31
and 4 for q = 7, and qruov_common.c rejects when rank < m - QRUOV_delta.

Theorem 2 (page 37) supposes qsign * eps_rank < 1/2, with eps_rank estimated
on page 37 as the probability that a uniform m x m matrix over F_q has rank
below m - Delta.  This script

  1. checks the quoted statements in the pinned specification (pdftotext) and
     the Delta values and parameter sets in the pinned headers;
  2. computes eps_rank(q, m, Delta) exactly (rational arithmetic) for every
     submitted set, with m read from the header, by the formula printed on
     page 37 and by counting matrices of each rank;
  3. finds where qsign * eps_rank reaches 1/2.  For the nine Appendix B sets
     Delta = 2 gives 2^43.5 (q = 31, eps = 2^-44.54) and 2^24.0 (q = 7,
     eps = 2^-25.01) signatures; the code's Delta = 3 and 4 give eps = 2^-79.22
     and 2^-69.93, and the premise holds through 2^64.

Control: for the six main sets (q = 127) Table 2 and the header both give
Delta = 2, so there is one value of eps_rank (2^-62.887, the specification's
1.173e-19) and one boundary; the specification's own limit of 2^61
signatures satisfies the premise.

Scope of the sentence: the same subsection says "For all parameter sets, we
choose q = 127", so "all parameter sets" in Section 4.5 may mean the six main
sets.  Under either reading the specification states no Delta for the
Appendix B sets other than 2: the values 3 and 4 appear only in the header.

This is a statement about the premise of a proof under the specification's
random-matrix estimate.  It is not an attack, no signature is produced and
no submission code is built or run.  Python standard library only, plus
pdftotext for step 1.

Exit status: 0 PASS, 1 FAIL, 77 SKIP (pdftotext is missing).
"""

import argparse
import hashlib
import json
import math
import re
import shutil
import subprocess
import unicodedata
from fractions import Fraction
from pathlib import Path


HERE = Path(__file__).resolve().parent
SPEC = HERE / "QR-UOV-spec.pdf"
SPEC_SHA256 = "23100e20c4b442c6e30ee449def82158c7b6ac5e4eb5c77869587075ddba9958"
PACKAGE = "submission/QR_UOV - Round3"
TREES = (
    "Reference_Implementation/ref",
    "Optimized_Implementation/opt",
    "Optimized_Implementation/avx2",
)
SOURCE_SHA256 = {
    "qruov_param.h": "4986e3bc6c3aebfb51ed25dfb43fb2d393948192c83d30aaae40af8f0b92f7c3",
    "qruov_common.c": "0a551833d268ea855851a4c3d4f6517d7d70219b88d3873a0f51caaccd5d8117",
}
RANK_TEST = "if (rank < QRUOV_m - QRUOV_delta) {"
CANDIDATE = "QR-UOV-6"

NIST_LOG2_SIGNING_QUERIES = 64
SPEC_LOG2_SIGNATURE_LIMIT = 61           # page 38: "qsign <= 2^61"

# Figures stated for this item in the review of 2026-11-15.
REVIEW = {
    "code_delta": {127: 2, 31: 3, 7: 4},
    "log2_eps_spec_delta": {31: -44.54, 7: -25.01},
    "log2_premise_boundary_spec_delta": {31: 43.5, 7: 24.0},
    "log2_eps_code_delta": {31: -79.22, 7: -69.93},
}

# Text as pdftotext -layout prints it, with the page (printed = physical).
SPEC_QUOTES = {
    "changelog_rule": (
        4, "We therefore choose ∆ as small as possible, subject to the condition "
           "that, for a realistic upper bound on the number of sig- natures "
           "generated, the probability of encountering a matrix L with rank less "
           "than m − ∆ remains below 1/2 for the security proof."),
    "algorithm_2_rank_test": (18, "10: if rank(L) < m − ∆ then 11: return ⊥"),
    "section_4_5_title": (
        21, "4.5 Parameter Sets This subsection specifies the proposed QR-UOV "
            "parameter sets"),
    "section_4_5_q_127_for_all_parameter_sets": (
        21, "Base field. For all parameter sets, we choose q = 127 for its "
            "implementation efficiency."),
    "section_4_5_delta_2_for_all_parameter_sets": (
        23, "Rank threshold. For the rank-restriction parameter ∆, we set ∆ = 2 "
            "for all parameter sets."),
    "section_4_5_estimate": (
        23, "Modeling L as a uniformly random matrix gives Pr[rank(L) < m−2] < "
            "2−62.88 ."),
    "lemma_3_premise": (36, "We assume qsign εrank < 1/2."),
    "theorem_2": (37, "Theorem 2 (UOV+QR-MQ ⇒ EUF-CMA)."),
    "theorem_2_premise": (37, "and we suppose qsign εrank < 1/2."),
    "eps_rank_definition": (
        37, "Therefore, the probability that A does not satisfy rank(A) ≥ m − ∆ is"),
    "proposed_sets_delta_2": (
        37, "For the proposed parameter sets, we set ∆ = 2. For q = 127 and every "
            "proposed value of m, the resulting rank-rejection probability is "
            "approximately"),
    "eps_rank_value": (37, "εrank (127, m, 2) ≈ 1.173 · 10−19 < 2−62.88 ."),
    "signature_limit": (
        38, "limiting the number of signature generations per key to qsign ≤ 261 "
            "gives"),
    "appendix_b": (
        68, "B Additional Parameters This section provides the additional "
            "parameter sets of QR-UOV. We use q = 7 and q = 31 as the "
            "finite-field sizes."),
}
APPENDIX_B_PAGES = (68, 69)


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def squash(text):
    """Drop all white space; fold ligatures and the Unicode minus sign."""
    text = unicodedata.normalize("NFKC", text).replace("−", "-")
    return "".join(text.split())


# --------------------------------------------------------------------------
# 1. Specification text and submitted headers
# --------------------------------------------------------------------------

def spec_pages(tool):
    """Text of each physical page; empty if pdftotext fails."""
    try:
        text = subprocess.run(
            [tool, "-layout", str(SPEC), "-"], capture_output=True, check=True,
            timeout=120,
        ).stdout.decode("utf-8", "replace")
    except (OSError, subprocess.SubprocessError):
        return []
    return text.split("\f")


def tool_version(tool):
    try:
        banner = subprocess.run(
            [tool, "-v"], capture_output=True, timeout=30,
        ).stderr.decode("utf-8", "replace")
    except (OSError, subprocess.SubprocessError):
        return "unknown"
    return banner.splitlines()[0].strip() if banner.strip() else "unknown"


def check_spec(pages):
    def page(number):
        return pages[number - 1] if number <= len(pages) else ""

    found = {
        name: squash(quote) in squash(page(number))
        for name, (number, quote) in SPEC_QUOTES.items()
    }
    # Table 2 (page 22): "(q, v, m, l)  n N V M tau_n1 tau_n2 tau_m Delta".
    table_2 = {}
    for line in page(22).splitlines():
        row = re.search(r"\((127), *(\d+), *(\d+), *(\d+)\)((?:\s+\d+){8})\s*$", line)
        if row:
            q, v, m, ell = (int(row.group(i)) for i in range(1, 5))
            table_2[(q, v, m, ell)] = int(row.group(5).split()[-1])
    table_2_has_delta_column = any(
        re.search(r"\(q, v, m, ℓ\).*τm\s+∆\s*$", line) for line in page(22).splitlines())
    # Table 16 (page 68): "q v m l" rows of the additional sets.
    table_16 = set()
    for line in page(68).splitlines():
        row = re.fullmatch(r"\s*(?:I|III|V)?\s*(7|31)\s+(\d+)\s+(\d+)\s+(\d+)\s*", line)
        if row:
            table_16.add(tuple(int(x) for x in row.groups()))
    # Every "Delta = <number>" in the document.
    values = {}
    for number, text in enumerate(pages, start=1):
        for value in re.findall(r"∆=(\d+)", squash(text)):
            values.setdefault(int(value), []).append(number)
    appendix_b_mentions = sum(page(number).count("∆") for number in APPENDIX_B_PAGES)
    return found, table_2, table_2_has_delta_column, table_16, values, appendix_b_mentions


def parse_header(text):
    """Parameter sets and the Delta rule of qruov_param.h."""
    sets = {}
    for name, body in re.findall(
            r"^#if defined\(QRUOV_PARAM_(\w+)\)\n((?:#define QRUOV_\w+ +\d+\n)+)#endif", text, re.M):
        macros = {key: int(value) for key, value in re.findall(r"#define QRUOV_(\w+) +(\d+)", body)}
        sets[name] = {key: macros[key] for key in ("security", "q", "L", "v", "m")}
    delta = {
        int(q): int(value)
        for q, value in re.findall(
            r"^#(?:if|elif) QRUOV_q == (\d+)\n#define QRUOV_delta +(\d+)$", text, re.M)
    }
    return sets, delta


# --------------------------------------------------------------------------
# 2. eps_rank, exactly
# --------------------------------------------------------------------------

def rank_counts(q, m):
    """Number of m x m matrices over F_q of each rank 0..m.

    N_r = [m choose r]_q * prod_{i<r} (q^m - q^i): choose the column space,
    then a surjection onto it.
    """
    counts, gaussian, surjections = [], 1, 1
    for r in range(m + 1):
        counts.append(gaussian * surjections)
        if r < m:
            gaussian, remainder = divmod(gaussian * (q ** (m - r) - 1), q ** (r + 1) - 1)
            assert remainder == 0
            surjections *= q ** m - q ** r
    return counts


def p_printed(q, m, a):
    """p_{m,q}(a) as printed on page 37:
    q^(-a^2) * prod_{j=a+1..m} (1 - q^-j)^2 / prod_{j=1..m-a} (1 - q^-j)."""
    numerator = math.prod(q ** j - 1 for j in range(a + 1, m + 1)) ** 2
    numerator_power = 2 * sum(range(a + 1, m + 1))          # divided by q^this
    denominator = math.prod(q ** j - 1 for j in range(1, m - a + 1))
    denominator_power = sum(range(1, m - a + 1))            # divided by q^this
    return Fraction(numerator * q ** denominator_power,
                    denominator * q ** (numerator_power + a * a))


def eps_rank(counts, q, m, delta):
    """Pr[rank < m - delta] for a uniform matrix: sum_{a > delta} p_{m,q}(a)."""
    return Fraction(sum(counts[:m - delta]), q ** (m * m))


def log2_fraction(value):
    return math.log2(value.numerator) - math.log2(value.denominator)


def premise_holds(eps, log2_qsign):
    """qsign * eps_rank < 1/2 at qsign = 2^log2_qsign."""
    return (1 << log2_qsign) * eps < Fraction(1, 2)


def smallest_delta(counts, q, m, log2_qsign):
    """Smallest Delta with 2^log2_qsign * eps_rank < 1/2 (the rule of page 4)."""
    return next(d for d in range(m + 1) if premise_holds(eps_rank(counts, q, m, d), log2_qsign))


# --------------------------------------------------------------------------

def emit(result, output):
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if output:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(encoded)
    print(encoded, end="")
    return {"PASS": 0, "SKIP": 77}.get(result["status"], 1)


def main():
    parser = argparse.ArgumentParser(description=" ".join(__doc__.split("\n\n")[0].split()))
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    result = {
        "candidate": CANDIDATE,
        "evidence": (
            "The pinned specification states Delta = 2 (Section 4.5, p.23, 'for all "
            "parameter sets'; Table 2; p.37) and no Delta for the nine Appendix B "
            "sets, while the pinned qruov_param.h uses Delta = 3 for q = 31 and 4 for "
            "q = 7; exact evaluation of the specification's rank estimate shows that "
            "Theorem 2's premise qsign*eps_rank < 1/2 would fail at 2^43.5 (q = 31) "
            "and 2^24.0 (q = 7) signatures with Delta = 2 and holds through 2^64 with "
            "the code's values, while for the six q = 127 sets specification and code "
            "agree; this is a premise of a proof under the specification's "
            "random-matrix estimate, not an attack, the submitted code already uses "
            "the larger thresholds, and no submission code is built or run."
        ),
        "source_sha256": {
            "QR-UOV/QR-UOV-spec.pdf": SPEC_SHA256,
            **{f"QR-UOV/{PACKAGE}/{tree}/{name}": digest
               for tree in TREES for name, digest in SOURCE_SHA256.items()},
        },
        "checks": {},
        "measurements": {},
    }
    tool = shutil.which("pdftotext")
    if tool is None:
        result["status"] = "SKIP"
        result["reason"] = "pdftotext not found"
        return emit(result, args.output)

    checks = {
        "pinned_specification": SPEC.is_file() and sha256(SPEC) == SPEC_SHA256,
        "pinned_headers_and_signing_source_in_three_trees": all(
            (HERE / PACKAGE / tree / name).is_file()
            and sha256(HERE / PACKAGE / tree / name) == digest
            for tree in TREES for name, digest in SOURCE_SHA256.items()
        ),
    }
    if not checks["pinned_headers_and_signing_source_in_three_trees"]:
        result.update({"checks": checks, "status": "FAIL"})
        return emit(result, args.output)

    # 1. Specification text and headers.
    pages = spec_pages(tool) if checks["pinned_specification"] else []
    found, table_2, table_2_delta_column, table_16, delta_values, appendix_b_mentions = \
        check_spec(pages)
    reference = HERE / PACKAGE / TREES[0]
    sets, code_delta = parse_header((reference / "qruov_param.h").read_text())
    main_sets = {name: p for name, p in sets.items() if p["q"] == 127}
    additional = {name: p for name, p in sets.items() if p["q"] != 127}

    def as_rows(group):
        return {(p["q"], p["v"], p["m"], p["L"]) for p in group.values()}

    checks.update({
        "spec_says_delta_2_for_all_parameter_sets_on_page_23":
            found["section_4_5_delta_2_for_all_parameter_sets"] and found["section_4_5_estimate"],
        "spec_table_2_gives_delta_2_for_the_six_main_sets":
            table_2_delta_column and len(table_2) == 6 and set(table_2.values()) == {2},
        "spec_theorem_2_supposes_qsign_eps_rank_below_one_half":
            found["theorem_2"] and found["theorem_2_premise"] and found["lemma_3_premise"]
            and found["eps_rank_definition"],
        "spec_other_quoted_statements_found": all(found.values()),
        "spec_appendix_b_states_no_delta": bool(pages) and appendix_b_mentions == 0
            and found["appendix_b"],
        "spec_states_no_delta_other_than_2_and_0":
            bool(delta_values) and set(delta_values) <= {0, 2},
        "header_delta_is_2_3_4_for_q_127_31_7": code_delta == REVIEW["code_delta"],
        "signing_code_rejects_rank_below_m_minus_delta":
            (reference / "qruov_common.c").read_text().count(RANK_TEST) == 1,
        "header_has_six_main_and_nine_additional_sets":
            len(main_sets) == 6 and len(additional) == 9
            and {p["q"] for p in additional.values()} == {7, 31},
        "header_sets_equal_spec_table_2_and_table_16":
            as_rows(main_sets) == set(table_2) and as_rows(additional) == table_16,
    })

    # 2. and 3. Exact eps_rank and the premise boundary.
    rows, counts_by_qm = [], {}
    count_identity = printed_formula = True
    for name, p in sorted(sets.items(), key=lambda item: (-item[1]["q"], item[1]["security"],
                                                           item[1]["L"])):
        q, m = p["q"], p["m"]
        if (q, m) not in counts_by_qm:
            counts = counts_by_qm[(q, m)] = rank_counts(q, m)
            count_identity &= sum(counts) == q ** (m * m)
            printed_formula &= all(
                p_printed(q, m, a) == Fraction(counts[m - a], q ** (m * m)) for a in range(8))
        counts = counts_by_qm[(q, m)]
        row = {"parameter_set": name, "category": p["security"], "q": q, "v": p["v"], "m": m,
               "l": p["L"], "only_delta_stated_in_specification": 2,
               "delta_in_header": code_delta[q]}
        for label, delta in (("specification_delta", 2), ("header_delta", code_delta[q])):
            eps = eps_rank(counts, q, m, delta)
            row[label] = {
                "delta": delta,
                "log2_eps_rank": round(log2_fraction(eps), 3),
                # qsign * eps < 1/2 exactly when qsign < 1 / (2 eps).
                "log2_qsign_where_premise_fails": round(-1 - log2_fraction(eps), 3),
                "premise_holds_at_2^61": premise_holds(eps, SPEC_LOG2_SIGNATURE_LIMIT),
                "premise_holds_at_2^64": premise_holds(eps, NIST_LOG2_SIGNING_QUERIES),
                "_eps": eps,
            }
        row["smallest_delta_with_premise_at_2^61"] = smallest_delta(
            counts, q, m, SPEC_LOG2_SIGNATURE_LIMIT)
        row["smallest_delta_with_premise_at_2^64"] = smallest_delta(
            counts, q, m, NIST_LOG2_SIGNING_QUERIES)
        rows.append(row)

    extra = [row for row in rows if row["q"] != 127]
    main_rows = [row for row in rows if row["q"] == 127]

    def by_q(group, label, key, digits):
        """The distinct rounded values of one figure, per q."""
        out = {}
        for row in group:
            out.setdefault(row["q"], set()).add(round(row[label][key], digits))
        return out

    spec_eps = by_q(extra, "specification_delta", "log2_eps_rank", 2)
    spec_boundary = by_q(extra, "specification_delta", "log2_qsign_where_premise_fails", 1)
    code_eps = by_q(extra, "header_delta", "log2_eps_rank", 2)
    checks.update({
        "rank_counts_sum_to_all_matrices_and_equal_printed_formula":
            bool(rows) and count_identity and printed_formula,
        "additional_sets_specification_and_header_delta_differ":
            len(extra) == 9 and all(row["delta_in_header"] != 2 for row in extra),
        "additional_sets_eps_rank_with_delta_2_is_2^-44.54_and_2^-25.01":
            spec_eps == {q: {value} for q, value in REVIEW["log2_eps_spec_delta"].items()},
        "additional_sets_premise_with_delta_2_fails_at_2^43.5_and_2^24.0":
            spec_boundary == {
                q: {value} for q, value in REVIEW["log2_premise_boundary_spec_delta"].items()}
            and not any(row["specification_delta"]["premise_holds_at_2^61"] for row in extra)
            and not any(row["specification_delta"]["premise_holds_at_2^64"] for row in extra),
        "additional_sets_eps_rank_with_header_delta_is_2^-79.22_and_2^-69.93":
            code_eps == {q: {value} for q, value in REVIEW["log2_eps_code_delta"].items()},
        "additional_sets_premise_with_header_delta_holds_at_2^64":
            len(extra) == 9 and all(row["header_delta"]["premise_holds_at_2^64"] for row in extra),
        # Control: the six main sets, where specification and header agree.
        "control_main_sets_specification_and_header_delta_agree":
            len(main_rows) == 6
            and all(row["delta_in_header"] == 2 == table_2.get(
                (row["q"], row["v"], row["m"], row["l"])) for row in main_rows)
            and all(row["specification_delta"]["_eps"] == row["header_delta"]["_eps"]
                    for row in main_rows),
        "control_main_sets_eps_rank_is_the_specification_value_and_holds_at_2^61":
            len(main_rows) == 6
            and all(Fraction(1172, 10 ** 22) < row["header_delta"]["_eps"]
                    < Fraction(1173, 10 ** 22) for row in main_rows)
            and all(log2_fraction(row["header_delta"]["_eps"]) < -62.88 for row in main_rows)
            and all(row["header_delta"]["premise_holds_at_2^61"] for row in main_rows),
    })

    for row in rows:
        for label in ("specification_delta", "header_delta"):
            del row[label]["_eps"]

    result.update({
        "checks": checks,
        "controls": sorted(name for name in checks if name.startswith("control_")),
        "measurements": {
            "parameter_sets": rows,
            "header_delta_by_q": {str(q): value for q, value in sorted(code_delta.items())},
            "additional_sets_by_q": {
                str(q): {
                    "m": sorted(row["m"] for row in extra if row["q"] == q),
                    "specification_delta_2": {
                        "log2_eps_rank": sorted(spec_eps[q]),
                        "log2_qsign_where_premise_fails": sorted(spec_boundary[q]),
                    },
                    "header_delta": {
                        "delta": code_delta[q],
                        "log2_eps_rank": sorted(code_eps[q]),
                        "log2_qsign_where_premise_fails": sorted(by_q(
                            extra, "header_delta", "log2_qsign_where_premise_fails", 1)[q]),
                    },
                }
                for q in sorted(spec_eps)
            },
            "main_sets_q_127": {
                "log2_eps_rank": sorted(by_q(main_rows, "header_delta", "log2_eps_rank", 3)[127]),
                "log2_qsign_where_premise_fails": sorted(by_q(
                    main_rows, "header_delta", "log2_qsign_where_premise_fails", 3)[127]),
                "premise_holds_at_2^64": sorted(
                    {row["header_delta"]["premise_holds_at_2^64"] for row in main_rows}),
                "note": "the failure at 2^64 for q = 127 is QR-UOV-2, not this item",
            },
            "smallest_delta_meeting_the_page_4_rule": {
                str(q): {
                    "at_2^61": sorted({row["smallest_delta_with_premise_at_2^61"]
                                       for row in rows if row["q"] == q}),
                    "at_2^64": sorted({row["smallest_delta_with_premise_at_2^64"]
                                       for row in rows if row["q"] == q}),
                    "header": code_delta[q],
                }
                for q in sorted(code_delta)
            },
            "log2_signing_queries_nist_baseline": NIST_LOG2_SIGNING_QUERIES,
            "log2_signature_limit_stated_in_specification": SPEC_LOG2_SIGNATURE_LIMIT,
        },
        "review_figures": {
            key: {str(q): value for q, value in table.items()} for key, table in REVIEW.items()
        },
        "spec": {
            "extracted_with": tool_version(tool) + ", option -layout",
            "pages_are": "physical PDF pages, equal to the printed page numbers",
            "quotes": {
                name: {"page": page, "text": quote, "found": found[name]}
                for name, (page, quote) in SPEC_QUOTES.items()
            },
            "table_2_delta_by_main_set": {
                str(list(key)): value for key, value in sorted(table_2.items())},
            "table_16_additional_sets": sorted(list(row) for row in table_16),
            "pages_of_each_stated_delta_value": {
                str(value): numbers for value, numbers in sorted(delta_values.items())},
            "delta_mentions_in_appendix_b_pages_68_69": appendix_b_mentions,
            "scope_note": (
                "Section 4.5 introduces 'the proposed QR-UOV parameter sets' and also "
                "says 'For all parameter sets, we choose q = 127', so its 'Delta = 2 "
                "for all parameter sets' may be meant for the six main sets only; "
                "Appendix B gives no Delta for its nine sets.  Either way 2 is the "
                "only rank threshold the specification states for a parameter set "
                "(Delta = 0 on p.72 is a proof variant)."
            ),
        },
        "not_covered": [
            "eps_rank is the specification's estimate for a uniformly random matrix "
            "(p.37); the rank law of the real matrix L is not measured (the "
            "specification calls the estimate heuristic, p.38).",
            "No attack follows: Delta only decides when the signer gives up, the "
            "submitted code uses the larger thresholds, and verification does not "
            "depend on Delta.",
            "The additive terms of Theorem 2 are not evaluated here (QR-UOV-2).",
            "Whether 'all parameter sets' on p.23 was meant to include Appendix B is "
            "a question of reading; the script records both sentences.",
            "No submission code is built or run; the three trees' qruov_param.h and "
            "qruov_common.c are read and pinned.",
        ],
        "status": "PASS" if all(checks.values()) else "FAIL",
    })
    return emit(result, args.output)


if __name__ == "__main__":
    raise SystemExit(main())
