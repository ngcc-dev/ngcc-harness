#!/bin/sh
set -eu

COMMIT=5568023b43000ce9eb38467705bb2731f230c73b
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d /tmp/ngcc-resolved-tccr.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

git init -q "$TMP/source"
git -C "$TMP/source" remote add origin https://github.com/acprk/ngcc-round1-cryptanalysis.git
if ! git -C "$TMP/source" fetch -q --depth=1 origin "$COMMIT"; then
    echo 'SKIP sign-21-1: pinned external artifact is unavailable' >&2
    exit 77
fi
git -C "$TMP/source" checkout -q --detach FETCH_HEAD
test "$(git -C "$TMP/source" rev-parse HEAD)" = "$COMMIT"
POC="$TMP/source/resolved-alpha-tccr-multitarget"
LOG="$TMP/run.log"

REFROOT="$HERE/Implementations/Reference_Implementation" \
    CC="${CC:-gcc}" FULL="${FULL:-0}" "$POC/run_all.sh" | tee "$LOG"

test "$(grep -Fc 'RESULT H2-STRUCTURE CONFIRMED' "$LOG")" -eq 4
test "$(grep -Fc 'RESULT: GENUINE SELF-LOCATING SEARCH -> KEY RECOVERY -> FORGERY OK' "$LOG")" -ge 5
grep -Fq 'model 2^B/(T+1)' "$LOG"
grep -Fq 'key recovery from ONE signature = 2^lambda/(T+1) TCCR evals' "$LOG"

echo 'ATTACK sign-21-1 ReSolveD-alpha CONFIRMED scaled multi-target signing-key recovery and fresh-message forgery'
echo 'LIMITATION sign-21-1 the full 2^248.2-or-larger enumeration was not run'
