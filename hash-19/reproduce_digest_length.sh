#!/bin/sh
# hash-19-3: the fixed-output MoFang wrappers ignore digest_len_bits.
#
# Builds every MoFang-256/-512/-768/-1024 tree (reference and optimized) with
# a sentinel-buffer driver and requests half the digest length and 8 bits.
# Each call must return 0 and write the full 32/64/96/128-byte digest.
# Control: the MoFang-256-XOF and MoFang-768-XOF trees honour the requested
# length (1 byte for an 8-bit request).
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-19-3: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED hash-19-3: $1"; exit 1; }
base='hash-19/MoFang/Implementations'
scratch=$(mktemp -d) || exit 2
trap 'rm -f -- "$scratch"/*; rmdir -- "$scratch"' EXIT HUP INT TERM
n=0
for kind in Reference Optimized; do
    for bits in 256 512 768 1024; do
        d="$base/${kind}_Implementation/MoFang-$bits"
        gcc -O2 -w -I"$d" -o "$scratch/t" "$here/reproduce_digest_length.c" \
            "$d/CryptHash_AlgorithmInstance.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$d build failed"; }
        for req in $((bits / 2)) 8; do
            out=$("$scratch/t" "$bits" "$req") || bad "$d run failed"
            [ "$out" = "requested=$req rc=0 written_bytes=$((bits / 8)) equals_full=1" ] ||
                bad "$kind MoFang-$bits: $out"
        done
        n=$((n + 1))
    done
done
echo "fixed-output: all $n trees return 0 for half-length and 8-bit requests and write the full digest"
for kind in Reference Optimized; do
    for bits in 256 768; do
        d="$base/${kind}_Implementation/MoFang-$bits-XOF"
        gcc -O2 -w -I"$d" -o "$scratch/t" "$here/reproduce_digest_length.c" \
            "$d/CryptHash_AlgorithmInstance.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$d build failed"; }
        out=$("$scratch/t" "$bits" 8) || bad "$d run failed"
        [ "$out" = "requested=8 rc=0 written_bytes=1 equals_full=0" ] || bad "$kind MoFang-$bits-XOF control: $out"
    done
done
echo 'control: the four XOF trees write exactly the 1 requested byte'
echo "ATTACK hash-19-3: all $n fixed-output MoFang wrappers ignore digest_len_bits and write the full digest: CONFIRMED"
exit 0
