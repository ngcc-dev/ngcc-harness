#!/usr/bin/env python3
"""Check new same-key DS-DOOM crossings against the pinned official estimator.

This reproduces model costs and their preceding above-target controls. It does
not execute a full-size decoder or prove the estimator's heuristic assumptions.
"""

import argparse
import hashlib
import importlib.util
import io
import os
from pathlib import Path
import sys
import tarfile
import urllib.error
import urllib.request


COMMIT = "39b78dcc077793cfa3ccdce8d032ece76825ca55"
URL = f"https://github.com/Gabsadio/Multi-Instance-KEM/archive/{COMMIT}.tar.gz"
ARCHIVE_SHA256 = "64b7c4d4cd8a44f6f7b3d4f25822f773f64fecdeb11c30923e8b626de7cd805e"
DOOM_SHA256 = "d53e8bb0b668f581d4fd9538446726760bdcb4898ca232dc24a64dd473018b1d"

# candidate, set, target bits, code length, dimension, weight, rotations,
# first below-target exponent (None means no crossing at 2^80 sessions).
CASES = (
    ("kem-26", "NSS-HQC-128", 128, 58886, 29443, 132, 29443, 77),
    ("kem-33", "QUBE-128", 128, 39327, 26218, 85, 13109, 71),
    ("kem-33", "QUBE-192", 192, 76071, 50714, 125, 25357, 72),
    ("kem-33", "QUBE-256", 256, 124953, 83302, 166, 41651, 74),
    ("kem-33", "QUBE-384", 384, 248271, 165514, 248, 82757, 80),
    ("kem-33", "QUBE-512-control", 512, 407553, 271702, 329, 135851, None),
    ("kem-36", "TRIKE-2-control", 128, 46743, 15581, 263, 15581, None),
    ("kem-36", "TRIKE-5", 256, 106089, 35363, 429, 35363, 47),
    ("kem-36", "TRIKE-7", 384, 209073, 69691, 659, 69691, 63),
    ("kem-36", "TRIKE-9", 512, 342129, 114043, 877, 114043, 63),
    ("kem-37", "TriQ-KEM-128", 128, 32602, 16301, 134, 16301, 73),
    ("kem-37", "TriQ-KEM-256", 256, 100726, 50363, 264, 50363, 77),
    ("kem-37", "TriQ-KEM-384", 384, 195302, 97651, 392, 97651, 77),
    ("kem-37", "TriQ-KEM-512", 512, 315254, 157627, 520, 157627, 78),
)


def load_estimator():
    try:
        archive = urllib.request.urlopen(URL, timeout=30).read()
    except urllib.error.URLError as exc:
        print(f"SKIP: pinned estimator unavailable: {exc}", file=sys.stderr)
        raise SystemExit(77) from exc
    assert hashlib.sha256(archive).hexdigest() == ARCHIVE_SHA256
    with tarfile.open(fileobj=io.BytesIO(archive), mode="r:gz") as tf:
        members = [m for m in tf.getmembers() if m.name.endswith("/doom.py")]
        assert len(members) == 1
        source = tf.extractfile(members[0]).read()
    assert hashlib.sha256(source).hexdigest() == DOOM_SHA256
    spec = importlib.util.spec_from_loader("pinned_doom", loader=None)
    module = importlib.util.module_from_spec(spec)
    exec(compile(source, "pinned-doom.py", "exec"), module.__dict__)
    return module.dsDOOM


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("candidate", choices=("all", "kem-26", "kem-33", "kem-36", "kem-37"))
    args = parser.parse_args()
    configured = os.environ.get("NGCC_ESTIMATOR_PYTHON")
    if configured and Path(configured).absolute() != Path(sys.executable).absolute():
        os.execv(configured, [configured, __file__, args.candidate])
    try:
        dsdoom = load_estimator()
    except ImportError as exc:
        raise SystemExit("the pinned estimator requires numpy and scipy") from exc

    for cand, name, target, n, k, w, rotations, first in CASES:
        if args.candidate != "all" and args.candidate != cand:
            continue
        exponent = first if first is not None else 80
        time, memory, _ = dsdoom(n, k, w, rotations * (1 << exponent))
        assert memory < target, (name, memory, target)
        if first is None:
            assert time >= target, (name, time, target)
            verdict = "CONTROL-ABOVE-TARGET"
        else:
            previous, _, _ = dsdoom(n, k, w, rotations * (1 << (first - 1)))
            assert previous >= target and time < target, (name, previous, time, target)
            verdict = "BELOW-TARGET-MODEL"
        print(f"{cand} {name}: sessions=2^{exponent} time=2^{time:.3f} "
              f"memory=2^{memory:.3f} {verdict}")
    print("CODE_MULTI_INSTANCE_ESTIMATES=PASS")


if __name__ == "__main__":
    main()
