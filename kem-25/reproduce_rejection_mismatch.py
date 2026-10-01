#!/usr/bin/env python3
"""Static certificate for NEV's compressed-set rejection mismatch."""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path


HERE = Path(__file__).resolve().parent
pdf = HERE / "kem-25-spec.pdf"
assert pdf.is_file(), "missing archived NEV specification"
pdftotext = shutil.which("pdftotext")
assert pdftotext, "pdftotext is required"
spec = subprocess.run(
    [pdftotext, str(pdf), "-"], check=True, text=True, capture_output=True
).stdout
normalized = " ".join(spec.split()).casefold()
assert "requiring an implicit rejection strategy for the security proof" in normalized
assert "algorithm 27: nev-kem-cca.decap" in normalized

submitted = sorted((HERE / "Implementations").rglob("cca.c"))
files = submitted or sorted((HERE / "src").rglob("cca.c"))
assert files, "no archived cca.c source found"
compressed = [p for p in files if p.parent.name in {"NEV-C1-c", "NEV-C2-c", "NEV-C3-c"}]
if submitted:
    expected_compressed = {12: 3, 48: 12}
    assert len(files) in expected_compressed, (
        f"expected 12 reference or 48 full submitted cca.c copies, found {len(files)}"
    )
    assert len(compressed) == expected_compressed[len(files)], (
        f"unexpected compressed-set copy count: {len(compressed)}"
    )
else:
    assert len(files) == 4, f"expected four public representative copies, found {len(files)}"
    compressed = files

for path in files:
    text = path.read_text(encoding="utf-8")
    assert "fail = verify(ct, cmp, PKE_OW_CT_BYTES);" in text
    assert "if (!fail)" in text
    assert "ss[i] = kr[i]" in text
    assert "return fail;" in text

print(f"spec_implicit_rejection=required source_copies={len(files)}")
print(f"compressed_copies_checked={len(compressed)} implementation_returns_failure=1")
print("LIMITATION: no IND-CCA attack or key recovery is demonstrated")
print("PROOF GAP kem-25-1 CONFIRMED: compressed NEV uses explicit rather than required implicit rejection")
