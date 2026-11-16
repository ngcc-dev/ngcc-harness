#!/bin/sh
# kem-36-11: TRIKE's kem_dec ignores ct_len_bytes and copies a fixed-size
# ciphertext_t out of the caller's buffer. Builds each of the eight trees with
# AddressSanitizer and passes a heap buffer of exactly ct_len_bytes = |ct| - 1
# bytes; ASan must report a read past that 1-byte-short region in kem_dec.
# Control: the same call with a full-length buffer is clean and decapsulates.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "${NGCC_ROOT:-$here/..}" || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-36-11: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-36-11: $1"; exit 1; }
impl='kem-36/Implementations and Test_Vectors/Implementations'
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
for d in "$impl"/Reference_Implementation/TRIKE-* "$impl"/Optimized_Implementation/TRIKE-*; do
    name="$(basename "$(dirname "$d")" | cut -c1-3)-$(basename "$d")"
    grep -Fq '	ciphertext_t *local_ct = (ciphertext_t *)ct;' "$d/src/KEM_AlgorithmInstance.c" || bad "$name differs"
    grep -Eq '(if|while|return).*[^*]ct_len_bytes' "$d/src/KEM_AlgorithmInstance.c" && bad "$name checks ct_len_bytes"
    (cd "$d" && gcc -O0 -g -std=gnu11 -w -fsanitize=address -IICCS -Isrc -o "$scratch/$name" \
        "$here/reproduce_short_ciphertext.c" ICCS/auxfunc.c ICCS/drng.c src/KEM_AlgorithmInstance.c \
        src/decoder.c src/gf2x.c src/sample.c -lm) 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$name build failed"; }
    ctl=$(ASAN_OPTIONS=detect_leaks=0 "$scratch/$name" 2>&1) || bad "$name control failed"
    printf '%s\n' "$ctl" | grep -q 'shared secrets match=1' || bad "$name control did not decapsulate"
    out=$(ASAN_OPTIONS=detect_leaks=0 "$scratch/$name" short 2>&1) && bad "$name short buffer ran clean"
    printf '%s\n' "$out" | grep -q 'ERROR: AddressSanitizer' &&
        printf '%s\n' "$out" | grep -q 'in kem_dec .*KEM_AlgorithmInstance.c' &&
        printf '%s\n' "$out" | grep -Eq 'located 0 bytes after [0-9]+-byte region' || bad "$name: no ASan over-read in kem_dec"
    echo "$name: control ok; $(printf '%s\n' "$out" | grep -m1 -o 'READ of size [0-9]*'), $(printf '%s\n' "$out" | grep -m1 -Eo 'located 0 bytes after [0-9]+-byte region')"
done
echo 'ATTACK kem-36-11: in all eight trees kem_dec reads past a heap ciphertext buffer of the declared length CONFIRMED'
exit 0
