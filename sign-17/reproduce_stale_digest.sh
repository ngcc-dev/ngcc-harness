#!/bin/sh
# sign-17-3: the OPS-SIG verifier discards the status of its message-digest
# call, so a failed digest leaves the previous message's digest in place.
#
# Checks the void crh wrapper in all six trees, then builds each reference
# level with malloc wrapped so that exactly one allocation of the size the
# digest helper requests can be made to fail. A signature on message A must
# verify for message B when that allocation fails right after verifying A, and
# must be rejected for B without the failure and after the stack is scrubbed.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-17-3: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-17-3: $1"; exit 1; }

impl='sign-17/Implementations and Test_Vectors/Implementations'
for d in "$impl"/Reference_Implementation/OPSsig-* "$impl"/Optimized_Implementation/OPSsig-*; do
    grep -Fq 'static void crh(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)' "$d/sign.c" &&
        grep -Fq 'return MEMORY_ALLOCATION_FAILED;' "$d/auxfunc.c" ||
        bad "$(basename "$d") digest wrapper differs from the archive"
done
echo 'source: all six trees wrap pseudohash in a void crh() that drops its error status'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
driver=$PWD/sign-17/reproduce_stale_digest.c
for level in 128 256 512; do
    d="$impl/Reference_Implementation/OPSsig-$level"
    (cd "$d" && gcc -O2 -std=c99 -w -I. -Wl,--wrap=malloc -o "$scratch/s$level" "$driver" \
        sign.c mult.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c auxfunc.c drng.c) \
        2>"$scratch/cc" || { cat "$scratch/cc"; bad "OPSsig-$level build failed"; }
    out=$("$scratch/s$level" 2>/dev/null) || bad "OPSsig-$level run failed"
    echo "OPSsig-$level: $out"
    case "$out" in
        'verify(A)=0 verify(B)=-'*' verify(B, digest allocation fails)=0 verify(B, digest allocation fails, stack scrubbed)=-'*) ;;
        *) bad "OPSsig-$level behaviour differs" ;;
    esac
done
echo 'CONFIRMED sign-17-3: at all three levels a signature on A verifies for B when the digest allocation fails after verifying A; it is rejected without the failure and after the stack is scrubbed'
exit 0
