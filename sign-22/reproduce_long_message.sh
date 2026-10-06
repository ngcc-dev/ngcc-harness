#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
compiler=${CC:-cc}

for level in 128 512; do
    src="$root/Implementations and Test_Vectors/Implementations/Reference_Implementation/Rhyme-SM3/Rhyme-SM3-$level"
    (
        cd "$src"
        "$compiler" -O2 -std=c99 -I. -Iinclude -Isrc/keygen \
            -DRHYME_NO_AES -DRHYME_MODE="$level" \
            -DOUTPUT_BLANK_TEST_VECTORS=0 \
            -o "$scratch/check-$level" "$root/reproduce_long_message.c" \
            src/poly.c src/ntt.c src/ntt_tables.c src/sampler.c \
            src/encoding.c src/packing.c src/sign.c src/zpntt.c \
            src/symmetric-shake.c sm3.c sm3_xof.c rhyme_xof.c \
            src/keygen/kg_main.c src/keygen/kg_solver.c \
            src/keygen/kg_zint.c src/keygen/kg_ntt.c src/keygen/kg_primes.c \
            auxfunc.c drng.c -lm
    )
    "$scratch/check-$level"
done
