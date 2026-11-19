#!/bin/sh
# sign-16-5: when the allocation inside Octarine's key-derivation hash fails,
# key generation returns success with the previous call's key pair.
#
# Checks the unchecked (rho, rho', K) derivation in the six SM3 trees
# (reference and optimized), then builds each reference level with malloc
# wrapped so that exactly one allocation fails: pseudoXOF's copy of the encoded
# seed xi (fixed size). An ordinary key generation (party A) is followed at once
# by one with that allocation failing (party B); B's public and secret keys must
# equal A's, and a signature made with A's secret key must verify under B's
# public key (and be rejected for a changed message). Controls: an ordinary key
# generation gives a different key pair, and the same failure after the stack
# has been overwritten gives a key pair equal to neither.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-16-5: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-16-5: $1"; exit 1; }

impl=sign-16/Implementations
for d in "$impl"/Reference_Implementation/Octarine-* "$impl"/Optimized_Implementation/Octarine-*; do
    grep -Fq '  RRLWR_SIGN_HASH_KDF(rho_rhoprime_K, xi);' "$d/sign.c" &&
        grep -Fq '  RRLWR_SIGN_HASH_H(out, RRLWR_SIGN_RHO_LEN + RRLWR_SIGN_RHOPRIME_LEN + RRLWR_SIGN_K_LEN, input, inlen);' "$d/hash_domain.h" &&
        grep -Fq 'RRLWR_SM3_XOF((output), (output_len), (input), (input_len))' "$d/parameters.h" &&
        grep -Fq 'return MEMORY_ALLOCATION_FAILED;' "$d/utils/auxfunc.c" ||
        bad "$d key derivation differs from the archive"
done
echo "source: reference and optimized trees derive (rho, rho', K) with a void hash that drops the pseudoXOF status"

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
get() { printf '%s\n' "$1" | tr ' ' '\n' | sed -n "s/^$2=//p"; }
for level in 128 256 512; do
    d="$impl/Reference_Implementation/Octarine-$level"
    (cd "$d" && gcc -O3 -w -I. -Iutils -Iarith -DRRLWR_SECURITY_LEVEL=$level -Wl,--wrap=malloc \
        -o "$scratch/k$level" "$here/reproduce_stale_keygen.c" arith/fprime.c arith/poly.c arith/ring.c \
        arith/packing.c arith/uniform.c sign_ring.c sign.c crt.c utils/auxfunc.c utils/drng.c) \
        2>"$scratch/cc" || { cat "$scratch/cc"; bad "Octarine-$level build failed"; }
    out=$("$scratch/k$level" 2>/dev/null) || bad "Octarine-$level run failed"
    echo "Octarine-$level: $out"
    [ "$(get "$out" allocations_of_that_size_per_keygen)" = 1 ] &&
        [ "$(get "$out" keygen_A)" = 0 ] && [ "$(get "$out" keygen_B)" = 0 ] &&
        [ "$(get "$out" B_equals_A)" = 1 ] &&
        [ "$(get "$out" keygen_control)" = 0 ] && [ "$(get "$out" control_equals_A)" = 0 ] &&
        [ "$(get "$out" keygen_scrubbed)" = 0 ] && [ "$(get "$out" scrubbed_equals_A)" = 0 ] &&
        [ "$(get "$out" scrubbed_equals_control)" = 0 ] &&
        [ "$(get "$out" A_signs_for_B)" = 0 ] && [ "$(get "$out" changed_message)" = -1 ] ||
        bad "Octarine-$level behaviour differs"
done
echo "all three reference levels return success with the previous call's key pair when the key-derivation allocation fails; the earlier holder's secret key signs for the new public key; ordinary and scrubbed-stack key generations differ"
echo "ATTACK sign-16-5: a failed key-derivation allocation makes Octarine key generation return the previous call's key pair: CONFIRMED"
exit 0
