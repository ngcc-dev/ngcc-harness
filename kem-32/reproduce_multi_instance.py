#!/usr/bin/env python3
"""Reproduce the QCTM DS-DOOM target crossings."""

import hashlib
import importlib.util
import io
import tarfile
import urllib.request


COMMIT = "39b78dcc077793cfa3ccdce8d032ece76825ca55"
URL = f"https://github.com/Gabsadio/Multi-Instance-KEM/archive/{COMMIT}.tar.gz"
ARCHIVE_SHA256 = "64b7c4d4cd8a44f6f7b3d4f25822f773f64fecdeb11c30923e8b626de7cd805e"
DOOM_SHA256 = "d53e8bb0b668f581d4fd9538446726760bdcb4898ca232dc24a64dd473018b1d"

CASES = (
    # The two non-circulant tail rows are omitted when making the 19 public
    # rotated targets, then restored as fixed-weight parity for verification.
    # Thus k_eff=(n-u+r0)-1 is one larger than the submitted code dimension.
    # name, n, k_eff, w, rotations, log2(sessions), target, affected
    ("QCTM128", 10070, 4958, 142, 19, 79, 128, True),
    ("QCTM256", 19000, 9784, 256, 19, 76, 256, True),
    ("QCTM512", 38000, 19892, 503, 19, 80, 512, False),
)


def load_estimator():
    archive = urllib.request.urlopen(URL).read()
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
    try:
        dsdoom = load_estimator()
    except ImportError as exc:
        raise SystemExit("the pinned estimator requires numpy and scipy") from exc

    for name, n, k, w, rotations, sessions, target, affected in CASES:
        time, memory, _ = dsdoom(n, k, w, rotations * (1 << sessions))
        if affected:
            previous, _, _ = dsdoom(
                n, k, w, rotations * (1 << (sessions - 1))
            )
            assert previous >= target and time < target
            verdict = "BELOW-TARGET"
        else:
            assert time >= target
            verdict = "CONTROL-ABOVE-TARGET"
        print(
            f"{name}: sessions=2^{sessions} time=2^{time:.3f} "
            f"memory=2^{memory:.3f} {verdict}"
        )

    print("QCTM_MULTI_INSTANCE_TARGETS=CONFIRMED")


if __name__ == "__main__":
    main()
