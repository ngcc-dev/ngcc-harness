#!/bin/sh
# sign-15-14: sig_verify authenticates one message and returns another.
#
# Builds each unmodified ATLAS reference implementation with this repository's
# API shim. Checks the attached-mode forgery and its controls, then an honest
# detached verification whose signature ends at a guard page: the copy loop
# reads past the advertised signature and faults. The optimized trees contain
# the same code (checked at source level here).
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-15-14: gcc unavailable'; exit 77; }

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
for tree in Reference_Implementation Optimized_Implementation; do
    for l in 128 192 256 512; do
        f=sign-15/Implementation/$tree/lwrdsa$l/SIG_lwrdsa$l.c
        grep -q 'm\[i\] = sn\[CRYPTO_BYTES + i\];' "$f" && grep -q 'if(sn != m)' "$f" ||
            { echo "NOT CONFIRMED sign-15-14: $f differs"; exit 1; }
    done
done
echo 'source: all eight trees hash m only when sn != m and return sn[CRYPTO_BYTES..]'

ok=0
for l in 128 192 256 512; do
    src=sign-15/Implementation/Reference_Implementation/lwrdsa$l
    gcc -O1 -std=gnu11 -D_GNU_SOURCE \
        -DNGCC_BUILD_SIG '-DNGCC_ID="sign-15"' '-DNGCC_ALG="ATLAS"' '-DNGCC_VARIANT="ref"' \
        -I "$src" -I api -o "$scratch/v$l" sign-15/reproduce_verify_substitution.c \
        $(ls "$src"/*.c | grep -v KAT_SIG.c) api/link_shim.c -lm 2>"$scratch/cc$l" ||
        { cat "$scratch/cc$l"; echo "NOT CONFIRMED sign-15-14: ATLAS-$l build failed"; exit 1; }
    out=$("$scratch/v$l" forge 2>&1); st=$?
    echo "ATLAS-$l: $out"
    [ "$st" -eq 0 ] || continue
    "$scratch/v$l" detached >"$scratch/d$l" 2>&1; st=$?
    if [ "$st" -ge 128 ] && grep -q detached_verify_start "$scratch/d$l" &&
        ! grep -q 'no fault' "$scratch/d$l"; then
        echo "ATLAS-$l: honest detached verify read past the signature (signal $((st - 128)))"
        ok=$((ok + 1))
    else
        cat "$scratch/d$l"
    fi
done

if [ "$ok" -ne 4 ]; then
    echo "NOT CONFIRMED sign-15-14: $ok/4 reference sets behaved as reported"
    exit 1
fi
echo 'CONFIRMED sign-15-14: attached-mode verification accepts chosen messages under one signature on 0^32 (4/4 sets, controls reject); detached verification over-reads the signature buffer'
exit 0
