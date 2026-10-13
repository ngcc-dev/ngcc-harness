#!/usr/bin/env python3
"""Evaluate the additive QROM terms of CreTAKE Theorems 1, 2 and 4.

Only arithmetic is certified here; this is not a protocol attack.
"""

from math import log2


def main() -> None:
    # Theorems 1 and 2, physical pp. 21 and 23. Set the number of
    # sessions, parties and oracle queries to their minimal positive values.
    sessions = parties = oracle_queries = 1
    k2s_coefficient = 2 * oracle_queries * sessions * (sessions + 2 * parties)
    s2k_coefficient = (2 * oracle_queries * sessions * parties * (sessions + 1)
                       + 2 * oracle_queries * sessions * (2 * sessions + 1))
    s2s_coefficient = (2 * oracle_queries * sessions
                       * (2 * sessions + sessions * parties + 1))
    assert k2s_coefficient == 6 and s2k_coefficient == 10
    assert s2s_coefficient == 8

    for level, quantum_target in ((128, 80), (256, 128), (512, 256)):
        k2s_bits = level / 2 - log2(k2s_coefficient)
        s2k_bits = level / 2 - log2(s2k_coefficient)
        s2s_bits = level / 2 - log2(s2s_coefficient)
        assert max(k2s_bits, s2k_bits, s2s_bits) < quantum_target
        print(f"{level}: K2S bound term 2^-{k2s_bits:.3f}; "
              f"S2K 2^-{s2k_bits:.3f}; S2S 2^-{s2s_bits:.3f}; "
              f"quantum target 2^-{quantum_target}")
    print("INFO kex-03-8 CONFIRMED: K2S/S2K/S2S bounds miss targets at one session and one oracle query; no attack follows")


if __name__ == "__main__":
    main()
