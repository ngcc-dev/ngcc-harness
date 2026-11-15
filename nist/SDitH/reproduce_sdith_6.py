#!/usr/bin/env python3
"""SDitH-6: the cipher-grinding pseudocode contradicts its own remark and the
code, and the two readings differ by one bit of forgery accounting.

ExpandEvaluationChallenge, cipher-based variant (SDitH specification, physical
PDF page 21, printed page 19):

     1: xof <- XOF.Init(h_piop, ctr)
     2: (p0 || p1 || k0 || k1) <- xof.GetBytes(4 * lambda)
     ...
    10: v_pow <- Extract_{w_pow - 1}(c0 xor c1)
    11: xof <- XOF.Init(h_piop, ctr, c0, c1)

The remark below it (physical page 22, printed page 20) says "Lines 1-4 and
the key schedule of the encryption call can be factorized, as they do not
depend on the counter".  As written, line 1 takes the counter.  The submitted
code follows the remark, not line 1: proofow_init_cipher_cat{1,3,5}_ref
(sdith_prng.c, lines 323-347 for Category 1) derives p0, p1, k0, k1 once from
a prefix byte and h_piop, with no counter, and runs the key schedule there.

Accounting.  All twelve sets have tau*kappa + w_pow - log2(d) = lambda.  The
cipher variant tests w_pow - 1 bits (line 10; the code's mask), so one counter
value passes a false statement with probability 2^-(lambda-1) and a forger
needs 2^(lambda-1) of them.

  literal line 1   one XOF call (and two key schedules) per counter value:
                   2^(lambda-1) XOF calls, 2^lambda key schedules and
                   2^lambda cipher calls
  code / remark    two cipher calls per counter value under fixed keys:
                   2^lambda cipher calls and 2^(lambda - w_pow) XOF calls

This script

  1. checks the quoted lines and the remark in the pinned specification
     (pdftotext), and that GetBytes and the one-index Extract are used once
     and defined nowhere;
  2. checks the code facts in the pinned sources (24 identical copies of
     sdith_prng.c; the AVX2 file of the optimized trees);
  3. recomputes the accounting for the six cipher-grinding sets from
     sdith_signature_parameters.c (tau, kappa, w_pow, d = mux_depth), which
     is compared with Tables 4 and 5 of the specification.

Control: for the six hash-grinding sets the pseudocode (XOF.Init(h_piop, ctr),
w_pow bits) and the code agree, and both readings give 2^lambda XOF calls.

Line 2: the four values are lambda bits each, so 4*lambda bits are needed,
not 4*lambda bytes.  The code draws 64 bytes at Category 1 and 128 at
Category 5 (4*lambda bits), and 128 bytes at Category 3, which is 4*256 bits,
not 4*192.

This is the accounting of an inconsistent specification, not an attack.  The
specification has no EUF-CMA proof yet (physical page 46); no submission code
is built or run.  Python standard library only, plus pdftotext for step 1.

Exit status: 0 PASS, 1 FAIL, 77 SKIP (pdftotext is missing).
"""

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import unicodedata
from fractions import Fraction
from pathlib import Path


HERE = Path(__file__).resolve().parent
SPEC = HERE / "SDitH-spec.pdf"
SPEC_SHA256 = "22dfed06d179afa759139487dca9c8f8d4a7cfea22bcedcb7736b50ce5b01c83"
PACKAGE = "submission/submission_package"
TREES = ("Reference_Implementation", "Optimized_Implementation")
SETS = tuple(
    f"cat{category}_{speed}{suffix}"
    for suffix in ("", "_cipherpow") for category in (1, 3, 5) for speed in ("short", "fast")
)
# src/<file> in every parameter-set directory of the named trees.
SOURCE_SHA256 = {
    "sdith_prng.c": (TREES, "aa8b9d804759c70cf2f26391daa02b6a97e8c235b81af208cd6c86e473758322"),
    "sdith_signature.c": (TREES, "99685ad55272b44d5cc6f7ae20b4822d168a1bf75a8f9fcbb2e0e1f2d8dd7aec"),
    "sdith_signature_parameters.c":
        (TREES, "8a851aee6e180a1b775b7b6856b050db974e701551be94f1ee7f7923ebb26f70"),
    "vole_parameters.c": (TREES, "7ef2eb12a221f67c958b0ec983e6dd67555f9fce7e97c0e8c34eb2c9b6d76667"),
    "sdith_prng_avx2.c":
        (TREES[1:], "100a286b3254a8c05fccd26c25ccea725b1e064cd21d99ba61140dfecd1049dd"),
}
CANDIDATE = "SDitH-6"

