#!/bin/sh
# sign-16-4: when the allocation inside Octarine's rho'' hash fails, signing
# returns a valid signature made with the previous signature's rho'', and the
# signing key follows from the two public signatures.
#
# Checks the unchecked rho'' hash in the six SM3 trees (reference and
# optimized), then builds each reference level with malloc wrapped so that
# exactly one allocation fails: pseudoXOF's copy of the encoded (K, rnd, mu)
# input (fixed size). Eight rounds of one ordinary signature followed at once
# by one faulty signature. From each pair the driver solves
# (c2 - c1) * s1 = z2 - z1 using public data only; the result must equal the
# key's s1, and a key rebuilt from pk and s1 must sign a fresh message that the
# unmodified verifier accepts (and rejects for a changed message). Control:
# two consecutive ordinary signatures give no short solution.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-16-4: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-16-4: $1"; exit 1; }

impl=sign-16/Implementations
for d in "$impl"/Reference_Implementation/Octarine-* "$impl"/Optimized_Implementation/Octarine-*; do
    grep -Fq '  RRLWR_SIGN_HASH_RHOPP(rhopp, K, rnd, mu);' "$d/sign.c" &&
        grep -Fq '  RRLWR_SIGN_HASH_H(out, RRLWR_SIGN_RHOPRIMEPRIME_LEN, input, inlen);' "$d/hash_domain.h" &&
        grep -Fq 'RRLWR_SM3_XOF((output), (output_len), (input), (input_len))' "$d/parameters.h" &&
        grep -Fq 'return MEMORY_ALLOCATION_FAILED;' "$d/utils/auxfunc.c" ||
        bad "$d rho'' derivation differs from the archive"
done
echo "source: reference and optimized trees derive rho'' with a void hash that drops the pseudoXOF status"

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
get() { printf '%s\n' "$1" | tr ' ' '\n' | sed -n "s/^$2=//p"; }
for level in 128 256 512; do
    d="$impl/Reference_Implementation/Octarine-$level"
    (cd "$d" && gcc -O3 -w -I. -Iutils -Iarith -DRRLWR_SECURITY_LEVEL=$level -Wl,--wrap=malloc \
        -o "$scratch/s$level" "$here/reproduce_stale_mask.c" arith/fprime.c arith/poly.c arith/ring.c \
        arith/packing.c arith/uniform.c sign_ring.c sign.c crt.c utils/auxfunc.c utils/drng.c) \
        2>"$scratch/cc" || { cat "$scratch/cc"; bad "Octarine-$level build failed"; }
    out=$("$scratch/s$level" 2>/dev/null) || bad "Octarine-$level run failed"
    echo "Octarine-$level: $out"
    [ "$(get "$out" reached)" = 8/8 ] && [ "$(get "$out" faulty_valid)" = 8 ] &&
        [ "$(get "$out" short_s1_recovered)" -ge 1 ] && [ "$(get "$out" s1_equal)" = 1 ] &&
        [ "$(get "$out" forgery_verify)" = 0 ] && [ "$(get "$out" changed_message_verify)" = -1 ] &&
        [ "$(get "$out" ordinary_pairs_recovered)" = 0 ] || bad "Octarine-$level behaviour differs"
done
echo "all three reference levels return valid signatures with the previous rho'' when its allocation fails; s1 is recovered from one ordinary and one faulty signature and signs a fresh message; ordinary pairs give nothing"
echo "ATTACK sign-16-4: a failed rho'' allocation makes Octarine signing reuse the previous mask and leak the signing key: CONFIRMED"
exit 0
