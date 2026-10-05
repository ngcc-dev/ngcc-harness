#!/bin/sh
set -eu

COMMIT=a3f0a6adb6b695922247d4b771f4deb73777a619
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d /tmp/ngcc-nss-hqc-seedy.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

git init -q "$TMP/source"
git -C "$TMP/source" remote add origin https://github.com/acprk/ngcc-round1-cryptanalysis.git
if ! git -C "$TMP/source" fetch -q --depth=1 origin "$COMMIT"; then
    echo 'SKIP kem-26-4: pinned external artifact is unavailable' >&2
    exit 77
fi
git -C "$TMP/source" checkout -q --detach FETCH_HEAD
test "$(git -C "$TMP/source" rev-parse HEAD)" = "$COMMIT"
POC="$TMP/source/nss-hqc-seedy-width"
LOG="$TMP/run.log"

echo 'Running pinned NSS-HQC full-size and scaled checks.'
REF="$HERE/Implementations and Test_Vectors/Implementations/Reference_Implementation" \
    "$POC/run_all.sh" > "$LOG" 2>&1
tail -n 45 "$LOG"

grep -Fq 'HQC-384  |seed_sk|=384 bit  |seed_y|=256 bit' "$LOG"
grep -Fq 'HQC-512  |seed_sk|=512 bit  |seed_y|=256 bit' "$LOG"
test "$(grep -Fc 'y rebuilt from seed_y alone: public test PASS, decode=0, ss(y-only)==encaps ss: YES, ==victim decaps: YES' "$LOG")" -eq 4
test "$(grep -Fc 'control: 32 random seed_y candidates, 0 pass the public test' "$LOG")" -eq 4
test "$(grep -Fc 'recovered ss==encaps ss: YES  ==victim decaps: YES' "$LOG")" -eq 2
test "$(grep -Fc 'search over 2^8: NOT FOUND' "$LOG")" -eq 2

echo 'ATTACK kem-26-4 NSS-HQC CONFIRMED the 384/512 decryption capability is determined by a 256-bit sub-seed'
echo 'LIMITATION kem-26-4 the full 2^256 enumeration was not executed; an 8-bit scale model exercises the complete recovery path'
