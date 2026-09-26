#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMMIT=5df01ac2b3e4b7a804fae5b3c5b4f8ad531cdaff
TMP=$(mktemp -d /tmp/ngcc-darts-recovery.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

git clone --quiet https://github.com/roadicing/darts-key-recovery-attack "$TMP/attack"
git -C "$TMP/attack" checkout --quiet "$COMMIT"
test "$(git -C "$TMP/attack" rev-parse HEAD)" = "$COMMIT"

# Use the frozen source already audited here instead of downloading another
# copy of the candidate archive.
mkdir "$TMP/attack/DARTS"
cp -a "$ROOT/sign-08/Implementations" "$TMP/attack/DARTS/"
(cd "$TMP/attack" && sh build.sh)

if [ -n "${NGCC_SAGE_PYTHON:-}" ]; then
    PY=$NGCC_SAGE_PYTHON
elif python3 -c 'import numpy' >/dev/null 2>&1; then
    PY=python3
elif command -v sage >/dev/null 2>&1 && sage -python -c 'import numpy' >/dev/null 2>&1; then
    PY='sage -python'
else
    echo 'DARTS recovery requires Python with NumPy; set NGCC_SAGE_PYTHON.' >&2
    exit 2
fi

D128="$TMP/attack/DARTS/Implementations/Reference_Implementation/DARTS128"
(cd "$D128" && $PY recover.py test_data --compare-sk)
python3 "$ROOT/sign-08/verify_recovered_key.py" \
    "$ROOT/sign-08/lib/libDARTS128.so" \
    "$D128/test_data/pk.bin" "$D128/test_data/recovered_sk.bin"
