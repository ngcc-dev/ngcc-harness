#!/bin/sh
# kem-28-2: OAEP-NTRU decapsulation returns the confirmation-tag difference,
# not a validity bit, and writes the shared secret before rejecting.
#
# At each reference level, flips each of the eight bits of one tag byte of an
# honest ciphertext. Requires kem_dec to return exactly that bit (1, 2, ...,
# 64, -128) and to leave the honest shared secret in the output buffer every
# time; the unmodified ciphertext is the control and must return 0. Also checks
# the same three source lines in the optimized and additional trees.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-28-2: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-28-2: $1"; exit 1; }

impl=kem-28/Implementations
for tree in Reference Optimized Additional; do
    for n in 648 1296 2592; do
        f=$impl/${tree}_Implementation/OAEP-NTRU-$n/KEM_AlgorithmInstance.c
        grep -q 'fail |= (ct\[i\] ^ sigma\[i - NTRUOAEP_POLYBYTES\]);' "$f" &&
            grep -q 'memcpy(ss, sigma + NTRUOAEP_SYMBYTES, NTRUOAEP_SSBYTES);' "$f" &&
            grep -q 'int8_t fail = 0;' "$f" || bad "$tree OAEP-NTRU-$n differs from the archived decapsulation"
    done
done
echo 'source: all nine trees OR the tag difference into an int8_t, copy the key unconditionally and return the accumulator'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
driver=$PWD/kem-28/reproduce_tag_return.c
for n in 648 1296 2592; do
    dir=$impl/Reference_Implementation/OAEP-NTRU-$n
    srcs=$(ls "$dir"/*.c | grep -v -e KAT_KEM.c -e test -e speed -e bench)
    gcc -O2 -w -I"$dir" -o "$scratch/t$n" "$driver" $srcs 2>"$scratch/cc" ||
        { cat "$scratch/cc"; bad "OAEP-NTRU-$n build failed"; }
    out=$("$scratch/t$n") || bad "OAEP-NTRU-$n run failed"
    echo "OAEP-NTRU-$n: $out"
    [ "$out" = 'valid_rc=0 plane_returns=8 key_written=8' ] || bad "OAEP-NTRU-$n behaviour differs"
done
echo 'CONFIRMED kem-28-2: at all three levels each flipped tag bit is returned as its own bit of the return value, and the rejected call still writes the honest shared secret'
exit 0
