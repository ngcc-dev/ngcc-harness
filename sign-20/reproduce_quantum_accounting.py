#!/usr/bin/env python3
"""Reproduce the Qingluan-128 quantum-accounting inconsistency."""

from __future__ import annotations

import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SPEC = ROOT / "sign-20-spec.pdf"


def pdf_text() -> str:
    try:
        output = subprocess.run(
            ["pdftotext", "-layout", str(SPEC), "-"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout
    except FileNotFoundError as exc:
        raise SystemExit("pdftotext is required") from exc
    return " ".join(output.split())


def main() -> int:
    text = pdf_text()

    # These are normative security-accounting statements in §§3.2.3–3.2.5.
    required = (
        "quantum = classical/2",
        "Qingluan-128: quantum binding 64",
        "the 80-bit floor is met as NIST category 1",
        "not as a literal",
        "128.8 forgery attempts",
    )
    for statement in required:
        assert statement in text, f"missing specification statement: {statement!r}"

    # The table records 2^143 classical key recovery. PDF extraction may place
    # whitespace between the base and exponent, so check its row structurally.
    assert re.search(r"Key recovery \(ISD w/.*?2\s*143", text), (
        "the Qingluan-128 2^143 key-recovery entry was not found"
    )

    target = 80.0
    forgery_classical = 128.8
    key_recovery_classical = 143.0
    forgery_quantum = forgery_classical / 2
    key_recovery_quantum = key_recovery_classical / 2

    assert forgery_quantum == 64.4
    assert key_recovery_quantum == 71.5
    assert min(forgery_quantum, key_recovery_quantum) < target

    print(f"Qingluan-128 target: {target:.1f} quantum bits")
    print(f"forgery exponent under specified rule: {forgery_quantum:.1f} bits")
    print(f"key-recovery exponent under specified rule: {key_recovery_quantum:.1f} bits")
    print("PROOF GAP sign-20-1 Qingluan-128 QUANTUM ACCOUNTING: CONFIRMED")
    print("LIMITATION: halved cost exponents are not end-to-end quantum gate costs.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
