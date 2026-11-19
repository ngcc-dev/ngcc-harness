#!/bin/sh
# sign-16-3: the Octarine verifier ignores pseudoXOF failures when it hashes
# the message representative mu and the challenge seed, so a failed hash
# leaves the previous message's mu (or challenge seed) in place.
#
# Checks the unchecked hash calls in the six SM3 trees (reference and
# optimized), then builds each reference level with malloc wrapped so that
# exactly one allocation of a chosen size fails: pseudoXOF's copy of the mu
# input for a 64- and a 5000-byte message, and its copy of the challenge
# input. A signature on message A must verify for message B when that
# allocation fails right after verifying A, and must be rejected for B without
# the failure and after the stack is scrubbed.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-16-3: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-16-3: $1"; exit 1; }

impl=sign-16/Implementations
for d in "$impl"/Reference_Implementation/Octarine-* "$impl"/Optimized_Implementation/Octarine-*; do
    [ -f "$d/hash_domain.h" ] || bad "$d missing"
    grep -Fq '  RRLWR_SIGN_HASH_H(out, RRLWR_SIGN_MU_LEN, input, inlen);' "$d/hash_domain.h" &&
        grep -Fq '  RRLWR_SIGN_HASH_H(out, RRLWR_SIGN_CTILDE_LEN, input, inlen);' "$d/hash_domain.h" &&
        grep -Fq '  RRLWR_SIGN_HASH_CH(ctildep, mu, w1);' "$d/sign.c" &&
        grep -Fq 'RRLWR_SM3_XOF((output), (output_len), (input), (input_len))' "$d/parameters.h" &&
        grep -Fq 'return MEMORY_ALLOCATION_FAILED;' "$d/utils/auxfunc.c" ||
        bad "$d hash calls differ from the archive"
done
echo 'source: reference and optimized trees discard the pseudoXOF status when hashing mu and the challenge seed'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
ok='verify(A)=0 verify(B)=-1 verify(B, allocation fails)=0 verify(B, allocation fails, stack scrubbed)=-1'
for level in 128 256 512; do
    d="$impl/Reference_Implementation/Octarine-$level"
    (cd "$d" && gcc -O3 -w -I. -Iutils -Iarith -DRRLWR_SECURITY_LEVEL=$level -Wl,--wrap=malloc \
        -o "$scratch/s$level" "$here/reproduce_stale_digest.c" arith/fprime.c arith/poly.c arith/ring.c \
        arith/packing.c arith/uniform.c sign_ring.c sign.c crt.c utils/auxfunc.c utils/drng.c) \
        2>"$scratch/cc" || { cat "$scratch/cc"; bad "Octarine-$level build failed"; }
    "$scratch/s$level" >"$scratch/out" 2>/dev/null || bad "Octarine-$level run failed"
    sed "s/^/Octarine-$level /" "$scratch/out"
    [ "$(grep -c '' "$scratch/out")" -eq 3 ] || bad "Octarine-$level route count differs"
    while IFS= read -r line; do
        case "$line" in *": $ok") ;; *) bad "Octarine-$level behaviour differs: $line" ;; esac
    done <"$scratch/out"
done
echo 'all three reference levels accept a signature on A for B when the mu or challenge pseudoXOF allocation fails after verifying A; all reject without the failure and after the stack is scrubbed'
echo 'ATTACK sign-16-3: an Octarine signature on one message verifies for another when a verifier hash allocation fails after a prior verification: CONFIRMED'
exit 0
