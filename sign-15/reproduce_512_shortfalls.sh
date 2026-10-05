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

grep -Eq '^ATLAS-512[[:space:]]+512[[:space:]]+512[[:space:]]+60/60[[:space:]]+2\^[[:space:]]+322\.67[[:space:]]+2\^[[:space:]]+277\.45' "$LOG"
grep -Fq 'smallest kappa with C(512,kappa)*2^kappa >= 2^512: kappa = 118 (2^512.18)' "$LOG"
test "$(grep -Ec '^(  Ref|  Opt)  ATLAS-512 .*non-zeros at positions \[256,452\): 0 .*positions ever hit 315/512' "$LOG")" -eq 2
for level in 256 512; do
    grep -Fq "ATLAS-$level, mu truncated to 5 bytes: collision" "$LOG"
    grep -Fq "ATLAS-$level: one sig_keygen draws 256 DRNG bits in 1 call(s)" "$LOG"
done
test "$(grep -Fc 'signature on M1: verify(M1)=0  verify(M2, never signed)=0  control verify(M3)=-1' "$LOG")" -eq 2
test "$(grep -Fc 'same 256-bit draw, different DRNG states: pk identical, sk identical' "$LOG")" -eq 2
test "$(grep -Fc 'signature made with the recovered key verifies under the victim pk: yes' "$LOG")" -eq 2
test "$(grep -Fc 'recovered sk == victim sk [SCORING]: yes' "$LOG")" -eq 2
for tree in Ref Opt; do
    grep -Fq "$tree  ATLAS-128 honest verify=0; unused sign bits 31..63: 33/33 flips give a DIFFERENT byte string that verifies; control (flip used sign bit 0) verify=-1" "$LOG"
    for level in 256 512; do
        grep -Fq "$tree  ATLAS-$level honest verify=0; unused sign bits 60..63: 4/4 flips give a DIFFERENT byte string that verifies; control (flip used sign bit 0) verify=-1" "$LOG"
    done
done

echo 'ATTACK sign-15-8 ATLAS-512 CONFIRMED challenge space below the 512-bit target'
echo 'ATTACK sign-15-9 ATLAS-256/512 CONFIRMED scaled collision-transfer forgery from the 384-bit unsalted message representative'
echo 'ATTACK sign-15-10 ATLAS-512 CONFIRMED scaled public-key search over the 256-bit key-generation seed'
echo 'MALLEABILITY sign-15-11 CONFIRMED unused challenge-sign bits give distinct accepted signatures'
echo 'ATTACK sign-15-12 ATLAS-512 CONFIRMED byte-wide position selection caps the challenge image at 2^277.45'
echo 'LIMITATION sign-15-8/sign-15-9/sign-15-10/sign-15-12 full-width searches were not run; reported costs are counting bounds backed by scaled experiments'
