#!/bin/sh
set -eu

COMMIT=6756633dfa096e729aeeb4dfce281ad5f345091f
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d /tmp/ngcc-tsuov-impl.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

git init -q "$TMP/source"
git -C "$TMP/source" remote add origin https://github.com/acprk/ngcc-round1-cryptanalysis.git
if ! git -C "$TMP/source" fetch -q --depth=1 origin "$COMMIT"; then
    echo 'SKIP sign-31-3/sign-31-4/sign-31-5/sign-31-6: pinned external artifact is unavailable' >&2
    exit 77
fi
git -C "$TMP/source" checkout -q --detach FETCH_HEAD
test "$(git -C "$TMP/source" rev-parse HEAD)" = "$COMMIT"
POC="$TMP/source/tsuov-impl-failopen"
LOG="$TMP/run.log"

REF="$HERE" "$POC/run_all.sh" | tee "$LOG"

test "$(grep -Fc 'RLIMIT= 0   (0 = ACCEPT)' "$LOG")" -eq 6
test "$(grep -Fl 'no RL = -1' "$POC"/logs/*_failopen_rl.log | wc -l)" -eq 6
test "$(grep -Fl 'RLIMIT= 0' "$POC"/logs/*_failopen_rl.log | wc -l)" -eq 6
test "$(grep -Fl 'READ of size' "$POC"/logs/*_asan1.log | wc -l)" -eq 6
test "$(grep -Fl 'READ of size' "$POC"/logs/*_asan2.log | wc -l)" -eq 6
test "$(grep -Fl 'WRITE of size' "$POC"/logs/*_asan3.log | wc -l)" -eq 6
grep -Fq 'cross-verify (ref verifies opt output and vice versa)' "$LOG"
test "$(grep -Fc 'non-canonical (all-zeros->31 in sig AND pk) 8/8 accepted' "$LOG")" -eq 6
test "$(grep -El '^SUMMARY .* honest 8/8 sigNC 8/8 pkNC 8/8 .* trailing 8/8 short 8/8 pklen1 8/8$' "$POC"/logs/*_probe.log | wc -l)" -eq 6

echo 'FAIL-OPEN sign-31-3 TSUOV CONFIRMED allocation failure leaves a stale message representative and accepts an unrelated message'
echo 'MEMORY sign-31-4 TSUOV CONFIRMED ignored key/signature lengths cause out-of-bounds reads'
echo 'MEMORY sign-31-5 TSUOV CONFIRMED message-length addition overflow causes a heap-buffer write'
echo 'MALLEABILITY sign-31-6 TSUOV CONFIRMED noncanonical signature/public-key encodings are accepted'
