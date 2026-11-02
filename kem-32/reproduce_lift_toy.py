#!/usr/bin/env python3
"""Replay Wang's scaled QCTM quotient-key lift at a pinned source revision.

The witness starts from a supplied quotient key, not a recovered full-size key.
"""

import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request


COMMIT = "90ac136c70e2c8aa41d68bc9129cbe8789f9dba5"
URL = f"https://raw.githubusercontent.com/yonggewang/ClaudeAI/{COMMIT}/NGCC/scripts/qctm_toy.py"
SHA256 = "4f95e8ee21b62e9306be0f0da3a38d2939f5b78bae79d95883a1c47bdd587658"
CASES = (("10", "11", "3", "48", "1"), ("10", "11", "3", "48", "2", "1"))


def main():
    try:
        source = urllib.request.urlopen(URL, timeout=30).read()
    except urllib.error.URLError as exc:
        print(f"SKIP: pinned QCTM toy unavailable: {exc}", file=sys.stderr)
        raise SystemExit(77) from exc
    assert hashlib.sha256(source).hexdigest() == SHA256
    with tempfile.TemporaryDirectory(prefix="ngcc-qctm-lift-") as directory:
        path = Path(directory) / "qctm_toy.py"
        path.write_bytes(source)
        for case in CASES:
            result = subprocess.run(
                [sys.executable, "-I", str(path), *case],
                check=True, capture_output=True, text=True, timeout=120,
            )
            output = result.stdout
            assert "lifted key: A_(t-1)(U_att, y_att) ∩ E == public code: True" in output
            assert "decoding with the lifted key:" in output
            assert "recovered e == e: True" in output
            assert "(D) lifting + decoding succeeded: True" in output
            print(f"QCTM_TOY_LIFT=PASS seed={case[4]} extra_frobenius={len(case) > 5}")


if __name__ == "__main__":
    main()
