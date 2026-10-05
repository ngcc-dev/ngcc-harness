#!/bin/sh
set -eu

COMMIT=5e9e7d7b325415cd355a81667f3bd825a0a8a935
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d /tmp/ngcc-atlas-512.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

git init -q "$TMP/source"
git -C "$TMP/source" remote add origin https://github.com/acprk/ngcc-round1-cryptanalysis.git
if ! git -C "$TMP/source" fetch -q --depth=1 origin "$COMMIT"; then
    echo 'SKIP sign-15-8/-9/-10/-11/-12: pinned external artifact is unavailable' >&2
    exit 77
fi
git -C "$TMP/source" checkout -q --detach FETCH_HEAD
test "$(git -C "$TMP/source" rev-parse HEAD)" = "$COMMIT"
POC="$TMP/source/morning-atlas-512-shortfalls"
LOG="$TMP/run.log"

echo 'Running pinned ATLAS checks; output follows after completion.'
REF="$HERE/Implementation" "$POC/run_all.sh" > "$LOG" 2>&1
tail -n 45 "$LOG"

grep -Fq 'ATLAS-512    512' "$LOG"
grep -Fq '2^    277.45' "$LOG"
test "$(grep -Fc 'verify(M2, never signed)=0' "$LOG")" -eq 2
grep -Fq 'ATLAS-512: one sig_keygen draws 256 DRNG bits' "$LOG"
grep -Fq 'signature made with the recovered key verifies under the victim pk: yes' "$LOG"
grep -Fq 'unused sign bits 60..63: 4/4 flips' "$LOG"

echo 'ATTACK sign-15-8 ATLAS-512 CONFIRMED challenge space below the 512-bit target'
echo 'ATTACK sign-15-9 ATLAS-256/512 CONFIRMED scaled collision-transfer forgery from the 384-bit unsalted message representative'
echo 'ATTACK sign-15-10 ATLAS-512 CONFIRMED scaled public-key search over the 256-bit key-generation seed'
echo 'MALLEABILITY sign-15-11 CONFIRMED unused challenge-sign bits give distinct accepted signatures'
echo 'ATTACK sign-15-12 ATLAS-512 CONFIRMED byte-wide position selection caps the challenge image at 2^277.45'
echo 'LIMITATION sign-15-8/sign-15-9/sign-15-10/sign-15-12 full-width searches were not run; reported costs are counting bounds backed by scaled experiments'
