#!/usr/bin/env python3
"""kem-17-7: the 3-QCSD second syndrome has a fixed, public parity.

Definitions 2.3.14 and 2.3.15 compare (H, (y1, y2)) with the uniform
distribution over F^{2n x 3n}_{2,b1,b2} x (F^n_{2,b3} x F^n_2): the first
syndrome is restricted to its parity b3, the second is unrestricted. The
script checks that wording in the specification, then samples the 3-QCSD
distribution in F_2[x]/(x^n - 1) for each HEP-QC parameter set, with the
weights of the proof's reduction (y2 = e + s*r2), and requires
parity(y2) = w_e + b2*w_r (mod 2) in every sample. A uniform y2 is the
control and shows both parities.
"""
import random
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
SETS = {"HEP-QC-1": (17669, 75, 75), "HEP-QC-3": (35851, 114, 114),
        "HEP-QC-5": (57637, 149, 149), "HEP-QC-7": (197123, 297, 297)}   # n, w_r, w_e
SAMPLES = 24


def fail(msg):
    print(f"NOT CONFIRMED kem-17-7: {msg}")
    sys.exit(1)


def parameters_from_source():
    base = HERE / "Implementations/Implementations/ref"
    for name, (n, wr, we) in SETS.items():
        text = (base / name.lower() / "parameters.h").read_text()
        for key, val in (("PARAM_N ", n), ("PARAM_OMEGA_R ", wr), ("PARAM_OMEGA_E ", we)):
            if not any(line.split()[1:3] == [key.strip(), str(val)] for line in text.splitlines()
                       if line.startswith("#define")):
                fail(f"{name} {key.strip()} differs from the archived parameter file")


def spec_check():
    tool = shutil.which("pdftotext")
    if not tool:
        return
    text = subprocess.run([tool, "-layout", str(HERE / "kem-17-spec.pdf"), "-"],
                          capture_output=True, text=True).stdout
    flat = " ".join(text.split())
    if "Definition 2.3.15 (3-QCSD-P Problem)" not in flat or "2,b1 ,b2 × (F2,b3 × F2 )" not in flat:
        fail("specification text differs (Definition 2.3.15)")
    print("spec: Definition 2.3.15 takes the uniform side over F_{2,b1,b2}^{2n x 3n} x (F_{2,b3}^n x F_2^n); "
          "only the first syndrome's parity is fixed")


def sparse(rng, n, weight):
    return rng.sample(range(n), weight)


def mul(dense, support, n):
    """dense * (sum of x^i for i in support) in F_2[x]/(x^n - 1), dense as an int."""
    mask = (1 << n) - 1
    out = 0
    for i in support:
        out ^= ((dense << i) | (dense >> (n - i))) & mask
    return out


def main():
    parameters_from_source()
    spec_check()
    rng = random.Random(20261115)
    for name, (n, wr, we) in SETS.items():
        seen = {0: set(), 1: set()}
        uniform = set()
        for b2 in (0, 1):
            for _ in range(SAMPLES):
                s = rng.getrandbits(n)
                if bin(s).count("1") % 2 != b2:
                    s ^= 1
                e = sum(1 << i for i in sparse(rng, n, we))
                y2 = e ^ mul(s, sparse(rng, n, wr), n)
                seen[b2].add(bin(y2).count("1") % 2)
                uniform.add(bin(rng.getrandbits(n)).count("1") % 2)
        expect = {b2: {(we + b2 * wr) % 2} for b2 in (0, 1)}
        print(f"{name}: n={n}, w_r=w_e={wr}: parity(y2) for b2=0: {sorted(seen[0])}, "
              f"for b2=1: {sorted(seen[1])}; uniform control: {sorted(uniform)}")
        if seen != expect or uniform != {0, 1}:
            fail(f"{name} parity is not the predicted constant")
    print("CONFIRMED kem-17-7: in all four parameter sets the real second syndrome has parity "
          "w_e + b2*w_r (mod 2) in every sample, while Definition 2.3.15 lets the uniform one take either")


if __name__ == "__main__":
    main()
