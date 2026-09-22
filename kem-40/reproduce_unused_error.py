#!/usr/bin/env python3
# Finding: kem-40-1
"""Static witness for the unused YuanYang.KEM encryption error."""
from pathlib import Path
import re

def main():
    base = Path(__file__).resolve().parent / "Implementations" / "Reference_Implementation"; checked = 0
    for source in sorted(base.glob("yuanyang-*/kem.c")):
        text = source.read_text(); match = re.search(r"static int yy_encrypt\(.*?\n}\n", text, re.S)
        if not match or "ring_samp(s,e,&rng);" not in match.group(0): return 2
        remainder = match.group(0).split("ring_samp(s,e,&rng);", 1)[1]
        uses = re.findall(r"(?<![A-Za-z0-9_])e\s*\[", remainder)
        print(f"{source.parent.name}: subsequent e[i] uses={len(uses)}")
        if uses: return 1
        checked += 1
    ok = checked == 3
    print("CONFIRMED: every yy_encrypt discards e" if ok else f"expected 3 copies, found {checked}")
    return 0 if ok else 1

if __name__ == "__main__": raise SystemExit(main())
