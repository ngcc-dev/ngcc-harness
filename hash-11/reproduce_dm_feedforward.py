"""Build and run the native Garnet-1024a feed-forward witness in a temporary dir."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "Garnet/Implementations/API_CryptHash/Implementations/Reference_Implementation/Garnet"
code = (SOURCE / "Garnet_1024.c").read_text()
code = re.sub(r"/\*.*?\*/|//[^\n]*", "", code, flags=re.S)
absorb_step = re.compile(
    r"keep_capacity5_4x4_st\(state, cap\);\s*"
    r"absorb_message5_4x4_st\(state, msg\);\s*"
    r"for \(j = 0; j < 12; j\+\+\)\s*"
    r"permutation_p_4x4_st\(state, zero_key, j\);\s*"
    r"absorb_message5_4x4_st\(state, msg\);\s*"
    r"absorb_capacity5_4x4_st\(state, cap\);"
)
if len(absorb_step.findall(code)) != 2:
    raise SystemExit("hash-11-2: reference absorb call sites no longer match")

with tempfile.TemporaryDirectory(prefix="ngcc-garnet-dm-") as temporary:
    binary = Path(temporary) / "witness"
    subprocess.run(
        [os.environ.get("CC", "cc"), "-O2", "-w", "-I", str(SOURCE),
         str(ROOT / "reproduce_dm_feedforward.c"), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)
