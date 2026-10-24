#!/usr/bin/env python3
"""Check DOVE Theorem 1's MQ-target and signing-simulation premises."""

from pathlib import Path
import shutil
import subprocess
import sys


base = Path(__file__).resolve().parent
if shutil.which("pdftotext") is None:
    print("SKIP sign-09-4: pdftotext is unavailable")
    sys.exit(77)

spec = " ".join(
    subprocess.check_output(["pdftotext", "-layout", str(base / "sign-09-spec.pdf"), "-"], text=True).split()
)
game = spec.split("Game 2 (Random Oracle Programming and Direct Forgery)", 1)
if len(game) != 2:
    raise SystemExit("NOT REPRODUCED sign-09-4: Game 2 missing")
game = game[1].split("5 Implementations and Performance", 1)[0]
proof_checks = {
    "back-patching": "Program (back-patch) the random oracle" in game,
    "pre-query abort": "has already been defined by a previous query" in game and "simulator aborts" in game,
    "uniform-signature assertion": "Uniformly sample a random vector" in game
    and "statistically indistinguishable" in game,
    "unembedded target": "target vector t∗ = H(msg∗ ∥ seedpk )" in game
    and "embedded in the public key" in game,
    "MQ challenge target": "Definition 1. (DOVE-MQ Problem)" in spec
    and "a target vector t" in spec,
    "specified target input": "SM3(msg ∥ seedpk" in spec,
}
for label, ok in proof_checks.items():
    if not ok:
        raise SystemExit(f"NOT REPRODUCED sign-09-4: missing {label}")

files = sorted(base.glob("Implementations/**/DOVE_*_ref/SIG_AlgorithmInstance.c"))
if len(files) != 4:
    raise SystemExit(f"NOT REPRODUCED sign-09-4: expected four submitted source copies, found {len(files)}")
for source in files:
    code = source.read_text()
    checks = (
        "if (matrix_solve(z, L_mat, t_minus_w) == 0)" in code,
    )
    if not all(checks):
        raise SystemExit(f"NOT REPRODUCED sign-09-4: source path changed in {source}")

    # Both Sign and Verify construct the target from message || seed_pk.
    # Check those actual source blocks, including the absence of salt, rather
    # than modelling a pre-queried input with an artificial set membership.
    marker = "size_t input_len = m_len_bytes + DOVE_PK_SEED_BYTES;"
    blocks = [part.split("free(input);", 1)[0] for part in code.split(marker)[1:]]
    if len(blocks) != 2 or any(
        "free(input);" not in part for part in code.split(marker)[1:]
    ):
        raise SystemExit(f"NOT REPRODUCED sign-09-4: Sign/Verify target blocks changed in {source}")
    for block in blocks:
        if not all(fragment in block for fragment in (
            "memcpy(input, m, m_len_bytes)",
            "memcpy(input + m_len_bytes, seed_pk, DOVE_PK_SEED_BYTES)",
            "pseudoXOF(DOVE_M*8,input, input_len*8, t)",
        )) or "salt" in block:
            raise SystemExit(f"NOT REPRODUCED sign-09-4: target input changed in {source}")

print("PROOF GAP sign-09-4 CONFIRMED: MQ-target embedding and signature-simulation premises checked")
print("The unsalted Sign/Verify target and Game 2's prior-query abort also conflict; no forgery shown")
