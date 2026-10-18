#!/usr/bin/env python3
"""Reproduce the SDitH Algorithm 10/12 Hashlines mismatch."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = ROOT / "SDitH/SDitH-spec.pdf"
SOURCE = ROOT / (
    "SDitH/submission/submission_package/Reference_Implementation/"
    "cat1_fast/src/sdith_signature.c"
)
TREE = SOURCE.parents[1]
SIGNER = (
    "par.vole_params.xof.xof_seed(&hash_lines_ctx, sign.circuit_in_pub, "
    "(par.num_inputs_pairs + 7) >> 3);"
)
VERIFIER = (
    "par.vole_params.xof.xof_seed(&verif_hash_lines_ctx, sign.circuit_in_pub, "
    "(par.num_inputs_pairs + 7) >> 3);"
)


def checked(command: list[str], cwd: Path | None = None) -> subprocess.CompletedProcess:
    result = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(
            f"command failed ({result.returncode}): {' '.join(command)}\n"
            f"{result.stdout}{result.stderr}"
        )
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    source = SOURCE.read_text(encoding="utf-8")
    if source.count(SIGNER) != 1 or source.count(VERIFIER) != 1:
        raise RuntimeError("expected exactly one signer and verifier witness-correction hash call")
    copies = sorted(
        (ROOT / "SDitH/submission/submission_package").glob(
            "*_Implementation/*/src/sdith_signature.c"
        )
    )
    copy_hashes = {hashlib.sha256(path.read_bytes()).hexdigest() for path in copies}
    if len(copies) != 24 or len(copy_hashes) != 1:
        raise RuntimeError(
            f"expected 24 byte-identical implementation copies, found "
            f"{len(copies)} files and {len(copy_hashes)} hashes"
        )

    pdf_text = checked(["pdftotext", "-layout", str(SPEC), "-"]).stdout
    sign_start = pdf_text.index("Algorithm 10 BLC.Commit")
    verify_start = pdf_text.index("Algorithm 12 BLC.RecomputeEvaluation")
    signing_algorithm = pdf_text[sign_start:verify_start]
    verification_algorithm = pdf_text[verify_start:verify_start + 8000]
    normalized_sign = " ".join(signing_algorithm.split())
    normalized_verify = " ".join(verification_algorithm.split())
    if "Hashlines (haux , α′plain , α′base , ∆wit)" not in normalized_sign:
        raise RuntimeError("Algorithm 10 witness-correction Hashlines input not found")
    if "Hashlines (haux , α′plain , α′base )" not in normalized_verify:
        raise RuntimeError("Algorithm 12 three-input Hashlines call not found")
    if "Hashlines (haux , α′plain , α′base , ∆wit)" in normalized_verify:
        raise RuntimeError("Algorithm 12 unexpectedly includes witness correction")

    harness = ROOT / "bin/nist-sig"
    library = ROOT / "SDitH/lib/cat1_fast.so"
    if not harness.is_file():
        checked(["make", "-C", str(ROOT / "api"), "harness"])
    if not library.is_file():
        checked(["make", "-C", str(ROOT / "SDitH"), "lib/cat1_fast.so"])
    control = checked([
        str(harness), "kat", str(library),
        str(ROOT / "SDitH/kat/cat1_fast.tsv"), "1",
    ])
    if "PASS" not in control.stdout:
        raise RuntimeError("unmodified library control did not report PASS")

    with tempfile.TemporaryDirectory(prefix="sdith-2-") as temporary:
        work = Path(temporary)
        copied = work / "cat1_fast"
        build = work / "build"
        shutil.copytree(TREE, copied)
        copied_source = copied / "src/sdith_signature.c"
        modified = copied_source.read_text(encoding="utf-8").replace(
            VERIFIER,
            "/* Algorithm 12 literal behavior: Delta_wit is omitted here. */",
        )
        if modified.count(VERIFIER) != 0 or modified.count(SIGNER) != 1:
            raise RuntimeError("temporary edit did not isolate the verifier input")
        copied_source.write_text(modified, encoding="utf-8")
        checked([
            "cmake", "-S", str(copied), "-B", str(build),
            "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_KATS=ON",
        ])
        checked(["cmake", "--build", str(build), "-j", "2"])
        literal = subprocess.run(
            [str(build / "generator/PQCgenKAT_cat1_fast")],
            cwd=build / "generator", capture_output=True, text=True,
        )
        combined = literal.stdout + literal.stderr
        if literal.returncode == 0 or "crypto_sign_open returned <1>" not in combined:
            raise RuntimeError(
                "literal Algorithm 12 control did not reject the honest signature\n"
                + combined
            )

    result = {
        "format": "SDitH-2-specification-mismatch-v1",
        "status": "PASS",
        "specification_sha256": hashlib.sha256(SPEC.read_bytes()).hexdigest(),
        "source": str(SOURCE.relative_to(ROOT)),
        "source_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
        "implementation_copies": {
            "count": len(copies),
            "common_sha256": next(iter(copy_hashes)),
        },
        "specification": {
            "algorithm_10_hashes_witness_correction": True,
            "algorithm_12_hashes_witness_correction": False,
        },
        "submitted_source": {
            "signer_hashes_circuit_in_pub": True,
            "verifier_hashes_circuit_in_pub": True,
            "unmodified_first_kat": "PASS",
        },
        "literal_algorithm_12_control": {
            "parameter": "cat1_fast",
            "returncode": literal.returncode,
            "observed": "crypto_sign_open returned <1>",
        },
        "limitations": [
            "The runtime control exercises cat1_fast; the pseudocode defect is parameter-independent.",
            "The submitted implementation is internally consistent and was not modified.",
            "The failure rejects honest input and does not produce a forgery.",
        ],
    }
    rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    print("PASS SDitH-2: literal Algorithm 12 rejects the first honest KAT signature")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
