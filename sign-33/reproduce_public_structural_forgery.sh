#!/bin/sh
set -eu

commit=91f2ddf0590a24ad39afc2ce2ee4f2627ec54726
archive_sha256=cdd6e44eaea61191ecd33b330f14d061c12c46a5ea13bb74b85d104fb29dbeb2
url="https://codeload.github.com/martinfeussner/NGCC-Signature-Audit/tar.gz/$commit"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/ngcc-vdoo-structural.XXXXXX")
trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM

if ! curl -L --fail --silent --show-error "$url" -o "$tmp/package.tar.gz"; then
    echo 'SKIP sign-33-6: pinned attack package is unavailable' >&2
    exit 77
fi
printf '%s  %s\n' "$archive_sha256" "$tmp/package.tar.gz" | sha256sum -c -
tar -xzf "$tmp/package.tar.gz" -C "$tmp"
root="$tmp/NGCC-Signature-Audit-$commit/VDOO/reproducer"

if [ -n "${NGCC_SAGE_PYTHON:-}" ]; then
    python=$NGCC_SAGE_PYTHON
elif [ -n "${PYTHON:-}" ]; then
    python=$PYTHON
elif python3 -c 'import numpy' >/dev/null 2>&1; then
    python=python3
elif command -v sage >/dev/null 2>&1 && sage -python -c 'import numpy' >/dev/null 2>&1; then
    python=$(sage -python -c 'import sys; print(sys.executable)')
else
    echo 'SKIP sign-33-6: Python with NumPy is unavailable; set NGCC_SAGE_PYTHON' >&2
    exit 77
fi
if ! "$python" -c 'import numpy' >/dev/null 2>&1; then
    echo 'SKIP sign-33-6: selected Python lacks NumPy; set NGCC_SAGE_PYTHON' >&2
    exit 77
fi

log="$tmp/reproducer.log"
if [ "${VDOO_FULL:-0}" = 1 ]; then
    (cd "$root" && PATH="$(dirname "$python"):$PATH" sh ./run_full.sh "$tmp/full") 2>&1 | tee "$log"
    grep -Fq 'SUBMITTED VERIFIER ACCEPTED THE PUBLIC-KEY-ONLY FORGERY' "$log"
    echo 'ATTACK sign-33-6 CONFIRMED: full public-key-only VDOO-128 recovery forged a fresh message'
else
    (cd "$root" && PATH="$(dirname "$python"):$PATH" sh ./verify_evidence.sh) 2>&1 | tee "$log"
    grep -Fq 'forgery=0 changed-message=-1 changed-signature=-1' "$log"
    grep -Fq 'VDOO evidence replay: PASS' "$log"
    echo 'EVIDENCE sign-33-6 CONFIRMED: posted VDOO-128 forgery and two reject controls replayed'
fi
