#!/usr/bin/env python3
"""Fixing-cost check for kex-08-2: are the two hard-coded NIIKE-lv512 secret
vectors inside the range [0, B*e_i] that Algorithm 4 samples from? At B = 1
this is the range [0, e_i] of Algorithm 8, the key generation of NGCC-III.

Reads the submitted sources only. It prints counts and implements no scheme.

    python3 -B kex-08/fixing_cost_lv512_key_range.py
"""

import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent
IMPL = ROOT / "Implementations and Test_Vectors" / "Implementations"
LV = "NIIKE-lv512"


def ints(body):
    return [int(x, 0) for x in body.replace("\n", " ").split(",") if x.strip()]


def define(text, name):
    return int(re.search(r"#define\s+%s\s+(\d+)" % name, text).group(1))


ref = IMPL / "Reference_Implementation" / LV
opt = IMPL / "Optimized_Implementation" / LV
helper = (ref / "protocols" / "bundleprotocols_internal.c").read_text()
setup = (ref / "precomp" / "include" / "protocol_setup.h").read_text()
ecpar = (ref / "precomp" / "include" / "ec_params.h").read_text()
tors = (ref / "precomp" / "torsion_constants.c").read_text()

B = define(setup, "ITERATIONS")
r = define(setup, "CYCLE_LENGTH")
n = define(ecpar, "P_LEN") + define(ecpar, "M_LEN")
e = ints(re.search(r"TORSION_ODD_POWERS\[[^\]]*\]\s*=\s*\{([^}]*)\}", tors).group(1))
vecs = [ints(b) for b in re.findall(r"secretkey_t temp\d = \{([^}]*)\}", helper)]

print("ITERATIONS B = %d, CYCLE_LENGTH r = %d, entries n = %d" % (B, r, n))
print("exponents: " + ", ".join("e_i = %d for %d of them" % (v, e.count(v))
                               for v in sorted(set(e))))
print("log2 of the specified key space prod(1 + B*e_i) = %.1f"
      % sum(math.log2(1 + B * x) for x in e))
assert len(e) == n and len(vecs) == 2
for k, v in enumerate(vecs, 1):
    assert len(v) == n
    out = sum(1 for a, b in zip(v, e) if a < 0 or a > B * b)
    print("literal vector %d: %d entries, values %s, outside [0, B*e_i]: %d"
          % (k, len(v), {x: v.count(x) for x in sorted(set(v))}, out))

# Expected DRNG draws of the commented-out sampling loop: one masked draw per
# attempt, accepted when the value is at most B*e_i.
mask = ints(re.search(r"secretkey_mask\[[^\]]*\]\s*=\s*\{([^}]*)\}",
                      (ref / "precomp" / "include" / "protocol_constants.h").read_text()).group(1))
draws = sum((m + 1) / (B * x + 1) for m, x in zip(mask, e))
print("expected draws of the sampling loop per key: %.0f" % draws)

same = (opt / "protocols" / "bundleprotocols_internal.c").read_bytes() == \
    (ref / "protocols" / "bundleprotocols_internal.c").read_bytes()
print("optimized helper byte-identical to reference: %s" % same)
