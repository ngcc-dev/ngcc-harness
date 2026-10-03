#!/usr/bin/env python3
"""Reproduce the HARE same-key DS-DOOM target crossings."""

import hashlib
import importlib.util
import io
import os
from pathlib import Path
import sys
import tarfile
import urllib.request
import urllib.error

COMMIT = "39b78dcc077793cfa3ccdce8d032ece76825ca55"
URL = f"https://github.com/Gabsadio/Multi-Instance-KEM/archive/{COMMIT}.tar.gz"
ARCHIVE_SHA256 = "64b7c4d4cd8a44f6f7b3d4f25822f773f64fecdeb11c30923e8b626de7cd805e"
DOOM_SHA256 = "d53e8bb0b668f581d4fd9538446726760bdcb4898ca232dc24a64dd473018b1d"

CASES = (
    # Use the conservative ordinary code dimension n rather than n-2.
    # name, code length, code dimension, weight, sessions, target, affected
    ("HARE-128", 41798, 20899, 158, 80, 128, False),
    ("HARE-256", 104758, 52379, 262, 72, 256, True),
    ("HARE-384", 209738, 104869, 386, 64, 384, True),
    ("HARE-512", 347962, 173981, 518, 75, 512, True),
)


def load_estimator():
    try:
        archive = urllib.request.urlopen(URL).read()
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
    configured = os.environ.get("NGCC_ESTIMATOR_PYTHON")
    if configured and Path(configured).resolve() != Path(sys.executable).resolve():
        os.execv(configured, [configured, __file__])
    try:
        dsdoom = load_estimator()
    except ImportError as exc:
        raise SystemExit("the pinned estimator requires numpy and scipy") from exc
    for name, n, k, w, sessions, target, affected in CASES:
        time, memory, _ = dsdoom(n, k, w, (n // 2) * (1 << sessions))
        if affected:
            previous, _, _ = dsdoom(n, k, w, (n // 2) * (1 << (sessions - 1)))
            assert previous >= target and time < target
            verdict = "BELOW-TARGET"
        else:
            assert time >= target
            verdict = "CONTROL-ABOVE-TARGET"
        print(f"{name}: sessions=2^{sessions} time=2^{time:.3f} memory=2^{memory:.3f} {verdict}")
    print("ATTACK kem-16-2 CONFIRMED: pinned DS-DOOM estimates cross three HARE targets")


if __name__ == "__main__":
    main()
