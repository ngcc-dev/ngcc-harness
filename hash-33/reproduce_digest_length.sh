#!/bin/sh
# hash-33-2: the fixed-output Thunder wrappers ignore digest_len_bits.
#
# Builds every Thunder-512/-768/-1024 tree (reference, optimized, additional;
# X86 and ARM directories) with a sentinel-buffer driver and requests 256 bits
# and 8 bits. Each call must return 0 and write the full 64/96/128-byte digest.
# The X86 Additional Thunder-1024 directory does not compile as shipped (its
# header and source both define IV) and is skipped with a message.
# Control: the Thunder-XOF trees honour the requested length.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-33-2: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED hash-33-2: $1"; exit 1; }
base='hash-33/Thunder/Implementations and Test_vector'
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
n=0
for tree in Thunder-X86 Thunder-ARM; do
    for kind in Reference Optimized Additional; do
        for bits in 512 768 1024; do
            d="$base/$tree/Implementations/${kind}_Implementation/Thunder-$bits"
            if ! gcc -O2 -w -I"$d" -o "$scratch/t" "$here/reproduce_digest_length.c" \
                "$d/CryptHash_AlgorithmInstance.c" 2>"$scratch/cc"; then
                if [ "$tree $kind $bits" = 'Thunder-X86 Additional 1024' ] && grep -q 'redefinition of .IV' "$scratch/cc"; then
                    echo 'note: Thunder-X86 Additional Thunder-1024 does not compile as shipped (IV defined twice); skipped'
                    continue
                fi
                cat "$scratch/cc"; bad "$d build failed"
            fi
            for req in 256 8; do
                out=$("$scratch/t" "$bits" "$req") || bad "$d run failed"
                [ "$out" = "requested=$req rc=0 written_bytes=$((bits / 8)) equals_full=1" ] ||
                    bad "$tree $kind Thunder-$bits: $out"
            done
            n=$((n + 1))
        done
    done
done
echo "fixed-output: all $n compiling trees return 0 for 256- and 8-bit requests and write the full digest"
for tree in Thunder-X86 Thunder-ARM; do
    for lvl in 256 384 512; do
        d="$base/$tree/Implementations/Reference_Implementation/Thunder-XOF-$lvl"
        gcc -O2 -w -I"$d" -o "$scratch/t" "$here/reproduce_digest_length.c" \
            "$d/CryptHash_AlgorithmInstance.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$d build failed"; }
        out=$("$scratch/t" 1024 256) || bad "$d run failed"
        [ "$out" = "requested=256 rc=0 written_bytes=32 equals_full=0" ] || bad "$tree Thunder-XOF-$lvl control: $out"
    done
done
echo 'control: the six XOF trees write exactly the 32 requested bytes'
echo "ATTACK hash-33-2: all $n compiling fixed-output Thunder wrappers ignore digest_len_bits and write the full digest: CONFIRMED"
exit 0
