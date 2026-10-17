#!/usr/bin/env python3
"""YuanYang.DSA: the Delta1 (ThetaDiv) denominator of the 512 set is reused by
the 1024 and 2048 sets (sign-34-6).  Static source certificate.

Specification (sign-34-spec.pdf, Section 4.2.2, Algorithms 12-13, physical
p. 19; Table 3, p. 26): r = exp(-2 pi^2 eta^2) and ThetaDiv multiplies by
1 + (2cos(2 pi c) - 2)/(r^-1 + r + 2), with eta = 1.026 / 0.974 / 0.928.
theta_div_q62() in every sign.c evaluates 1 + (cos - 1)*theta_den[0], so
theta_den[0] must equal 2/(r^-1 + r + 2) in Q62 for the set's own eta.

Control: the 512 trees match exactly, and the same fpr.h files encode the
RingSamp width inv_2sqreta for the correct per-set eta.
"""
import math
import re
import sys
from pathlib import Path

ID = "sign-34-6"
ROOT = Path(__file__).resolve().parent.parent
IMPL = ROOT / "sign-34" / "Implementations"
ETA = {512: 1.026, 1024: 0.974, 2048: 0.928}          # spec Table 3
EPS_LOG2 = {512: -29, 1024: -26, 2048: -23.5}          # spec Table 3 smoothness


def fail(msg):
    print(f"NOT CONFIRMED {ID}: {msg}")
    sys.exit(1)


trees = sorted(IMPL.rglob("yuanyang-*/fpr.h"))
if len(trees) != 6:
    fail(f"expected 6 fpr.h files (ref + opt x 3 sets), found {len(trees)}")

seen = {}
for f in trees:
    d = int(re.search(r"yuanyang-(\d+)", str(f)).group(1))
    t = f.read_text(errors="replace")
    mt = re.search(r"theta_den\[2\]\s*=\s*\{\s*YY_FPR_Q62_RAW\((0x[0-9a-fA-F]+)\)", t)
    me = re.search(r"fpr_yuanyang_inv_2sqreta\s*=\s*YY_FPR_RAW\((0x[0-9a-fA-F]+)\)", t)
    if not (mt and me):
        fail(f"{f.relative_to(ROOT)}: constants not found")
    line = t[:mt.start()].count("\n") + 1
    src = (f.parent / "sign.c").read_text(errors="replace")
    body = src.split("theta_div_q62(fpr center)", 1)[1].split("\n}", 1)[0]
    if "fpr_q62_mul(cm1, fpr_q62_yuanyang_theta_den[0])" not in body:
        fail(f"{f.parent.relative_to(ROOT)}/sign.c: ThetaDiv formula changed")
    den = int(mt.group(1), 16) / 2**62
    eta_ringsamp = 1 / math.sqrt(2 * int(me.group(1), 16) / 2**43)
    r = math.exp(-2 * math.pi**2 * ETA[d] ** 2)
    want = 2 / (1 / r + r + 2)
    eta_impl = math.sqrt(-math.log(den / 2) / (2 * math.pi**2))
    frac = den / want
    print(f"{f.relative_to(IMPL)}:{line}-{line + 1}")
    print(f"  theta_den[0] = {mt.group(1)} = {den:.6e}  (implies eta = {eta_impl:.4f})")
    print(f"  required for eta = {ETA[d]}: {want:.6e} = {round(want * 2**62):#x}"
          f"  (2r = 2^{math.log2(2 * r):.2f}; Table 3 epsilon = 2^{EPS_LOG2[d]})")
    print(f"  RingSamp eta from inv_2sqreta = {eta_ringsamp:.4f}; "
          f"fraction of the centre-dependent correction applied = {frac:.3f}; "
          f"max uncorrected per-coordinate factor = {4 * (want - den):.2e}")
    if abs(eta_ringsamp - ETA[d]) > 5e-4:
        fail(f"{f}: RingSamp eta does not match Table 3")
    if abs(math.log2(2 * r) - EPS_LOG2[d]) > 0.1:
        fail(f"{f}: 2r does not match Table 3 epsilon")
    seen.setdefault(d, []).append((round(want * 2**62), int(mt.group(1), 16)))

ok512 = all(abs(w - g) <= 1 for w, g in seen[512])
bad_hi = all(abs(w - g) > 1 for d in (1024, 2048) for w, g in seen[d])
copied = all(g == seen[512][0][1] for d in (1024, 2048) for _, g in seen[d])
print(f"control: 512 constants match the specification: {ok512}")
if ok512 and bad_hi and copied:
    print(f"CONFIRMED {ID}: YuanYang.DSA-1024 and -2048 (reference and optimized) use the "
          f"eta=1.026 ThetaDiv denominator of the 512 set; 12.8% / 2.3% of the specified "
          f"Delta1 correction is applied")
    sys.exit(0)
fail(f"ok512={ok512} mismatch_1024_2048={bad_hi} copied={copied}")
