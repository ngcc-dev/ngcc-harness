#!/bin/sh
# sign-02-5: the BiT verifier ignores failed allocations in its message-digest
# helper and challenge hash, so a failed digest leaves the previous message's
# digest (or challenge) in place.
#
# Checks the void digest helpers in all six trees, then builds each reference
# level with malloc wrapped so that exactly one allocation of a chosen size
# fails. Routes: the helper's own copy of tr || m for a 5000-byte message (all
# levels); pseudohash's first copy of tr || m for a 64-byte message, and of
# mu || w1 for the recomputed challenge (BiT-256/512; BiT-128 uses sm3hash,
# which allocates nothing). A signature on message A must verify for message B
# when that allocation fails right after verifying A, and must be rejected for
# B without the failure and after the stack is scrubbed.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-02-5: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-02-5: $1"; exit 1; }

impl=sign-02/Implementations
for d in "$impl"/Reference_Implementation/BiT-* "$impl"/Optimized_Implementation/BiT-*-avx2; do
    [ -f "$d/symmetric.c" ] || bad "$d missing"
    grep -Eq 'if \((msg == NULL|!msg)\) return;' "$d/symmetric.c" &&
        grep -Fq 'void bit_h256_2(uint8_t out[32],' "$d/symmetric.c" &&
        grep -Fq 'return MEMORY_ALLOCATION_FAILED;' "$d/auxfunc.c" ||
        bad "$(basename "$d") digest helper differs from the archive"
    case $d in *-128*) ;; *)
        grep -Fq 'void bit_h512_2(uint8_t out[64],' "$d/symmetric.c" &&
            grep -Fq '(void)pseudohash(512, in, (unsigned long long)inlen * 8, out);' "$d/symmetric.c" ||
            bad "$(basename "$d") bit_h512 differs from the archive" ;;
    esac
done
echo 'source: all six trees return from void bit_h*_2 without writing the digest when malloc fails; BiT-256/512 also cast pseudohash status to void'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
ok='verify(A)=0 verify(B)=-1 verify(B, allocation fails)=0 verify(B, allocation fails, stack scrubbed)=-1'
for level in 128 256 512; do
    d="$impl/Reference_Implementation/BiT-$level"
    (cd "$d" && gcc -O2 -std=c99 -w -I. -Wl,--wrap=malloc -o "$scratch/s$level" "$here/reproduce_stale_digest.c" \
        $(ls *.c | grep -v KAT_SIG.c)) 2>"$scratch/cc" || { cat "$scratch/cc"; bad "BiT-$level build failed"; }
    "$scratch/s$level" >"$scratch/out" 2>/dev/null || bad "BiT-$level run failed"
    sed "s/^/BiT-$level /" "$scratch/out"
    case $level in 128) want=1 ;; *) want=3 ;; esac
    [ "$(grep -c '' "$scratch/out")" -eq "$want" ] || bad "BiT-$level route count differs"
    while IFS= read -r line; do
        case "$line" in *": $ok") ;; *) bad "BiT-$level behaviour differs: $line" ;; esac
    done <"$scratch/out"
done
echo 'all three reference levels accept a signature on A for B when the 5000-byte digest copy fails after verifying A; BiT-256/512 also via the 64-byte digest copy and the challenge hash; all reject without the failure and after the stack is scrubbed'
echo 'ATTACK sign-02-5: a BiT signature on one message verifies for another when a verifier digest allocation fails after a prior verification: CONFIRMED'
exit 0
