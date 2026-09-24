"""Canonical MAMBA-NIKE estimator inputs.

The Delta columns in ``parameters.json`` are the source of truth.  The
compression widths are derived as t_x = log2(q / Delta_x), with q = 8192.
No independent hand-written T values are kept here.
"""

from __future__ import annotations

import json
from math import log2
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "parameters.json"
LOG2Q = 13
Q = 1 << LOG2Q


def _ilog2_power_of_two(x: int) -> int:
    assert x > 0 and x & (x - 1) == 0
    return int(log2(x))


def _load_profiles():
    data = json.loads(MANIFEST.read_text())
    assert int(data["q"]) == Q
    assert int(data["log2q"]) == LOG2Q
    rows = []
    eta = {}
    for item in data["profiles"]:
        name = item["name"]
        target = int(item["level"])
        n = int(item["n"])
        delta_pk = int(item["delta_pk"])
        delta_u = int(item["delta_u"])
        t_pk = LOG2Q - _ilog2_power_of_two(delta_pk)
        t_u = LOG2Q - _ilog2_power_of_two(delta_u)
        assert t_pk == 13 - log2(delta_pk)
        assert t_u == 13 - log2(delta_u)
        rows.append((name, target, n, delta_pk, delta_u, t_pk, t_u))
        eta[name] = int(item["eta_s"])
        assert eta[name] == int(item["eta_r"])
    return rows, eta


PROFILES, ETA_RECOMMENDED = _load_profiles()
PROFILES_BASELINE = PROFILES