CATEGORIES = {1: (128, "shake128"), 3: (192, "shake256"), 5: (256, "shake256")}
REVIEW_INIT_LINES = (323, 347)           # sdith_prng.c:323-347

# Text as pdftotext -layout prints it, with the physical PDF page (printed
# page = physical - 2).  Comparison ignores white space.
SPEC_QUOTES = {
    "grinding_rule": (12, "such that (d/N τ ) · 2−wpow ≤ 2−λ to achieve a λ-bit security."),
    "enc_sizes_are_lambda_bits": (
        17, "the key and plaintext sizes of both Enc and Enc∗ are always λ bits, and the "
            "ciphertext size of Enc∗ is also always λ bits."),
    "xof_api": (
        19, "Once initialized, the XOF can be queried with the routine XOF.GetByte() to "
            "gen- erate the next byte of the output y associated to x."),
    "hash_variant": (
        21, "hash-based grinding consists of the following steps: "
            "1: xof ← XOF.Init(hpiop , ctr) "
            "2: {i∗ [e]}e<τ ← SampleIntegers(xof, {0, . . . , 2κ − 1}, τ ) "
            "3: vpow ← SampleFieldElements(xof, F2 , wpow ) "
            "4: return ({i∗ [e]}e<τ , vpow )"),
    "cipher_variant_lines_1_to_4": (
        21, "The variant based on cipher-based grinding consists of the following steps: "
            "1: xof ← XOF.Init(hpiop , ctr) "
            "2: (p0 ∥ p1 ∥ k0 ∥ k1 ) ← xof.GetBytes(4 · λ) "
            "3: k0′ ← 0 ∥ Extract1→λ (k0 ) "
            "4: k1′ ← 1 ∥ Extract1→λ (k1 )"),
    "cipher_variant_lines_5_to_7": (
        21, "5: ctr bytes ← MapToBits(ctr) "
            "6: p′0 ← ctr bytes ∥ Extract32→λ−1 (p0 ) ∥ 1 "
            "7: p′1 ← ctr bytes ∥ Extract32→λ−1 (p1 ) ∥ 1"),
    "cipher_variant_lines_8_to_10": (
        21, "8: c0 ← Enc∗ (key = k0′ , ptx = p′0 ) "
            "9: c1 ← Enc∗ (key = k1′ , ptx = p′1 ) "
            "10: vpow ← Extractwpow −1 (c0 ⊕ c1 )"),
    "cipher_variant_lines_11_to_13": (
        21, "11: xof ← XOF.Init(hpiop , ctr, c0 , c1 ) "
            "12: {i∗ [e]}e<τ ← SampleIntegers(xof, {0, . . . , 2κ − 1}, τ ) "
            "13: return ({i∗ [e]}e<τ , vpow )"),
    "extract_definition": (
        22, "where Extractm→n (bitstring) takes as input a bitstring bitstring := "
            "(b0 , b1 , . . .) and returns the substring (bm , . . . , bn−1 )."),
    "remark": (
        22, "Let us stress that Lines 1–4 and the key schedule of the encryption call can "
            "be factorized, as they do not depend on the counter."),
    "parameter_rule": (41, "τ · κ − log2 (d) + wpow ≥ λ ,"),
    "no_proof_yet": (
        46, "formal proof of the EUF-CMA security of SD-in-the-Head will be included in a "
            "future version of"),
}
PRINTED_PAGE = {12: 10, 17: 15, 19: 17, 21: 19, 22: 20, 41: 39, 42: 40, 43: 41, 46: 44}


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def squash(text):
    """Drop all white space; fold ligatures and the Unicode minus sign."""
    text = unicodedata.normalize("NFKC", text).replace("−", "-")
    return "".join(text.split())


