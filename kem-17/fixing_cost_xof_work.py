#!/usr/bin/env python3
"""Fixing-cost counts for kem-17 (HEP-QC): sizes and SM3 compression counts.

Stdlib only. Prints numbers; implements no part of the scheme.

Part 1 reproduces specification Table 4.2 (physical p. 18) from the formulas
of section 4.2 and Table 4.1, then evaluates them with a wider seed.

Part 2 counts SM3 compression calls made through the wrapper XOF
(common/symmetric.c:42-44,111-116) and the hashes H and J, per operation.
The contest pseudoXOF (Lib/auxfunc/auxfunc.c:482-522) hashes msg || counter
once for every 32 output bytes, and SM3 pads a B-byte input to
ceil((8B + 65) / 512) blocks.

  submitted: 208-byte input buffer, 26-byte state (CTX_LEN * 8 bits).
  level:     input buffer and state both level/8 bytes.
  level+pad: 208-byte input buffer, state level/8 bytes.
  full:      208-byte input buffer, 208-byte state (bit length -> byte length).

Rejected samples are not counted, except the expected 1/0.288788 attempts of
the invertible-matrix loop. Calls to G and the seed-splitting hash are a few
compressions each and are left out. Only SM3 work is counted; polynomial
multiplication, decoding and matrix work are not converted to this unit.
"""

SETS = {
    # name: (level, n, n1, n2, k, omega, omega_r)   Table 4.1, physical p. 18
    "HEP-QC-1": (128, 17669, 46, 384, 128, 66, 75),
    "HEP-QC-3": (192, 35851, 56, 640, 192, 100, 114),
    "HEP-QC-5": (256, 57637, 90, 640, 256, 131, 149),
    "HEP-QC-7": (512, 197123, 220, 896, 512, 261, 297),
}
TABLE_4_2 = {  # ek, dk, ct
    "HEP-QC-1": (285889, 285969, 4433),
    "HEP-QC-3": (866210, 866298, 8978),
    "HEP-QC-5": (1852485, 1852581, 14421),
    "HEP-QC-7": (12644449, 12644577, 49297),
}
SALT = 16
P_INVERTIBLE = 0.288788  # prod_{i>=1} (1 - 2^-i)


def ceil_div(a, b):
    return -(-a // b)


def sizes(name, seed_pub, seed_sec):
    _, n, n1, n2, k, _, _ = SETS[name]
    ek = seed_pub + ceil_div(n, 8) + 8 * k * ceil_div(n, 64)
    dk = ek + seed_sec + ceil_div(k, 8) + seed_sec
    ct = ceil_div(n, 8) + ceil_div(n1 * n2, 8) + SALT
    return ek, dk, ct


def sm3_blocks(nbytes):
    return ceil_div(8 * nbytes + 65, 512)


def xof_call(out_bytes, in_bytes):
    """Compressions in one pseudoXOF call."""
    return ceil_div(out_bytes, 32) * sm3_blocks(in_bytes + 4)


class Wrapper:
    def __init__(self, aux, state):
        self.aux, self.state = aux, state
        self.output = 0.0   # compressions producing output bytes
        self.update = 0.0   # compressions producing the next state
        self.other = 0.0    # H(ek), J

    def init(self, seed_bytes):
        self.update += xof_call(self.state, seed_bytes)

    def get(self, out_bytes, times=1):
        self.output += times * xof_call(out_bytes, self.aux)
        self.update += times * xof_call(self.state, self.aux)

    def total(self):
        return self.output + self.update + self.other


def expand_dk(w, name, seed):
    _, n, _, _, k, omega, _ = SETS[name]
    nprime = 64 * ceil_div(n, 64)
    w.init(seed)
    w.get(3, 2 * omega)                      # vector.c:69, y and x
    w.get(k // 8, k / P_INVERTIBLE)          # vector.c:349
    w.get(4, nprime)                         # vector.c:421


def encrypt(w, name, seed):
    _, n, _, _, _, _, omega_r = SETS[name]
    w.init(seed)                             # theta
    w.init(32)                               # public seed
    w.get(ceil_div(n, 8))                    # vector.c:206, h
    w.get(4 * omega_r, 3)                    # vector.c:104


def count(name, op, aux, state, seed):
    _, n, n1, n2, k, _, _ = SETS[name]
    nprime = 64 * ceil_div(n, 64)
    ek = TABLE_4_2[name][0]
    w = Wrapper(aux, state)
    if op == "keygen":
        w.get(seed)                          # KEM_HEP_QC.c:72
        w.init(seed)
        w.get(seed)
        w.get(k // 8)
        expand_dk(w, name, seed)
        w.init(32)
        w.get(ceil_div(n, 8))
        w.get(ceil_div(k * (nprime - n1 * n2), 8))   # code.c:138
    elif op == "encaps":
        w.get(k // 8)                        # KEM_HEP_QC.c:125
        w.get(SALT)
        w.other += sm3_blocks(ek)            # H(ek), symmetric.c:52
        encrypt(w, name, seed)
    elif op == "decaps":
        expand_dk(w, name, seed)
        w.other += sm3_blocks(ek)
        encrypt(w, name, seed)
        w.other += sm3_blocks(SALT + 16 * ceil_div(n, 64)) + 4   # J
    return w


def main():
    print("Part 1: sizes in bytes (ek, dk, ct)")
    for name in SETS:
        got = sizes(name, 32, 32)
        print(f"  {name}: formula {got}  Table 4.2 {TABLE_4_2[name]}  "
              f"match={got == TABLE_4_2[name]}")
    a = sizes("HEP-QC-7", 32, 64)
    b = sizes("HEP-QC-7", 64, 64)
    base = TABLE_4_2["HEP-QC-7"]
    print(f"  HEP-QC-7, secret seeds 64 B, public seed 32 B: {a}")
    print(f"  HEP-QC-7, every seed 64 B: {b}  "
          f"ek +{b[0] - base[0]} B = +{100 * (b[0] - base[0]) / base[0]:.5f}%")

    print()
    print("Part 2: SM3 compressions per operation")
    print("  one call of 4 output bytes, output + state update:")
    for label, aux, state in (("submitted", 208, 26), ("level 256", 32, 32),
                              ("level 512", 64, 64), ("level+pad 512", 208, 64),
                              ("full", 208, 208)):
        print(f"    {label:14s} {xof_call(4, aux)} + {xof_call(state, aux)}"
              f" = {xof_call(4, aux) + xof_call(state, aux)}")
    for name in ("HEP-QC-5", "HEP-QC-7"):
        level = SETS[name][0]
        lv = level // 8
        seed = 32 if name != "HEP-QC-7" else 64
        variants = (("submitted", 208, 26, 32), ("level", lv, lv, seed),
                    ("level+pad", 208, lv, seed), ("full", 208, 208, seed))
        for op in ("keygen", "encaps", "decaps"):
            base = count(name, op, 208, 26, 32)
            print(f"  {name} {op}: submitted total {base.total():.0f} "
                  f"(output {base.output:.0f}, state update {base.update:.0f}, "
                  f"H and J {base.other:.0f}); "
                  f"state update share {100 * base.update / base.total():.1f}%")
            for label, aux, state, sd in variants[1:]:
                w = count(name, op, aux, state, sd)
                print(f"      {label:10s} total {w.total():.0f}  "
                      f"ratio {w.total() / base.total():.2f}")


if __name__ == "__main__":
    main()
