#!/bin/sh
# hash-14-2: the fixed-output Laurus-512/-768/-1024 wrappers ignore digest_len_bits.
#
# Builds every fixed-output Laurus tree that exports CryptHash (the reference
# implementation; the optimized directories have no CryptHash wrapper) with a
# sentinel-buffer driver and requests 256 bits and 8 bits. Each call must
# return 0 and write the full 64/96/128-byte digest. (The Laurus-XOF wrapper
# is the separate, already-filed hash-14-2.)
# Control: a full-length request writes exactly the full digest.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-14-2: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED hash-14-2: $1"; exit 1; }
base='hash-14/Laurus/Implementations/Reference_Implementation'
scratch=$(mktemp -d) || exit 2
trap 'rm -f -- "$scratch"/*; rmdir -- "$scratch"' EXIT HUP INT TERM
n=0
for bits in 512 768 1024; do
    d="$base/Laurus-$bits"
    gcc -O2 -w -I"$d" -o "$scratch/t" "$here/reproduce_digest_length.c" \
        "$d/CryptHash_AlgorithmInstance.c" "$d/laurus_$bits.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$d build failed"; }
    for req in 256 8; do
        out=$("$scratch/t" "$bits" "$req") || bad "$d run failed"
        [ "$out" = "requested=$req rc=0 written_bytes=$((bits / 8)) equals_full=1" ] ||
            bad "Laurus-$bits: $out"
    done
    out=$("$scratch/t" "$bits" "$bits") || bad "$d run failed"
    [ "$out" = "requested=$bits rc=0 written_bytes=$((bits / 8)) equals_full=1" ] ||
        bad "Laurus-$bits control: $out"
    n=$((n + 1))
done
echo "fixed-output: all $n trees return 0 for 256- and 8-bit requests and write the full digest"
echo 'control: full-length requests write exactly the full digest'
echo "ATTACK hash-14-2: all $n fixed-output Laurus wrappers ignore digest_len_bits and write the full digest: CONFIRMED"
exit 0
