#!/bin/sh
# sign-15-16: ATLAS-128 and -192 sample secrets from a narrower, shifted range
# than the specified uniform [-eta, eta].
#
# Builds each unmodified reference set with this repository's API shim,
# generates one key and prints the observed s1 coefficient range against the
# specified eta (Table 2: 16, 8, 4, 4).
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-15-16: gcc unavailable'; exit 77; }
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
res=
for l in 128 192 256 512; do
    src=sign-15/Implementation/Reference_Implementation/lwrdsa$l
    gcc -O1 -std=gnu11 -D_GNU_SOURCE -DNGCC_BUILD_SIG '-DNGCC_ID="sign-15"' \
        '-DNGCC_ALG="ATLAS"' '-DNGCC_VARIANT="ref"' -I "$src" -I api \
        -o "$scratch/s$l" sign-15/reproduce_secret_range.c \
        $(ls "$src"/*.c | grep -v KAT_SIG.c) api/link_shim.c -lm 2>"$scratch/cc$l" ||
        { cat "$scratch/cc$l"; echo "NOT CONFIRMED sign-15-16: ATLAS-$l build failed"; exit 1; }
    out=$("$scratch/s$l") || exit 1
    echo "ATLAS-$l: $out"
    res="$res $(printf '%s' "$out" | sed -n 's/.*range \[\(-*[0-9]*\),\(-*[0-9]*\)\].*/\1,\2/p')"
done
set -- $res
if [ "$1" = 1,16 ] && [ "$2" = -7,8 ] && [ "$3" = -4,4 ] && [ "$4" = -4,4 ]; then
    echo 'CONFIRMED sign-15-16: s1 lies in [1,16] (ATLAS-128) and [-7,8] (ATLAS-192) instead of uniform [-eta,eta]; ATLAS-256 and -512 match'
    exit 0
fi
echo 'NOT CONFIRMED sign-15-16: observed ranges differ from the report'
exit 1
