#!/usr/bin/env python3
"""Check the additive AFS-KEX key-secrecy failure terms in Table 1.

The constants are transcribed from the frozen specification. This is a
proof-bound certificate, not a protocol attack or a DFR measurement.
"""

from math import log2


# (profile, classical level, stated -log2(delta_AFS)) from Table 1, p. 17.
SETS = (
    ("C128", 128, 67.98),
    ("C256", 256, 122.24),
    ("C384", 384, 166.17),
    ("C512", 512, 124.13),
    ("C128-s", 128, 74.44),
    ("C256-s", 256, 158.22),
    ("C384-s", 384, 223.72),
    ("C512-s", 512, 168.48),
)
QUANTUM_TARGET = {128: 80, 256: 128, 384: 192, 512: 256}


def main() -> None:
    affected = []
    for name, level, delta_bits in SETS:
        # Theorem 9.3 has 16*N*S^2*delta; Theorem 10.6 has
        # 5*N*S^2*delta. Set N=S=1 to minimize these terms.
        pmake_bits = delta_bits - log2(16)
        puake_bits = delta_bits - log2(5)
        target = QUANTUM_TARGET[level]
        misses = pmake_bits < target or puake_bits < target
        if misses:
            affected.append(name)
        print(f"{name}: pMAKE {pmake_bits:.2f}, pUAKE {puake_bits:.2f}, "
              f"quantum target {target}, {'BELOW' if misses else 'above'}")
    assert affected == ["C128", "C256", "C384", "C512", "C128-s", "C512-s"]
    print("INFO kex-02-6 CONFIRMED: stated QROM bounds miss six BW-set quantum targets at N=S=1; no attack follows")


if __name__ == "__main__":
    main()
