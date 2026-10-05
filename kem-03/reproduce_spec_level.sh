#!/bin/sh
set -eu

COMMIT=e6b324cd411758ab5b18a08de999276bf260c9aa
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d /tmp/ngcc-bag-loong-spec.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

git init -q "$TMP/source"
git -C "$TMP/source" remote add origin https://github.com/acprk/ngcc-round1-cryptanalysis.git
if ! git -C "$TMP/source" fetch -q --depth=1 origin "$COMMIT"; then
    echo 'SKIP kem-03-4/kem-03-5: pinned external artifact is unavailable' >&2
    exit 77
fi
git -C "$TMP/source" checkout -q --detach FETCH_HEAD
test "$(git -C "$TMP/source" rev-parse HEAD)" = "$COMMIT"
POC="$TMP/source/bag-loong-spec-level"
LOG="$TMP/run.log"

echo 'Running pinned BAG-Loong checks; this can take about 20 minutes.'
REF="$HERE/Implementations/Reference_Implementation" PY="${PYTHON:-python3}" \
    "$POC/run_all.sh" > "$LOG" 2>&1
tail -n 60 "$LOG"

test "$(grep -Fc 'RESULT rank: keys=20 violations=0' "$LOG")" -eq 4
test "$(grep -Fc 'all_4_columns=True' "$LOG")" -eq 12
grep -Fq 'BAG-Loong-256 (m,N,k,r)=(67,130,65,8)' "$LOG"
grep -Fq 'BAG-Loong-512 (m,N,k,r)=(97,208,104,10)' "$LOG"
test "$(grep -Ec 'RESULT kat BAG-Loong-[0-9]+: 0/10' "$LOG")" -eq 4
test "$(grep -Ec 'RESULT kat BAG-Loong-[0-9]+: 10/10' "$LOG")" -eq 4
test "$(grep -Ec 'tail support dim .*exceeded 100/100' "$LOG")" -eq 4
grep -Fq 'BAG-Loong-256 (m,N,k,r)=(67,130,65,8) omega=2.807: classical 2^246.9 (claim 256)' "$LOG"
grep -Fq 'BAG-Loong-384 (m,N,k,r)=(83,166,83,9) omega=2.807: classical 2^330.7 (claim 384)' "$LOG"
grep -Fq 'BAG-Loong-512 (m,N,k,r)=(97,208,104,10) omega=2.807: classical 2^430.3 (claim 512)' "$LOG"
if grep -Fq 'w=3 256-key ' "$LOG"; then
    grep -Eq 'w=3 256-key .* best=[^ ]+ 243\.3 ' "$LOG"
    grep -Eq 'w=3 384-key .* best=[^ ]+ 333\.3 ' "$LOG"
    grep -Eq 'w=3 512-key .* best=[^ ]+ 432\.9 ' "$LOG"
else
    echo 'NOT CHECKED kem-03-4: optional CryptographicEstimators figures (dependency unavailable)'
fi

echo 'ATTACK kem-03-4 BAG-Loong CONFIRMED merged-support rank lowers the estimated key-recovery costs below the 256/384/512-bit targets'
echo 'CORRECTNESS kem-03-5 BAG-Loong CONFIRMED the submitted decoder cap rejects 40/40 round trips with specification-conforming supports'
echo 'LIMITATION kem-03-4 full-parameter key recovery was not executed; the full-size costs are estimator and closed-form bounds'
