#!/usr/bin/env python3
"""Create or check the exact deterministic SHA-256 source manifest."""

import argparse
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "MANIFEST.tsv"
EXCLUDED_PATHS = {"MANIFEST.tsv", "MANIFEST.sha256"}


def included(path):
    """Exclude self-referential manifests and interpreter caches.

    The manifest is a source-package gate. Python may create `__pycache__`
    directories while running gates, validators, or syntax checks; those runtime
    cache files are not source artifacts and must not make the source tree fail
    its own manifest gate.
    """
    rel = path.relative_to(ROOT).as_posix()
    parts = rel.split("/")
    if rel in EXCLUDED_PATHS:
        return False
    if "__pycache__" in parts:
        return False
    if rel.endswith((".pyc", ".pyo")):
        return False
    return True


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def generate():
    rows = ["sha256\tpath\n"]
    for path in sorted(p for p in ROOT.rglob("*") if p.is_file() and included(p)):
        rows.append("{}\t{}\n".format(digest(path), path.relative_to(ROOT).as_posix()))
    return "".join(rows)


def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--write", action="store_true")
    group.add_argument("--check", action="store_true")
    args = parser.parse_args()

    generated = generate()
    if args.check:
        if not OUT.exists() or OUT.read_text(encoding="utf-8") != generated:
            print("source manifest check: FAIL")
            return 1
        print("source manifest check: PASS")
        return 0

    OUT.write_text(generated, encoding="utf-8")
    manifest_hash = hashlib.sha256(generated.encode("utf-8")).hexdigest()
    (ROOT / "MANIFEST.sha256").write_text(
        "{}  MANIFEST.tsv\n".format(manifest_hash), encoding="utf-8"
    )
    print(OUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
