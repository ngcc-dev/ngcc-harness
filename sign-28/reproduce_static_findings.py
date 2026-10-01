#!/usr/bin/env python3
"""Static certificates for sign-28-3 and sign-28-4."""

from pathlib import Path
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
SPEC = HERE / "sign-28-spec.pdf"
SETS = ("160s", "160f", "256s", "256f", "512s", "512f")


def need(text: str, pattern: str, where: str) -> None:
    if re.search(pattern, text, re.S) is None:
        raise AssertionError(f"missing {pattern!r} in {where}")


with tempfile.TemporaryDirectory(prefix="ngcc-sydo-static-") as td:
    txt = Path(td) / "spec.txt"
    subprocess.run(["pdftotext", "-layout", str(SPEC), str(txt)], check=True)
    spec = txt.read_text(errors="replace")

need(spec, r"iv\s*←\s*Hash4\s*\(ivpre\s*\)", "Algorithms 2/3")
need(spec, r"extractor Ext which is allowed to\s+program Hash4", "Lemma 19")
need(spec, r"if nodes is not all zeros then\s*22\s*:\s*return", "Algorithm 13")

for name in SETS:
    ref = HERE / "Implementations" / "Reference_Implementation" / f"sydo_{name}" / "src"
    sydo = (ref / "sydo.c").read_text()
    bavc = (ref / "bavc_impl.inc").read_text()
    uhash = (ref / "universal_hashing_impl.inc").read_text()
    need(sydo, r"const uint8_t\* iv = seed_iv \+ lambda_bytes", f"reference {name}")
    secpar_bits = int(name[:3])
    need(sydo, rf'"sydo_{name}", {secpar_bits},', f"reference {name} parameter table")
    assert secpar_bits // 8 > 8
    need(sydo, r"memcpy\(sig \+ layout\.iv_offset, iv, layout\.iv_size\)", f"reference {name}")
    need(sydo, r"sydo_ref_vole_reconstruct\(params, sig \+ layout\.iv_offset", f"reference {name}")
    if "0x04" in sydo:
        raise AssertionError(f"unexpected domain-four hash in reference {name} sydo.c")
    need(bavc, r"trace_stage\(params, \"verify leaf_hash\".*?ok = true", f"reference {name}")
    need(uhash, r"uint8_t tmp\[8\].*?left = lambda_bytes > byte_off.*?store64_partial_le\(tmp, product\[i\], left\).*?j != left.*?tmp\[j\]", f"reference {name}")

    opt = HERE / "Implementations" / "Optimized_Implementation" / f"sydo_{name}" / "src"
    sign = (opt / "sign.cpp").read_text()
    verify = (opt / "verify.cpp").read_text()
    vec = (opt / "vector_com.inc").read_text()
    need(sign, r"finalize\(seed_iv\.data\(\), sizeof\(seed_iv\)\).*?memcpy\(&iv, &seed_iv\[sizeof\(seed\)\]", f"optimized {name}")
    need(verify, r"memcpy\(&iv, iv_ptr, sizeof\(iv\)\).*?vole_reconstruct", f"optimized {name}")
    need(vec, r"while \(opening_pos < OPEN_SIZE\).*?opening\[opening_pos\+\+\] != 0.*?return false", f"optimized {name}")

print("ATTACK sign-28-3 SYDO CONFIRMED implementation omits the specified Hash4 derivation")
print("ATTACK sign-28-4 SYDO CONFIRMED reference universal hash reads beyond tmp[8]")