# --------------------------------------------------------------------------
# 1. Specification text
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


def printed_number(page):
    """The page number in the running head of a page."""
    head = next((line for line in page.splitlines() if line.strip()), "")
    found = re.fullmatch(r"\s*(?:(\d+)\s+The Syndrome .* Signature Scheme"
                         r"|The Syndrome .* Signature Scheme\s+(\d+))\s*", head)
    return int(found.group(1) or found.group(2)) if found else None


def check_spec(pages):
    def page(number):
        return pages[number - 1] if number <= len(pages) else ""

    found = {
        name: squash(quote) in squash(page(number))
        for name, (number, quote) in SPEC_QUOTES.items()
    }
    flat = [squash(text) for text in pages]
    # Every use of Extract with a subscript, and every page that names it.
    extract_uses = re.findall(r"Extract([^()]*)\(", squash(page(21)))
    extract_pages = [n for n, text in enumerate(flat, start=1) if "Extract" in text]
    getbytes_pages = [n for n, text in enumerate(flat, start=1) for _ in range(text.count("GetBytes"))]
    # Tables 4 and 5: kappa, tau, w_pow per parameter set.
    tables = {}
    for number in (42, 43):
        for level, speed, cipher, kappa, tau, w in re.findall(
                r"SDitH3-L(\d)-gf2-(short|fast)(-cipherpow)?\s+(\d+)\s+(\d+)\s+(\d+)\s", page(number)):
            name = f"cat{level}_{speed}" + ("_cipherpow" if cipher else "")
            tables[name] = {"kappa": int(kappa), "tau": int(tau), "w_pow": int(w)}
    printed = {number: printed_number(page(number)) for number in PRINTED_PAGE}
    return found, extract_uses, extract_pages, getbytes_pages, tables, printed


# --------------------------------------------------------------------------
# 2. Code facts
# --------------------------------------------------------------------------

def function(text, name):
    """(body, first line, last line) of the function `name`; its closing brace
    is the first line that is exactly '}'."""
    start = re.search(rf"^EXPORT \w+ {name}\(", text, re.M)
    if not start:
        return "", 0, 0
    end = text.index("\n}\n", start.start()) + 2
    first = text.count("\n", 0, start.start()) + 1
    return text[start.start():end], first, first + text.count("\n", start.start(), end - 1)


def cipher_init_facts(text, category, suffix):
    """What proofow_init_cipher_cat<category><suffix> does."""
    _, shake = CATEGORIES[category]
    body, first, last = function(text, f"proofow_init_cipher_cat{category}{suffix}")
    drawn = re.search(rf"xof_output_{shake}\(&xof, s->p, (\d+)\);", body)
    return {
        "lines": [first, last],
        "xof_absorbs_prefix_byte_then_h_piop_only": (
            f"xof_init_and_seed_{shake}(&xof, &PROOFOW_H0_PREFIX, 1);\n"
            f"  xof_seed_{shake}(&xof, h_piop, H_PIOP_BYTES);\n"
            f"  xof_finalize_{shake}(&xof);" in body
            and body.count("xof_seed_") == 1
        ),
        "no_counter_in_function": bool(body) and not re.search(r"\bctr\b", body),
        "key_schedule_here": len(re.findall(r"\w+_key_schedule_x2_\w+\(", body)) == 1,
        "mask_covers_w_minus_1_bits":
            "s->mask_w = (UINT64_C(1) << (proofow_w - 1)) - 1;" in body,
        "bytes_drawn_for_p0_p1_k0_k1": int(drawn.group(1)) if drawn else None,
    }


