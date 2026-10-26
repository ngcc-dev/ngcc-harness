#!/bin/sh
# Order-2 cubic jet distinguisher for UVW public codes.
# Default: scaled model keys with twin-shape and random controls (seconds).
# NGCC_LONG=1: also a full-size public key from the frozen UVW-KEM-128
# key generation and a random control (about 1.3 GB and 30 s per count on
# 28 cores; much longer on fewer).
set -eu
cd "$(dirname "$0")/.."
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-38-8: gcc unavailable'; exit 77; }
tmp=$(mktemp -d)
trap 'rm -r "$tmp"' EXIT HUP INT TERM
omp=-fopenmp
echo 'int main(void){return 0;}' > "$tmp/t.c"
gcc -fopenmp "$tmp/t.c" -o "$tmp/t" 2>/dev/null || omp=
gcc -std=c11 -O3 -march=native $omp -o "$tmp/jet" kem-38/reproduce_jet_distinguisher.c

excess() { sed -n 's/.* excess=\(-*[0-9]*\)$/\1/p'; }
ok=1
for nk in "140 22" "200 26"; do
    set -- $nk
    for mode in 0 1 2; do
        line=$("$tmp/jet" "$1" "$2" "$mode" 1)
        echo "$line"
        e=$(echo "$line" | excess)
        if [ "$mode" -eq 0 ]; then [ "$e" -gt 0 ] || ok=0; else [ "$e" -eq 0 ] || ok=0; fi
    done
done
if [ "$ok" -ne 1 ]; then echo 'NOT REPRODUCED kem-38-8: scaled model counts'; exit 1; fi
echo 'DISTINGUISHER kem-38-8 (scaled): GRS-structured models exceed the generic count; twin-shape and random controls match it'

if [ "${NGCC_LONG:-0}" != 1 ]; then
    echo 'Set NGCC_LONG=1 for the full-size UVW-KEM-128 public key.'
    exit 0
fi
src=kem-38/Implementations/Reference_Implementation/UVW-KEM-128
gcc -std=c99 -O2 -w -I "$src/include" -o "$tmp/keygen" kem-38/reproduce_jet_keygen.c \
    "$src/src/KEM_AlgorithmInstance.c" "$src/src/drng.c" "$src/src/auxfunc.c" \
    "$src/src/gf_math.c" "$src/src/list_decoding.c" "$src/src/uvw_constants.c" -lm
"$tmp/keygen" 1 > "$tmp/pk.bin"
real=$("$tmp/jet" 860 53 3 1 "$tmp/pk.bin")
echo "$real"
control=$("$tmp/jet" 860 53 2 1)
echo "$control [random control]"
if [ "$(echo "$real" | excess)" -gt 0 ] && [ "$(echo "$control" | excess)" -eq 0 ]; then
    echo 'CONFIRMED kem-38-8 UVW-KEM-128: a submitted public key exceeds the generic cubic-jet count; the random control does not'
else
    echo 'NOT REPRODUCED kem-38-8: full-size counts'
    exit 1
fi
