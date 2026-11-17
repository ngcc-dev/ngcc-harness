#!/bin/sh
# hash-05-5: uHash returns success without writing a digest for any
# digest_len_bits other than 512, 768 or 1024.
#
# Builds the three reference instance directories with a sentinel-buffer
# driver and requests 256 bits and 8 bits. Each call must return 0 and leave
# the output buffer untouched. Control: a 512-bit request writes 64 bytes.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-05-5: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED hash-05-5: $1"; exit 1; }
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
n=0
for bits in 512 768 1024; do
    d=$(find hash-05/uHash/Implementations -type d -path "*Reference_Implementation/uHash-$bits" | head -n 1)
    [ -n "$d" ] || bad "reference uHash-$bits not found"
    grep -q 'if (digest_len_bits == 1024)' "$d/CryptHash_AlgorithmInstance.c" || bad "uHash-$bits wrapper differs from the archive"
    gcc -O2 -w -I"$d" -o "$scratch/t" "$here/reproduce_digest_length.c" \
        "$d/CryptHash_AlgorithmInstance.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "uHash-$bits build failed"; }
    for req in 256 8; do
        out=$("$scratch/t" 512 "$req") || bad "uHash-$bits run failed"
        [ "$out" = "requested=$req rc=0 written_bytes=0 equals_full=0" ] || bad "uHash-$bits: $out"
    done
    out=$("$scratch/t" 512 512) || bad "uHash-$bits control failed"
    [ "$out" = "requested=512 rc=0 written_bytes=64 equals_full=1" ] || bad "uHash-$bits control: $out"
    n=$((n + 1))
done
echo "all $n reference directories return 0 and write nothing for 256- and 8-bit requests; a 512-bit request writes 64 bytes"
echo "ATTACK hash-05-5: uHash returns success without writing a digest for unsupported lengths: CONFIRMED"
exit 0
