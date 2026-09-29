#!/usr/bin/env python3
"""Compile and run the AFS-TrEDM final-block replay certificate."""

from __future__ import annotations

import os
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "reproduce_final_block_replay.c"
SETS = ("AFS-TrEDM-512", "AFS-TrEDM-768", "AFS-TrEDM-1024")


def main() -> int:
    cc = os.environ.get("CC", "cc")
    with tempfile.TemporaryDirectory(prefix="afs-tredm-replay-") as tmp:
        for name in SETS:
            src = ROOT / "src" / name
            exe = Path(tmp) / name
            command = [
                cc,
                "-O2",
                "-std=c99",
                f"-I{src}",
                str(SOURCE),
                str(src / "afs_p1600.c"),
                str(src / "afs_sbox64.c"),
                str(src / "afs_lmds1600_s6.c"),
                "-o",
                str(exe),
            ]
            subprocess.run(command, check=True)
            subprocess.run([str(exe)], check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
