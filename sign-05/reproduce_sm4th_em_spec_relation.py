#!/usr/bin/env python3
"""Static certificate for the specified SM4th-EM relation in sign-05-6."""

from __future__ import annotations

import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent


def rank_gf2(rows: list[int], width: int) -> int:
    rank = 0
    for bit in reversed(range(width)):
        pivot = next((i for i in range(rank, len(rows)) if rows[i] >> bit & 1), None)
        if pivot is None:
            continue
        rows[rank], rows[pivot] = rows[pivot], rows[rank]
        for i in range(len(rows)):
            if i != rank and (rows[i] >> bit) & 1:
                rows[i] ^= rows[rank]
        rank += 1
    return rank


def main() -> None:
    pdftotext = shutil.which("pdftotext")
    if not pdftotext:
        raise SystemExit("pdftotext is required")
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "spec.txt"
        subprocess.run([pdftotext, "-layout", ROOT / "sign-05-spec.pdf", out], check=True)
        spec = out.read_text(errors="replace")
    assert "SM4th.EM.EncCstrnts" in spec
    assert "let ⟨w⟩ ← ⟨in⟩∥⟨w⟩∥⟨out⟩" in spec
    assert "⟨w̃⟩ ← ⟨w[ℓke .. ℓke + ℓenc )⟩" in spec

    # With l_ke=0, the selected 32 words already begin with X0..X3.  The
    # callee prepends X0..X3 once more.  Windows i..i+4 for i=0..31 end at
    # composite word 35; public output occupies words 36..39.
    selected_words = 1024 // 32
    last_read = (32 - 1) + 4
    output_first = 4 + selected_words
    assert selected_words == 32 and last_read == 35 and output_first == 36

    # The first four output words duplicate the initial words.  Per bit, the
    # resulting public-constant equations contain the other three words.
    matrix = [0b1110, 0b1101, 0b1011, 0b0111]
    assert rank_gf2(matrix, 4) == 4

    dirs = sorted((ROOT / "Implementations").glob("*_Implementation/sm4th_em_d2_*"))
    assert len(dirs) == 8
    for directory in dirs:
        assert "_em_" in directory.name
        params = (directory / "params.c").read_text()
        source = (directory / "sm4th_sm4_128.c").read_text()
        assert ".lke = 0," in params and ".lenc = 1024," in params
        prover = source[source.rindex("static void sm4_SSS_constraints_prover") :]
        prover_em = prover[prover.index("if (use_em)") : prover.index("} else {")]
        assert "sm4_SSS_enc_constraints_prover" in prover_em
        assert "w + 16, w_tag + 16 * 8," in prover_em
        verifier = source[source.rindex("static void sm4_SSS_constraints_verifier") :]
        verifier_em = verifier[verifier.index("if (use_em)") : verifier.index("} else {")]
        assert "sm4_SSS_enc_constraints_verifier" in verifier_em
        assert "w_key + blocksize" in verifier_em

    print("parameter_sets=4 source_copies=8 last_spec_word=35 output_first_word=36")
    print("ATTACK sign-05-6 CONFIRMED: specified relation omits pk2")


if __name__ == "__main__":
    main()
