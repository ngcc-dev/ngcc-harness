#!/usr/bin/env python3
"""Reproduce NSS-HQC static-key Stern costs with a pinned estimator version."""

import importlib.metadata
from math import lgamma, log, log2


CASES = (
    # level, ring length, weight of each secret vector, raw Stern cost
    (128, 29443, 73, 167.887),
    (256, 54493, 117, 255.868),
    (384, 122579, 175, 372.995),
    (512, 217901, 233, 489.799),
)


def main():
    assert importlib.metadata.version("cryptographic-estimators") == "2.1.1"
    from cryptographic_estimators.SDEstimator import SDEstimator
    from cryptographic_estimators.SDEstimator.SDAlgorithms import (
        BallCollision, BJMM, BJMMdw, BJMMpdw, BJMMplus,
        BothMay, Dumer, MayOzerov, Prange,
    )

    excluded = [BallCollision, BJMM, BJMMdw, BJMMpdw, BJMMplus,
                BothMay, Dumer, MayOzerov, Prange]
    for level, n, wsk, expected in CASES:
        length, weight = 2 * n, 2 * wsk
        estimator = SDEstimator(
            n=length, k=n, w=weight, memory_bound=512,
            bit_complexities=True, excluded_algorithms=excluded,
        )
        stern = estimator.algorithms()[0]
        raw = stern.time_complexity()
        memory = stern.memory_complexity()
        discount = 0.5 * log2(n)
        # Expected unrelated weight-w errors matching this public syndrome.
        log2_spurious = (lgamma(length + 1) - lgamma(weight + 1)
                         - lgamma(length - weight + 1)) / log(2) - n
        assert abs(raw - expected) < 0.02, (level, raw, expected)
        assert log2_spurious < -1000 and memory < level
        if level == 128:
            assert raw > level and raw - discount > level
        else:
            assert raw < level and raw - discount < level
        print(f"NSS-HQC-{level}: key SD({length},{n},{weight}) "
              f"Stern raw=2^{raw:.3f} memory=2^{memory:.3f} "
              f"with-QC-discount=2^{raw-discount:.3f} "
              f"spurious-solutions<2^{log2_spurious:.0f}", flush=True)
    print("NSS_HQC_STATIC_KEY_TARGETS=CONFIRMED")


if __name__ == "__main__":
    main()
