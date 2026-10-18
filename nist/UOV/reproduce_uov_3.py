#!/usr/bin/env python3
"""Recompute the uov-Is collision-forgery RAM-gate estimate."""

from math import log2


def main():
    q, m = 16, 64
    r = log2(q)
    # UOV v3.0 section 4.1 uses 2^10.7 sqrt(q^m m r).
    specification_cost = 10.7 + (m * r + log2(m * r)) / 2
    rescaling_gain = log2(q) / 2
    rescaled_cost = specification_cost - rescaling_gain
    checks = {
        "specification_cost": abs(specification_cost - 142.7) < 1e-9,
        "sqrt_q_gain": abs(rescaling_gain - 2.0) < 1e-9,
        "rescaled_cost": abs(rescaled_cost - 140.7) < 1e-9,
    }
    print(f"specification RAM-gate estimate: 2^{specification_cost:.1f}")
    print(f"sqrt(q) rescaling gain: 2^{rescaling_gain:.1f}")
    print(f"rescaled RAM-gate estimate: 2^{rescaled_cost:.1f}")
    print("PASS UOV-3" if all(checks.values()) else f"FAIL UOV-3: {checks}")
    return 0 if all(checks.values()) else 1


if __name__ == "__main__":
    raise SystemExit(main())
