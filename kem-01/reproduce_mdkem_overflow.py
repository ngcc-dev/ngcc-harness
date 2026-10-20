#!/usr/bin/env python3
"""Build the submitted Aigis-Enc+-I deterministic helper with ASan and controls."""

import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
src = here / "Implementations/Reference_Implementation/Aigis-Enc+-I"
sources = "kem owcpa polyvec poly pack ntt reduce cbd gen_a fips202 hashkdf verify precomp".split()
with tempfile.TemporaryDirectory(prefix="ngcc-aigis-mdkem-") as dirname:
    target = Path(dirname) / "witness"
    cmd = ["cc", "-std=c11", "-O1", "-g", "-DPARAMS=1", "-DUSE_SHA3", "-I", str(src),
           str(here / "reproduce_mdkem_overflow.c"), *(str(src / (s + ".c")) for s in sources),
           "-o", str(target)]
    subprocess.run(cmd + ["-fsanitize=address", "-fno-sanitize-recover=address"], check=True,
                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
    asan = subprocess.run([str(target)], env=env, capture_output=True, text=True)
    assert asan.returncode != 0, "ASan run did not fail"
    assert "stack-buffer-overflow" in asan.stderr, asan.stderr[-2000:]
    assert "mdkem_enc" in asan.stderr or "hash256" in asan.stderr, asan.stderr[-2000:]
    subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    plain = subprocess.run([str(target)], capture_output=True, text=True, check=True)
    assert "random_match=1" in plain.stdout, plain.stdout
    print("CONFIRMED kem-01-4: ASan stack write in mdkem_enc; " + plain.stdout.strip())
