#!/bin/sh
set -eu

# Pinned public experiment by Zhenyu Xiong and Mingsheng Wang.
COMMIT=43be9988ab37c8dc41ac72b804820822beebbbbe
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d /tmp/ngcc-yuanyang-sampler.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

git clone --quiet https://github.com/acprk/ngcc-round1-cryptanalysis.git "$TMP/audit"
git -C "$TMP/audit" checkout --quiet "$COMMIT"
test "$(git -C "$TMP/audit" rev-parse HEAD)" = "$COMMIT"

PYTHON_BIN=${PYTHON_BIN:-python3}
"$PYTHON_BIN" -c 'import numpy' || {
    echo "The pinned regression requires NumPy; set PYTHON_BIN to a Python with NumPy." >&2
    exit 2
}
mkdir "$TMP/bin"
ln -s "$(command -v "$PYTHON_BIN")" "$TMP/bin/python3"

cd "$TMP/audit/yuanyang-dsa-sampler-constant"
PATH="$TMP/bin:$PATH" \
REF="$HERE/Implementations/Reference_Implementation/yuanyang-512" \
    ./run_all.sh "${1:-20000}"

echo "== pinned isolated Delta2 comparison (400,000-signature key-0 logs)"
grep 'per-sig attempts:' logs/orig_k0.out logs/d2scaled_k0.out
