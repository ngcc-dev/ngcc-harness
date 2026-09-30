#!/usr/bin/env python3
"""Reproduce the NSS-HQC ephemeral-decoding work factors."""

import importlib.metadata
import json
import math
import sys


CASES = {
    "128": (58886, 29443, 132, 29443, 117, 15),
    "256": (108986, 54493, 212, 54493, 187, 19),
    "384": (245158, 122579, 318, 122579, 279, 26),
    "512": (435802, 217901, 426, 217901, 372, 33),
}


def imports():
    version = importlib.metadata.version("cryptographic-estimators")
    if version != "2.1.1":
        raise SystemExit(f"expected cryptographic-estimators==2.1.1, found {version}")
    from cryptographic_estimators.SDEstimator import SDEstimator
    from cryptographic_estimators.SDEstimator.SDAlgorithms import (
        BallCollision,
        BJMM,
        BJMMdw,
        BJMMpdw,
        BJMMplus,
        BothMay,
        Dumer,
        MayOzerov,
        Prange,
        Stern,
    )
    return SDEstimator, Stern, [
        BallCollision, BJMM, BJMMdw, BJMMpdw, BJMMplus, BothMay,
        Dumer, MayOzerov, Prange,
    ]


def estimate(level, values, classes):
    SDEstimator, Stern, excluded = classes
    n, k, w, targets, additive_weight, rs_radius = values
    estimator = SDEstimator(
        n=n,
        k=k,
        w=w,
        memory_bound=512,
        bit_complexities=True,
        excluded_algorithms=excluded,
    )
    stern = estimator.algorithms()[0]
    raw = stern.time_complexity()
    memory = stern.memory_complexity()
    discount = 0.5 * math.log2(targets)
    log2_solutions = (
        math.lgamma(n + 1) - math.lgamma(w + 1)
        - math.lgamma(n - w + 1)
    ) / math.log(2) - (n - k)
    quantizer = [(j * 3 + 2) // 5 for j in range(6)]
    complement = all(quantizer[5 - j] == 3 - quantizer[j] for j in range(6))
    bad_symbols = additive_weight // 32
    return {
        "level": level,
        "raw_time": raw,
        "memory": memory,
        "qc_doom_discount": discount,
        "discounted_time": raw - discount,
        "spurious_solutions_log2": log2_solutions,
        "max_bad_outer_symbols": bad_symbols,
        "rs_error_radius": rs_radius,
        "public_message_recovery": complement and bad_symbols <= rs_radius,
        "parameters": stern.get_optimal_parameters_dict(),
    }


def main():
    wanted = sys.argv[1:] or list(CASES)
    if any(level not in CASES for level in wanted):
        raise SystemExit("usage: reproduce_isd_estimate.py [128|256|384|512 ...]")
    classes = imports()
    results = [estimate(level, CASES[level], classes) for level in wanted]
    print(json.dumps(results, indent=2, sort_keys=True))
    expected = {"256": 256, "384": 384, "512": 512}
    for row in results:
        if row["level"] in expected:
            assert row["raw_time"] < expected[row["level"]]
            assert row["public_message_recovery"]
    print("NSS_HQC_EPHEMERAL_DECODING_TARGETS=CONFIRMED")


if __name__ == "__main__":
    main()
