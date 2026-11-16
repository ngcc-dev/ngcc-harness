#!/bin/sh
# kem-40-7: yy_encapsulate_API ignores pk_len_bytes and decodes the public key
# at its fixed length, so a heap buffer of the declared length is overread.
#
# Builds every reference and optimized instance with AddressSanitizer. A
# full-length heap key must encapsulate cleanly; a key one byte short, in a
# heap buffer of exactly that length, must trigger a heap-buffer-overflow read
# in yuanyang_decode_uniform called from yy_encapsulate_API.
set -u
here=$(cd "$(dirname "$0")" && pwd)
[ -d kem-40/Implementations ] || cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-40-7: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-40-7: $1"; exit 1; }
ulimit -s unlimited 2>/dev/null || ulimit -s "$(ulimit -H -s)" 2>/dev/null
ASAN_OPTIONS=detect_leaks=0; export ASAN_OPTIONS

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
echo 'int main(void){return 0;}' >"$scratch/p.c"
gcc -fsanitize=address -o "$scratch/p" "$scratch/p.c" 2>/dev/null && "$scratch/p" ||
    { echo 'SKIP kem-40-7: AddressSanitizer unavailable'; exit 77; }
for tree in Reference Optimized; do
    for s in 512 1024 2048; do
        d=kem-40/Implementations/${tree}_Implementation/yuanyang-$s
        grep -Fq '	yuanyang_decode_uniform(h,YUANYANG_D,YUANYANG_Q,4,pk);' "$d/kem.c" ||
            bad "$tree yuanyang-$s differs from the archive"
        (cd "$d" && gcc -O1 -g -fsanitize=address -fno-omit-frame-pointer -w -I. -o "$scratch/t" \
            "$here/reproduce_short_pk.c" prng.c auxfunc.c drng.c ntt.c codec.c poly_inv_ntt.c \
            KEM_AlgorithmInstance.c kem.c kem-keygen.c -lm) 2>"$scratch/cc" ||
            { cat "$scratch/cc"; bad "$tree yuanyang-$s build failed"; }
        "$scratch/t" full >"$scratch/full" 2>&1 || { cat "$scratch/full"; bad "$tree yuanyang-$s control failed"; }
        "$scratch/t" short >"$scratch/short" 2>&1 && bad "$tree yuanyang-$s short key not detected"
        grep -q 'heap-buffer-overflow' "$scratch/short" &&
            grep -q 'in yuanyang_decode_uniform' "$scratch/short" &&
            grep -q 'in yy_encapsulate_API' "$scratch/short" ||
            { cat "$scratch/short"; bad "$tree yuanyang-$s report differs"; }
        echo "$tree yuanyang-$s: full key clean; $(grep -o 'located 0 bytes after [0-9]*-byte region' "$scratch/short")"
    done
done
echo 'ATTACK kem-40-7: in all six YuanYang builds a public key one byte short, in a heap buffer of the declared length, is read out of bounds by encapsulation: CONFIRMED'
exit 0
