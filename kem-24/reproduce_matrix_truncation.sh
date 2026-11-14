#!/bin/sh
# kem-24-4: Scabbard-128 truncates three of every four public-matrix
# coefficients to eight bits.
#
# Checks the unpacker in the reference, optimized and Cortex-M4 Scabbard-128
# trees, then builds the archived gen_a for all three reference levels and
# compares every coefficient of eight matrices with the specification's
# BytesToPoly applied to the same pseudoXOF output. Scabbard-128 must disagree
# and keep positions 1-3 below 256; Scabbard-256/512 are controls and must
# agree exactly.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-24-4: gcc unavailable'; exit 77; }

impl=kem-24/Implementations
for f in Reference_Implementation/scabbard128 Optimized_Implementation/scabbard128 \
         Additional_Implementations/Cortex_M4/scabbard/scabbard128; do
    grep -q '(((buf\[7 \* i + 2\] & 0xff) << 2) & 0xff)' "$impl/$f/poly.c" &&
        grep -q '(((buf\[7 \* i + 6\] & 0xff) << 6) & 0xff)' "$impl/$f/poly.c" ||
        { echo "NOT CONFIRMED kem-24-4: $f/poly.c differs from the archived unpacker"; exit 1; }
done
echo 'source: reference, optimized and Cortex-M4 scabbard128 poly_modq_frombytes mask shifted bytes with 0xff'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
driver=$PWD/kem-24/reproduce_matrix_truncation.c
for level in 128 256 512; do
    (cd "$impl/Reference_Implementation/scabbard$level" &&
        gcc -O2 -w -I. -o "$scratch/a$level" auxfunc.c cbd.c drng.c minal.c poly.c polyvec.c "$driver" -lcrypto) \
        2>"$scratch/cc" || { cat "$scratch/cc"; echo "NOT CONFIRMED kem-24-4: scabbard$level build failed"; exit 1; }
    out=$("$scratch/a$level") || { echo "NOT CONFIRMED kem-24-4: scabbard$level run failed"; exit 1; }
    echo "scabbard$level gen_a: $out"
    eval "r$level=\$out"
done
echo "$r256" | grep -q ' mismatches=0 ' && echo "$r512" | grep -q ' mismatches=0 ' ||
    { echo 'NOT CONFIRMED kem-24-4: Scabbard-256/512 controls do not match BytesToPoly'; exit 1; }
max=${r128##*max_by_index_mod4=}
p0=${max%%,*}; rest=${max#*,}; p1=${rest%%,*}; rest=${rest#*,}; p2=${rest%%,*}; p3=${rest#*,}
mis=${r128#* mismatches=}; mis=${mis%% *}
[ "$mis" -gt 0 ] && [ "$p0" -gt 255 ] && [ "$p1" -le 255 ] && [ "$p2" -le 255 ] && [ "$p3" -le 255 ] ||
    { echo 'NOT CONFIRMED kem-24-4: Scabbard-128 coefficients are not truncated as expected'; exit 1; }
echo "CONFIRMED kem-24-4: Scabbard-128 gen_a differs from BytesToPoly in $mis coefficients; positions 1-3 mod 4 never exceed 255 (max $p1, $p2, $p3) while position 0 reaches $p0; Scabbard-256/512 match exactly"
exit 0
