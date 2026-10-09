#!/usr/bin/env python3
"""Build and run the CHIME-1024 controlled-capacity certificate."""

import subprocess
import tempfile
from pathlib import Path


root = Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix="ngcc-chime-capacity-") as directory:
    binary = Path(directory) / "capacity-match"
    subprocess.run(
        ["cc", "-O2", "-std=gnu11", "-I", str(root),
         str(root / "reproduce_capacity_match.c"), "-o", str(binary)],
        check=True,
        capture_output=True,
    )
    subprocess.run([str(binary)], check=True)
