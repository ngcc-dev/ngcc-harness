#!/bin/sh
# Unmodified HEP-QC encapsulation: the public salt is the next PRNG state prefix.
# Usage: sh kem-17/reproduce_prng_state_leak.sh [search_bits]   (default 16)
set -eu
cd "$(dirname "$0")/.."
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-17-3 generator check: gcc unavailable'; exit 77; }
bits=${1:-16}
root=$(pwd)
impl="$root/kem-17/Implementations/Implementations"
tmp=$(mktemp -d)
trap 'rm -r "$tmp"' EXIT HUP INT TERM
for set in 1 3 5 7; do
    dir="$impl/ref/hep-qc-$set"
    gcc -std=c99 -O2 -w -DHEP_QC_ARCH_REF=1 \
        -I "$impl/common" -I "$impl/common/hep-qc-$set" -I "$impl/ref" -I "$dir" \
        -I "$root/api" -o "$tmp/probe-$set" \
        "$root/kem-17/reproduce_prng_state_leak.c" \
        "$impl/ref/KEM_HEP_QC.c" "$impl/ref/PKE_HEP_QC.c" \
        "$impl/common/code.c" "$impl/common/crypto_memset.c" "$impl/common/fft.c" \
        "$impl/common/gf.c" "$impl/common/gf2x.c" "$impl/common/parsing.c" \
        "$impl/common/reed_muller.c" "$impl/common/reed_solomon.c" \
        "$impl/common/symmetric.c" "$impl/common/vector.c" \
        "$root/api/auxfunc.c" "$root/api/drng.c"
    "$tmp/probe-$set" "$bits"
done
