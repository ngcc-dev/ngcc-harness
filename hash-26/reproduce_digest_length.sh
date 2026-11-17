#!/bin/sh
# hash-26-2: the reference CHIME wrappers ignore digest_len_bits.
#
# Builds the CHIME-512 and CHIME-1024 reference trees with a sentinel-buffer
# driver and requests 256 bits and 8 bits. Each call must return 0 and write
# the full 64/128-byte digest.
# Control: the optimized trees (C++, built when g++ is present) reject both
# requests with -1 and write nothing.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-26-2: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED hash-26-2: $1"; exit 1; }
base='hash-26/CHIME/Implementations'
scratch=$(mktemp -d) || exit 2
trap 'rm -f -- "$scratch"/*; rmdir -- "$scratch"' EXIT HUP INT TERM
n=0
for bits in 512 1024; do
    d="$base/Reference_Implementation/CHIME-$bits"
    gcc -O2 -w -I"$d" -o "$scratch/t" "$here/reproduce_digest_length.c" \
        "$d/CryptHash_AlgorithmInstance.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$d build failed"; }
    for req in 256 8; do
        out=$("$scratch/t" "$bits" "$req") || bad "$d run failed"
        [ "$out" = "requested=$req rc=0 written_bytes=$((bits / 8)) equals_full=1" ] ||
            bad "reference CHIME-$bits: $out"
    done
    n=$((n + 1))
done
echo "reference: both trees return 0 for 256- and 8-bit requests and write the full digest"
if command -v g++ >/dev/null 2>&1; then
    for bits in 512 1024; do
        d="$base/Optimized_Implementation/CHIME-$bits"
        gcc -O2 -w -c -o "$scratch/drv.o" "$here/reproduce_digest_length.c" &&
        g++ -O2 -w -march=native -std=c++17 -I"$d" -o "$scratch/t" "$scratch/drv.o" \
            -x c++ "$d/CryptHash_AlgorithmInstance.c" -x none "$d/P.cpp" "$d/chime$bits.cpp" \
            2>"$scratch/cc" || { cat "$scratch/cc"; bad "$d build failed"; }
        for req in 256 8; do
            out=$("$scratch/t" "$bits" "$req") || bad "$d run failed"
            [ "$out" = "requested=$req rc=-1 written_bytes=0 equals_full=0" ] || bad "optimized CHIME-$bits control: $out"
        done
    done
    echo 'control: the two optimized trees return -1 and write nothing'
else
    echo 'control skipped: g++ unavailable for the C++ optimized trees'
fi
echo "ATTACK hash-26-2: both reference CHIME wrappers ignore digest_len_bits and write the full digest: CONFIRMED"
exit 0
