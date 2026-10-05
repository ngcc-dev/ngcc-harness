#!/bin/sh
set -eu

COMMIT=b1b7e429d4bf9673f2040f3e951df827a6d0dbe6
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d /tmp/ngcc-flit-fo.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

git init -q "$TMP/source"
git -C "$TMP/source" remote add origin https://github.com/acprk/ngcc-round1-cryptanalysis.git
if ! git -C "$TMP/source" fetch -q --depth=1 origin "$COMMIT"; then
    echo 'SKIP kem-15-2: pinned external artifact is unavailable' >&2
    exit 77
fi
git -C "$TMP/source" checkout -q --detach FETCH_HEAD
test "$(git -C "$TMP/source" rev-parse HEAD)" = "$COMMIT"
POC="$TMP/source/flit-opt-verify-fo-bypass"
LOG="$TMP/run.log"

REF="$HERE/Implementations" "$POC/run_all.sh" | tee "$LOG"

test "$(grep -Ec 'called EQUAL [1-9][0-9]*, NOT-equal' "$LOG")" -ge 4
test "$(grep -Fc 'accept rate 0.0000%' "$LOG")" -ge 3
test "$(grep -Fc "oracle == predict(correct m'): YES ; == predict(wrong m'): NO" "$LOG")" -eq 5
grep -Fq 'summary: tested ' "$LOG"

echo 'ORACLE kem-15-2 FLIT CONFIRMED optimized equality test accepts invalid ciphertexts and exposes a plaintext-checking oracle'
echo 'LIMITATION kem-15-2 no full secret-key recovery was executed'
