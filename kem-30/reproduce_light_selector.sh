#!/bin/sh
# kem-30-4: the reference POLARLAC-Light public-key decoder reads past its
# one-byte zero selector, and for the second polynomial past the key itself.
#
# Checks that the reference Light poly_decompress lacks the selector-capacity
# guard present in the reference POLARLAC-128 decoder of the same format, then
# builds both with AddressSanitizer. An honest key whose second polynomial is
# given 9 or 40 zero low bytes, passed to kem_enc in a heap buffer of exactly
# 530 bytes, must give a heap-buffer-overflow in poly_decompress 0 bytes after
# the key. Controls: the unmodified key, and 40 zero bytes in the first
# polynomial (whose extra selector reads stay inside the key), run cleanly;
# POLARLAC-128 runs cleanly on the same inputs.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
id=kem-30-4
command -v gcc >/dev/null 2>&1 || { echo "SKIP $id: gcc unavailable"; exit 77; }
bad() { echo "NOT CONFIRMED $id: $1"; exit 1; }

ref=kem-30/Implementations/Reference_Implementation/x86
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
ASAN_OPTIONS=detect_leaks=0
export ASAN_OPTIONS
printf 'int main(void){return 0;}\n' >"$scratch/t.c"
gcc -fsanitize=address -o "$scratch/t" "$scratch/t.c" >/dev/null 2>&1 && "$scratch/t" ||
    { echo "SKIP $id: AddressSanitizer unavailable"; exit 77; }

decoder() { awk '/^void poly_decompress\(int16_t \*c, const uint8_t \*code_c\)/{f=1} f{print} f&&/^}/{exit}' "$1"; }
guard='if (candidates < PK_ZERO_SELECTOR_BITS) {'
decoder "$ref/POLARLAC-Light/poly.c" | grep -Fq 'uint16_t sel = (selector[byte_idx] >> bit_pos) & 1U;' &&
    ! decoder "$ref/POLARLAC-Light/poly.c" | grep -Fq "$guard" &&
    grep -Fq 'if (candidates >= PK_ZERO_SELECTOR_BITS) {' "$ref/POLARLAC-Light/poly.c" &&
    grep -Fqx '#define PK_ZERO_SELECTOR_BYTES 1' "$ref/POLARLAC-Light/params.h" ||
    bad "reference POLARLAC-Light decoder differs from the archive"
decoder "$ref/POLARLAC-128/poly.c" | grep -Fq "$guard" || bad "reference POLARLAC-128 decoder lacks the guard"
echo 'source: the Light encoder refuses a ninth zero, but the Light decoder reads selector[candidates >> 3] unguarded; POLARLAC-128 guards it'

for v in Light 128; do
    (cd "$ref/POLARLAC-$v" && gcc -O1 -g -fsanitize=address -fno-omit-frame-pointer -std=c99 -D_DEFAULT_SOURCE -w \
        -DBIT_USE_SHAKE=0 -DRL_KEM_USE_CONJ_NTT_REJECTION=0 -I. -o "$scratch/$v" "$here/reproduce_light_selector.c" \
        auxfunc.c drng.c fips202.c symmetric.c sample.c poly.c pke.c polar.c ntt.c fft.c KEM_AlgorithmInstance.c -lm) \
        2>"$scratch/cc" || { cat "$scratch/cc"; bad "POLARLAC-$v build failed"; }
done
clean() { out=$("$scratch/$1" $2 $3 2>&1) && case "$out" in *"enc=0") return 0 ;; esac; echo "$out"; return 1; }
for z in 9 40; do
    "$scratch/Light" 1 $z >"$scratch/out" 2>&1 && { cat "$scratch/out"; bad "Light: no fault with $z zeros"; }
    grep -q 'ERROR: AddressSanitizer: heap-buffer-overflow' "$scratch/out" &&
        grep -q 'is located 0 bytes after 530-byte region' "$scratch/out" &&
        grep -q 'in poly_decompress .*poly.c' "$scratch/out" && grep -q 'in kem_enc ' "$scratch/out" ||
        { cat "$scratch/out"; bad "Light: no over-read past the key with $z zeros"; }
done
clean Light 1 0 && clean Light 0 40 || bad "Light controls not clean"
for z in 0 9 40; do clean 128 1 $z || bad "POLARLAC-128 not clean with $z zeros"; done
echo 'reference POLARLAC-Light: 9 and 40 zero low bytes in the second polynomial read past the 530-byte key in poly_decompress; the unmodified key and 40 zeros in the first polynomial run cleanly'
echo 'reference POLARLAC-128 [control]: the same inputs run cleanly'
echo "ATTACK $id: a correctly sized malformed public key makes reference POLARLAC-Light encapsulation read past the key: CONFIRMED"
exit 0
