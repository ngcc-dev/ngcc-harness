#!/bin/sh
# hash-07-2: the fixed-output Dragon wrappers ignore digest_len_bits.
#
# Builds every Dragon-512/-768/-1024 tree (reference, optimized, additional;
# x86 and ARM directories) with a sentinel-buffer driver and requests 256 bits
# and 8 bits. Each call must return 0 and write the full 64/96/128-byte digest.
# Control: the Dragon-XOF trees honour the requested length.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-07-2: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED hash-07-2: $1"; exit 1; }
base='hash-07/Dragon/Implementations and Test_vector'
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
n=0
for tree in Dragon-x86 Dragon-ARM; do
    for kind in Reference Optimized Additional; do
        for bits in 512 768 1024; do
            d="$base/$tree/Implementations/${kind}_Implementation/Dragon-$bits"
            gcc -O2 -w -I"$d" -o "$scratch/t" "$here/reproduce_digest_length.c" \
                "$d/CryptHash_AlgorithmInstance.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$d build failed"; }
            for req in 256 8; do
                out=$("$scratch/t" "$bits" "$req") || bad "$d run failed"
                [ "$out" = "requested=$req rc=0 written_bytes=$((bits / 8)) equals_full=1" ] ||
                    bad "$tree $kind Dragon-$bits: $out"
            done
            n=$((n + 1))
        done
    done
done
echo "fixed-output: all $n trees return 0 for 256- and 8-bit requests and write the full digest"
for tree in Dragon-x86 Dragon-ARM; do
    for lvl in 256 384 512; do
        d="$base/$tree/Implementations/Reference_Implementation/Dragon-XOF-$lvl"
        gcc -O2 -w -I"$d" -o "$scratch/t" "$here/reproduce_digest_length.c" \
            "$d/CryptHash_AlgorithmInstance.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$d build failed"; }
        out=$("$scratch/t" 1024 256) || bad "$d run failed"
        [ "$out" = "requested=256 rc=0 written_bytes=32 equals_full=0" ] || bad "$tree Dragon-XOF-$lvl control: $out"
    done
done
echo 'control: the six XOF trees write exactly the 32 requested bytes'
echo "ATTACK hash-07-2: all $n fixed-output Dragon wrappers ignore digest_len_bits and write the full digest: CONFIRMED"
exit 0
