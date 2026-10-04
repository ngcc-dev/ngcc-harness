#!/usr/bin/env python3
"""Reproduce generic bounded-error LWE estimates for sign-07-4."""

from __future__ import annotations

import math
import os
import subprocess
import sys
from pathlib import Path


EXPECTED_COMMIT = "53da5982597709ba0fdf94ea37a84d822310fd84"


def main() -> None:
    estimator = Path(os.environ.get("LATTICE_ESTIMATOR_PATH", "../lattice-estimator")).resolve()
    if not estimator.is_dir():
        raise SystemExit("set LATTICE_ESTIMATOR_PATH to the pinned malb/lattice-estimator checkout")
    commit = subprocess.check_output(["git", "-C", estimator, "rev-parse", "HEAD"], text=True).strip()
    if commit != EXPECTED_COMMIT:
        raise SystemExit(f"lattice-estimator commit {commit}, expected {EXPECTED_COMMIT}")
    sys.path.insert(0, str(estimator))
    from estimator import LWE, ND  # type: ignore
    from sage.all import log  # type: ignore

    rows = (
        ("CS-128", 256, 32257, 3, 5, 42.6),
        ("CS-256", 512, 64513, 3, 7, 114.7),
        ("CS-512", 512, 64513, 6, 5, 95.5),
    )
    for name, n, q, k, beta, expected in rows:
        params = LWE.Parameters(
            n=n,
            q=q,
            Xs=ND.Uniform(-1, 1, n),
            Xe=ND.Uniform(-(1 << (beta - 1)) + 1, 1 << (beta - 1), k * n),
            m=k * n,
            tag=name,
        )
        estimates = LWE.estimate.rough(params, quiet=True)
        values = {attack: float(log(cost["rop"], 2)) for attack, cost in estimates.items()}
        best_name, best = min(values.items(), key=lambda item: item[1])
        assert math.isclose(best, expected, abs_tol=0.25), (name, values)
        print(f"{name}: best={best_name} log2_rop={best:.3f} all={values}")
    print("ATTACK sign-07-4 CONFIRMED: collapsed public-key estimates miss every target")


if __name__ == "__main__":
    main()