def cipher_grind_facts(text, category):
    """What proofow_grind_w_cipher_cat<category>_ref does for each counter."""
    _, shake = CATEGORIES[category]
    body, _, _ = function(text, f"proofow_grind_w_cipher_cat{category}_ref")
    loop = body[body.find("while (ctr < CTR_MAX) {"):]
    passed = loop[loop.find("& s->mask_w) == 0) {"):]
    return {
        "two_cipher_calls_per_counter":
            len(re.findall(r"_ctrle_nocarry_1block_ref\(", loop)) == 2
            and len(re.findall(r"_ctrle_nocarry_1block_ref\(", body)) == 2,
        "no_key_schedule_and_no_first_xof_per_counter":
            bool(loop) and "key_schedule" not in body and "PROOFOW_H0_PREFIX" not in body,
        "tests_masked_bits_of_c0_xor_c1":
            "if (((s->c[0].v64[0] ^ s->c[1].v64[0]) & s->mask_w) == 0) {" in loop,
        "second_xof_only_after_the_test_passes_with_prefix_h_piop_ctr_c": (
            f"xof_init_and_seed_{shake}(&xof, &PROOFOW_H1_PREFIX, 1);\n"
            f"      xof_seed_{shake}(&xof, s->h_piop, H_PIOP_BYTES);\n"
            f"      xof_seed_{shake}(&xof, &ctr, PROOFOW_CTR_REVEALED_BYTES);" in passed
            and body.count("xof_init_and_seed_") == 1
        ),
    }


def hash_grind_facts(text, category):
    """What proofow_grind_w_shake_cat<category> does for each counter."""
    _, shake = CATEGORIES[category]
    body, _, _ = function(text, f"proofow_grind_w_shake_cat{category}")
    init, _, _ = function(text, f"proofow_init_shake_cat{category}")
    loop = body[body.find("for (uint64_t ctr = *ctr_in_out; ctr < CTR_MAX; ctr++) {"):]
    # The finalize wrappers of the two SHAKE variants have the same body, and
    # the Category 5 function calls the one named shake128.
    wrappers = [
        function(text, f"xof_finalize_and_output_{name}")[0].replace(name, "") for name in
        ("shake128", "shake256")
    ]
    return {
        "one_xof_over_h_piop_and_counter_per_counter_no_prefix": bool(re.search(
            rf"xof_init_and_seed_{shake}\(&ctx, s->h_piop, H_PIOP_BYTES\);\n"
            rf"    xof_seed_{shake}\(&ctx, &ctr, 4\);\n"
            rf"    xof_finalize_and_output_shake(?:128|256)\(", loop))
            and bool(wrappers[0]) and wrappers[0] == wrappers[1]
            and body.count("xof_init_and_seed_") == 1 and "PROOFOW_H" not in body,
        "tests_w_bits": "s->w = proofow_w;" in init
            and "if (extract_kappabit_uint(s->w, s->kappa_tau, s->delta0_and_vgrind) == 0) {" in loop,
    }


def parse_parameters(text):
    """lambda, kappa, tau, w_pow, d and the grinding variant of each set."""
    sets = {}
    for name, body in re.findall(
            r"^const signature_parameters (\w+)_PARAMETERS = \{(.*?)^\s*\};", text, re.M | re.S):
        def field(key):
            return int(re.search(rf"\.{key} = (\d+),", body).group(1))
        arities = re.search(r"\.mux_arities = \{([^}]*)\}", body).group(1).split(",")
        sets[name.lower()] = {
            "lambda": field("lambda"), "kappa": field("kappa"), "tau": field("tau"),
            "w_pow": field("proofow_w"), "d": field("mux_depth"), "mux_arities": len(arities),
            "variant": re.search(r"\.proofow_variant = PROOFOW_VARIANT_(\w+)", body).group(1),
        }
    return sets


