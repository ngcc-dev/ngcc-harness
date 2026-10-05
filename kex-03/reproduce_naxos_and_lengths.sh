#!/bin/sh
set -eu

COMMIT=996e9e01bb71210b4debd44e565f543833d0d135
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d /tmp/ngcc-cretake-naxos.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

git init -q "$TMP/source"
git -C "$TMP/source" remote add origin https://github.com/acprk/ngcc-round1-cryptanalysis.git
if ! git -C "$TMP/source" fetch -q --depth=1 origin "$COMMIT"; then
    echo 'SKIP kex-03-4/kex-03-5: pinned external artifact is unavailable' >&2
    exit 77
fi
git -C "$TMP/source" checkout -q --detach FETCH_HEAD
test "$(git -C "$TMP/source" rev-parse HEAD)" = "$COMMIT"
POC="$TMP/source/cretake-naxos-binding"
LOG="$TMP/run.log"

REF="$HERE/Implementations" "$POC/run_all.sh" | tee "$LOG"

grep -Fq 'absorbed_is_public=YES' "$LOG"
test "$(grep -Ec '^\[Reference_Implementation\] RESULT .*staterev_key_recovered=20/20' "$LOG")" -eq 12
test "$(grep -Ec '^\[Reference_Implementation/fixed\] RESULT .*staterev_key_recovered=0/20' "$LOG")" -eq 4
test "$(grep -Ec '^\[Reference_Implementation/fix0301\] RESULT .*staterev_key_recovered=20/20' "$LOG")" -eq 3
test "$(grep -Ec -- '-asan].*READ of size [0-9]+' "$LOG")" -eq 150
! grep -Eq -- 'SURVIVED|-> ok' "$LOG"

echo 'ATTACK kex-03-4 CreTAKE CONFIRMED state reveal plus public data recovers S2K/S2S session keys without search'
echo 'MEMORY kex-03-5 CreTAKE CONFIRMED ignored message lengths cause pre-authentication out-of-bounds reads'
