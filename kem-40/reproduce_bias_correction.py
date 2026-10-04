#!/usr/bin/env python3
"""Static certificate for the YuanYang even-compression correction defects."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parent
FILES = sorted((ROOT / "Implementations").glob("**/kem.c"))
assert len(FILES) == 6, f"expected six kem.c copies, found {len(FILES)}"

affected = []
control = []
for path in FILES:
    text = path.read_text(encoding="utf-8", errors="replace")
    assert "sc/2+1-sc%2" in text
    assert re.search(
        r"expanded->tmp\[i\]\s*=\s*i<YUANYANG_D/4\+"
        r"\(1-SCALETAB\[YUANYANG_LOGD-9\]%2\)\*2\*\(i>=YUANYANG_D/2\)",
        text,
    )
    name = path.parent.name
    if name.endswith(("1024", "2048")):
        affected.append(path)
    else:
        control.append(path)

assert len(affected) == 4 and len(control) == 2

# For even sc, the written compression offset is sc/2+1.  In the expansion
# expression the extra two positions are guarded by i>=d/2 but compared with
# d/4+2, so no such i can satisfy the comparison for d>=8.
for d in (1024, 2048):
    assert not any(i < d // 4 + 2 * (i >= d // 2) for i in range(d // 2, d))

print("CONFIRMED kem-40-4: all 1024/2048 reference and optimized trees retain the two even-rounding defects; 512 is the odd-k control")
