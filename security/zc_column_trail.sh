#!/bin/sh
# hash-30-2 / hash-31-3: 8-column L-fixed trail in ZC-1536.
# Builds security/zc_column_trail.c against the unmodified ZC-DM reference
# permutation (identical in ZC-DMC), checks that L fixes the trail patterns,
# measures the one-round D->D rate at every start round, finds fresh
# three-round collisions of ZC-DM/ZC-DMC-1536-512/-768 from the zero IV, and
# replays two stored four-round collisions.  All collisions are checked with
# the independent Python mode model.  Runs in about 10 seconds.
set -eu
cd "$(dirname "$0")/.."
command -v cc >/dev/null 2>&1 || { echo 'SKIP hash-30-2/hash-31-3: cc unavailable'; exit 77; }
tmp=$(mktemp -d "${TMPDIR:-/tmp}/ngcc-zc.XXXXXX") || exit 77
trap 'rm -rf "$tmp"' EXIT
P=hash-30/ZC-DM/Implementations/lib/low/ZuD-1536/plain
cmp -s $P/ZuD1536-plain.c hash-31/ZC-DMC/Implementations/lib/low/ZuD-1536/plain/ZuD1536-plain.c ||
    { echo 'NOT CONFIRMED: ZC-DM and ZC-DMC permutations differ'; exit 1; }
cc -O2 -w -I$P -Ihash-30/ZC-DM/Implementations/lib/common -o "$tmp/zt" security/zc_column_trail.c -lm
"$tmp/zt" check
"$tmp/zt" rate 32 22 1
: > "$tmp/r3.txt"
for mn in "DM 512" "DMC 512" "DM 768" "DMC 768"; do
    echo "$mn 3" >> "$tmp/r3.txt"
    # shellcheck disable=SC2086
    "$tmp/zt" $mn 3 32 1 30 2>/dev/null >> "$tmp/r3.txt"
done
cat "$tmp/r3.txt" security/zc_column_trail_r4.txt | python3 security/zc_column_trail_verify.py || {
    echo 'NOT CONFIRMED hash-30-2/hash-31-3'; exit 1; }
echo 'CONFIRMED hash-30-2/hash-31-3: L-fixed trail and reduced-round collisions (full 12 rounds not attacked)'
