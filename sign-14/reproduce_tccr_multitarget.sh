#!/bin/sh
set -eu

COMMIT=fffffb85983e71188430446f526ede3a211aded0
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d /tmp/ngcc-lynxer-tccr.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

git init -q "$TMP/source"
git -C "$TMP/source" remote add origin https://github.com/acprk/ngcc-round1-cryptanalysis.git
if ! git -C "$TMP/source" fetch -q --depth=1 origin "$COMMIT"; then
    echo 'SKIP sign-14-2: pinned external artifact is unavailable' >&2
    exit 77
fi
git -C "$TMP/source" checkout -q --detach FETCH_HEAD
test "$(git -C "$TMP/source" rev-parse HEAD)" = "$COMMIT"
POC="$TMP/source/lynxer-bavc-tccr-multitarget"
LOG="$TMP/run.log"

echo 'Running pinned Lynxer replay; output follows after completion.'
REFROOT="$HERE/Implementations/Reference_Implementation" \
    "$POC/run_all.sh" "${SEARCH_BITS:-20}" > "$LOG" 2>&1
tail -n 45 "$LOG"

test "$(grep -Fc 'bad=0' "$LOG")" -ge 4
grep -Fq '[key-recovery] recovered k == secret OWF key? YES' "$LOG"
grep -Fq '[FORGERY] fresh message signed with RECOVERED key, UNMODIFIED verify: ACCEPT' "$LOG"
grep -Fq '256 ~2^248.2, 384 ~2^375.6, 512 ~2^503.2' "$LOG"
python3 "$HERE/reproduce_tccr_image_bounds.py"

echo 'ATTACK sign-14-2 Lynxer CONFIRMED shared-tweak tree-node counting bound'
echo 'ATTACK sign-14-3 Lynxer CONFIRMED scaled hidden-leaf recovery, fresh-message forgery, and multi-target counting bound'
echo 'LIMITATION sign-14-2/sign-14-3 the full exponential enumerations were not run'
