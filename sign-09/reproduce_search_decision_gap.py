#!/usr/bin/env python3
"""Check the two incompatible DOVE proof premises in the frozen specification."""

from pathlib import Path
import subprocess
import sys


spec = Path(__file__).resolve().with_name("sign-09-spec.pdf")
try:
    result = subprocess.run(
        ["pdftotext", "-layout", str(spec), "-"],
        capture_output=True,
        text=True,
        check=True,
    )
except FileNotFoundError:
    print("SKIP sign-09-2: pdftotext is unavailable")
    sys.exit(77)
except subprocess.CalledProcessError as exc:
    print(f"NOT REPRODUCED sign-09-2: pdftotext failed: {exc}", file=sys.stderr)
    sys.exit(1)

text = " ".join(result.stdout.split())
checks = {
    "search definition": "Definition 2. (DOVE-UOV Problem)" in text
    and "find a matrix" in text.split("Definition 2. (DOVE-UOV Problem)", 1)[1].split("4.4 Security Proof", 1)[0],
    "decision game": "Game 1 (Trapdoor Indistinguishability)" in text
    and "random symmetric matrices" in text.split("Game 1 (Trapdoor Indistinguishability)", 1)[1].split("Game 2", 1)[0],
    "asserted equivalence": "This is precisely the definition of the DOVE-UOV problem" in text,
}
for label, passed in checks.items():
    if not passed:
        print(f"NOT REPRODUCED sign-09-2: missing {label}", file=sys.stderr)
        sys.exit(1)
print("PROOF GAP sign-09-2 CONFIRMED: search assumption used for a decision game")