# --------------------------------------------------------------------------
# 3. Accounting
# --------------------------------------------------------------------------

def accounting(p):
    """Expected forgery work in log2, under the literal pseudocode and under
    the code, for one parameter set.

    One counter value passes a false statement with probability at most
    (d / N^tau) * 2^-(bits tested), N = 2^kappa (specification p.12).
    """
    cipher = p["variant"] == "CIPHER"
    log2_d = p["d"].bit_length() - 1
    tested = p["w_pow"] - 1 if cipher else p["w_pow"]
    success = Fraction(p["d"], 1 << (p["kappa"] * p["tau"])) / (1 << tested)
    attempts = 1 / success
    assert attempts.denominator == 1 and attempts.numerator & (attempts.numerator - 1) == 0
    log2_attempts = attempts.numerator.bit_length() - 1
    row = {
        "design_bits_tau_kappa_plus_w_minus_log2_d": p["kappa"] * p["tau"] + p["w_pow"] - log2_d,
        "grinding_bits_tested": tested,
        "log2_success_per_counter_value": -log2_attempts,
        "log2_counter_values_needed": log2_attempts,
    }
    if cipher:
        # Line 11 runs only when the tested bits are zero: 2^-(w-1) of the attempts.
        second_xof = log2_attempts - tested
        row["literal_line_1"] = {
            "log2_xof_calls": log2_attempts,                 # line 1, every counter value
            "log2_xof_calls_line_11": second_xof,
            "log2_key_schedules": log2_attempts + 1,
            "log2_cipher_calls": log2_attempts + 1,
        }
        row["code"] = {
            "log2_xof_calls": second_xof,                    # only the second XOF
            "log2_key_schedules": None,                      # once per h_piop
            "log2_cipher_calls": log2_attempts + 1,
        }
    else:
        both = {"log2_xof_calls": log2_attempts, "log2_key_schedules": None,
                "log2_cipher_calls": None}
        row["literal_line_1"], row["code"] = dict(both), dict(both)
    return row


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

    files = {
        f"{tree}/{name}/src/{source}": digest
        for source, (trees, digest) in SOURCE_SHA256.items() for tree in trees for name in SETS
    }
    result = {
        "candidate": CANDIDATE,
        "evidence": (
            "In the pinned specification the cipher-grinding ExpandEvaluationChallenge "
            "(physical p.21, printed p.19) starts with xof <- XOF.Init(h_piop, ctr) and "
            "takes p0, p1, k0, k1 from it, its own remark (physical p.22) says lines 1-4 "
            "and the key schedule do not depend on the counter, and the pinned code "
            "derives them once from a prefix byte and h_piop alone; with w_pow - 1 bits "
            "tested and tau*kappa + w_pow - log2(d) = lambda, a forger needs "
            "2^lambda cipher calls under either reading, and the literal line 1 adds "
            "2^(lambda-1) XOF calls and 2^lambda key schedules, so no reduction in "
            "forgery work follows; for the six hash-grinding sets pseudocode and code "
            "agree at 2^lambda XOF calls; this is a specification inconsistency, not an "
            "attack, the "
            "specification has no EUF-CMA proof yet, and no submission code is built "
            "or run."
        ),
        "source_sha256": {
            "SDitH/SDitH-spec.pdf": SPEC_SHA256,
            **{
                f"SDitH/{PACKAGE}/<tree>/<set>/src/{source} ({len(trees) * len(SETS)} copies)": digest
                for source, (trees, digest) in SOURCE_SHA256.items()
            },
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
        "pinned_sources_identical_in_every_parameter_set_directory": all(
            (HERE / PACKAGE / path).is_file() and sha256(HERE / PACKAGE / path) == digest
            for path, digest in files.items()
        ),
    }
    if not checks["pinned_sources_identical_in_every_parameter_set_directory"]:
        result.update({"checks": checks, "status": "FAIL"})
        return emit(result, args.output)

    # 1. Specification text.
    pages = spec_pages(tool) if checks["pinned_specification"] else []
    found, extract_uses, extract_pages, getbytes_pages, tables, printed = check_spec(pages)
    one_index = [use for use in extract_uses if "→" not in use]
    checks.update({
        "spec_cipher_variant_line_1_takes_the_counter_and_line_2_reads_from_it":
            found["cipher_variant_lines_1_to_4"],
        "spec_remark_says_lines_1_to_4_do_not_depend_on_the_counter": found["remark"],
        "spec_cipher_variant_tests_w_pow_minus_1_bits_of_c0_xor_c1":
            found["cipher_variant_lines_8_to_10"] and found["cipher_variant_lines_11_to_13"]
            and found["cipher_variant_lines_5_to_7"],
        "spec_quotes_are_on_physical_pages_21_22_printed_19_20":
            printed.get(21) == 19 and printed.get(22) == 20 and printed == PRINTED_PAGE,
        "spec_getbytes_used_once_and_only_getbyte_defined":
            getbytes_pages == [21] and found["xof_api"],
        "spec_values_of_line_2_are_lambda_bits": found["enc_sizes_are_lambda_bits"],
        "spec_extract_defined_with_two_indices_and_used_once_with_one":
            found["extract_definition"] and one_index == ["wpow-1"]
            and len(extract_uses) == 5 and extract_pages == [21, 22, 26],
        "spec_other_quoted_statements_found": all(found.values()),
    })

    # 2. Code facts.
    reference = HERE / PACKAGE / TREES[0] / SETS[0] / "src"
    prng = (reference / "sdith_prng.c").read_text()
    prng_avx2 = (HERE / PACKAGE / TREES[1] / SETS[0] / "src/sdith_prng_avx2.c").read_text()
    signature = (reference / "sdith_signature.c").read_text()
    dispatch = (reference / "vole_parameters.c").read_text()
    init = {c: cipher_init_facts(prng, c, "_ref") for c in CATEGORIES}
    init_avx = {c: cipher_init_facts(prng_avx2, c, "_avx") for c in CATEGORIES}
    grind = {c: cipher_grind_facts(prng, c) for c in CATEGORIES}
    hash_grind = {c: hash_grind_facts(prng, c) for c in CATEGORIES}
    drawn = {c: init[c]["bytes_drawn_for_p0_p1_k0_k1"] for c in CATEGORIES}

    def all_true(table, *keys):
        return all(table[c][key] is True for c in CATEGORIES for key in keys)

    checks.update({
        "code_cipher_init_derives_p_and_k_from_prefix_and_h_piop_without_counter":
            all_true(init, "xof_absorbs_prefix_byte_then_h_piop_only", "no_counter_in_function")
            and all_true(init_avx, "xof_absorbs_prefix_byte_then_h_piop_only",
                         "no_counter_in_function"),
        "code_cipher_key_schedule_runs_once_in_init":
            all_true(init, "key_schedule_here") and all_true(init_avx, "key_schedule_here")
            and all_true(grind, "no_key_schedule_and_no_first_xof_per_counter"),
        "code_cipher_grind_is_two_cipher_calls_per_counter_and_w_minus_1_bits":
            all_true(grind, "two_cipher_calls_per_counter", "tests_masked_bits_of_c0_xor_c1")
            and all_true(init, "mask_covers_w_minus_1_bits")
            and all_true(init_avx, "mask_covers_w_minus_1_bits"),
        "code_second_xof_uses_another_prefix_with_h_piop_counter_and_c":
            all_true(grind, "second_xof_only_after_the_test_passes_with_prefix_h_piop_ctr_c")
            and "const uint8_t PROOFOW_H0_PREFIX = 5;" in signature
            and "const uint8_t PROOFOW_H1_PREFIX = 6;" in signature,
        "code_init_function_is_at_sdith_prng_c_lines_323_to_347":
            tuple(init[1]["lines"]) == REVIEW_INIT_LINES,
        "code_dispatches_the_checked_functions": all(
            f"vole_params->proofow_init = proofow_init_cipher_cat{c}_ref;" in dispatch
            and f"vole_params->proofow_grind_w = proofow_grind_w_cipher_cat{c}_ref;" in dispatch
            and f"vole_params->proofow_init = proofow_init_shake_cat{c};" in dispatch
            and f"vole_params->proofow_grind_w = proofow_grind_w_shake_cat{c};" in dispatch
            for c in CATEGORIES
        ),
        # Line 2: 4*lambda bits at Categories 1 and 5 (64 and 128 bytes), not
        # 4*lambda bytes.  Category 3 draws 128 bytes, four 256-bit words.
        "code_draws_4_lambda_bits_not_bytes_at_categories_1_and_5":
            drawn[1] * 8 == 4 * 128 and drawn[5] * 8 == 4 * 256
            and all(init_avx[c]["bytes_drawn_for_p0_p1_k0_k1"] == drawn[c] for c in CATEGORIES),
    })

    # 3. Accounting.
    parameters = parse_parameters((reference / "sdith_signature_parameters.c").read_text())
    rows = []
    for name in SETS:
        p = parameters.get(name)
        if p is None:
            continue
        rows.append({"parameter_set": name, **{k: p[k] for k in (
            "lambda", "kappa", "tau", "w_pow", "d")}, "grinding": p["variant"].lower(),
            **accounting(p)})
    cipher_rows = [row for row in rows if row["grinding"] == "cipher"]
    hash_rows = [row for row in rows if row["grinding"] == "shake"]
    checks.update({
        "parameters_read_for_twelve_sets_six_of_each_grinding":
            sorted(parameters) == sorted(SETS) and len(cipher_rows) == 6 and len(hash_rows) == 6
            and all(name.endswith("_cipherpow") == (parameters[name]["variant"] == "CIPHER")
                    for name in SETS)
            and all(p["lambda"] == CATEGORIES[int(name[3])][0] for name, p in parameters.items()),
        "parameters_equal_spec_tables_4_and_5": bool(rows) and tables == {
            name: {"kappa": p["kappa"], "tau": p["tau"], "w_pow": p["w_pow"]}
            for name, p in parameters.items()},
        "d_is_4_the_number_of_mux_arities":
            bool(rows) and all(p["d"] == 4 == p["mux_arities"] for p in parameters.values()),
        "tau_kappa_plus_w_pow_minus_log2_d_equals_lambda_for_twelve_sets":
            len(rows) == 12 and all(
                row["design_bits_tau_kappa_plus_w_minus_log2_d"] == row["lambda"] for row in rows),
        "cipher_sets_one_counter_value_succeeds_with_2^-(lambda-1)":
            len(cipher_rows) == 6 and all(
                row["grinding_bits_tested"] == row["w_pow"] - 1
                and row["log2_counter_values_needed"] == row["lambda"] - 1 for row in cipher_rows),
        "cipher_sets_literal_line_1_costs_2^(lambda-1)_xof_calls":
            len(cipher_rows) == 6 and all(
                row["literal_line_1"]["log2_xof_calls"] == row["lambda"] - 1
                for row in cipher_rows),
        "cipher_sets_code_reading_costs_2^lambda_cipher_calls":
            len(cipher_rows) == 6 and all(
                row["code"]["log2_cipher_calls"] == row["lambda"]
                and row["code"]["log2_xof_calls"] == row["lambda"] - row["w_pow"]
                for row in cipher_rows),
        "cipher_sets_both_readings_need_2^lambda_cipher_calls":
            len(cipher_rows) == 6 and all(
                row["code"]["log2_cipher_calls"] == row["lambda"]
                and row["literal_line_1"]["log2_cipher_calls"] == row["lambda"]
                for row in cipher_rows),
        "cipher_sets_literal_reading_adds_xof_and_key_schedule_work":
            len(cipher_rows) == 6 and all(
                row["literal_line_1"]["log2_xof_calls"] > row["code"]["log2_xof_calls"]
                and row["literal_line_1"]["log2_key_schedules"] == row["lambda"]
                for row in cipher_rows),
        # Control: the hash-grinding sets.
        "control_hash_variant_code_follows_its_pseudocode":
            found["hash_variant"]
            and all_true(hash_grind, "one_xof_over_h_piop_and_counter_per_counter_no_prefix",
                         "tests_w_bits"),
        "control_hash_sets_both_readings_cost_2^lambda_xof_calls":
            len(hash_rows) == 6 and all(
                row["literal_line_1"] == row["code"]
                and row["code"]["log2_xof_calls"] == row["lambda"]
                and row["grinding_bits_tested"] == row["w_pow"] for row in hash_rows),
    })

    result.update({
        "checks": checks,
        "controls": sorted(name for name in checks if name.startswith("control_")),
        "measurements": {
            "parameter_sets": rows,
            "cipher_sets_log2_work": {
                row["parameter_set"]: {
                    "lambda": row["lambda"],
                    "literal_line_1_xof_calls": row["literal_line_1"]["log2_xof_calls"],
                    "code_cipher_calls": row["code"]["log2_cipher_calls"],
                    "code_xof_calls": row["code"]["log2_xof_calls"],
                }
                for row in cipher_rows
            },
            "hash_sets_log2_xof_calls_either_reading": {
                row["parameter_set"]: row["code"]["log2_xof_calls"] for row in hash_rows},
            "code": {
                "cipher_init_ref": {f"cat{c}": init[c] for c in CATEGORIES},
                "cipher_init_avx2": {f"cat{c}": init_avx[c] for c in CATEGORIES},
                "cipher_grind_ref": {f"cat{c}": grind[c] for c in CATEGORIES},
                "hash_grind": {f"cat{c}": hash_grind[c] for c in CATEGORIES},
                "xof_prefix_bytes": {"first_xof": 5, "second_xof": 6},
            },
            "line_2_bytes": {
                f"cat{c}": {
                    "lambda": CATEGORIES[c][0],
                    "drawn_by_the_code": drawn[c],
                    "4_lambda_bits_in_bytes": 4 * CATEGORIES[c][0] // 8,
                    "4_lambda_bytes": 4 * CATEGORIES[c][0],
                    "code_equals_4_lambda_bits": drawn[c] * 8 == 4 * CATEGORIES[c][0],
                }
                for c in CATEGORIES
            },
        },
        "spec": {
            "extracted_with": tool_version(tool) + ", option -layout",
            "pages_are": "physical PDF pages (printed page = physical - 2)",
            "printed_page_of_physical_page": {str(k): v for k, v in sorted(printed.items())},
            "quotes": {
                name: {"page": page, "text": quote, "found": found[name]}
                for name, (page, quote) in SPEC_QUOTES.items()
            },
            "extract_subscripts_used_on_page_21": extract_uses,
            "pages_naming_extract": extract_pages,
            "pages_naming_getbytes": getbytes_pages,
            "tables_4_and_5": tables,
        },
        "not_covered": [
            "No attack: the literal reading costs more per counter value than the "
            "code's, and no signature is forged; the bit is in what a proof may count "
            "as one unit of work.",
            "The specification has no EUF-CMA proof (physical p.46), so no stated "
            "bound is evaluated.",
            "Costs of one XOF call, key schedule or cipher call in gates are not "
            "compared.",
            "The low-level AVX2 grind loops are not read; only the init functions of "
            "sdith_prng_avx2.c are checked.",
            "The Category 3 code draws four 256-bit words where the pseudocode "
            "implies 4*192 bits; this further difference is recorded, not analysed.",
            "No submission code is built or run, and no test vector is compared.",
        ],
        "status": "PASS" if all(checks.values()) else "FAIL",
    })
    return emit(result, args.output)


if __name__ == "__main__":
    raise SystemExit(main())
