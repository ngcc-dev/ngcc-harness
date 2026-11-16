#!/bin/sh
# sign-06-8: COMPASS-SIG signing discards pseudoXOF failures. If the helper's
# first allocation fails while signing expands the matrix A, the call returns
# 0 with a signature made with a wrong challenge but the same deterministic
# mask y. With the honest signature on the same message this gives s1.
#
# Checks that the XOF wrapper ignores pseudoXOF's status in all eight trees,
# then builds each archived parameter set with malloc wrapped so that exactly
# one allocation (pseudoXOF's 38-byte input copy) fails in one signing call.
# Key generation: failing the same helper's input copy while the DRBG seed is
# expanded returns 0 with a working key pair built from unwritten heap bytes;
# the full secret key is rebuilt from public data (zero bytes in a fresh
# process; the XOF of the previous public key after an earlier keygen).
# Signing: requires s1 recovered exactly from public data and the two signatures, and a
# key rebuilt from pk and s1 forges a fresh message; control: a re-signature
# without the fault is identical to the honest one.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "${NGCC_ROOT:-$here/..}" || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-06-8: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-06-8: $1"; exit 1; }

impl='sign-06/Implementations'
for d in "$impl"/Reference_Implementation/COMPASS-SIG-* "$impl"/Optimized_Implementation/COMPASS-SIG-*; do
    grep -Fq 'state->stream_extra = malloc(state->stream_extra_len);' "$d/symmetric-shake.c" &&
        grep -Eq '^  pseudoXOF\(state->stream_extra_len \* 8,' "$d/symmetric-shake.c" &&
        grep -Fq 'return MEMORY_ALLOCATION_FAILED;' "$d/auxfunc.c" ||
        bad "$d: XOF wrapper differs from the archive"
done
echo 'source: all eight trees call pseudoXOF without checking its status (symmetric-shake.c stream_init, fill_squeeze)'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
driver=$here/reproduce_fault_key_recovery.c
for level in 128 256 384 512; do
    d="$impl/Reference_Implementation/COMPASS-SIG-$level"
    (cd "$d" && gcc -O2 -std=c99 -w -I. -DCOMPASS_SIG_MODE=$level -Wl,--wrap=malloc -o "$scratch/r$level" "$driver" \
        sign.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c symmetric-shake.c drng.c auxfunc.c) \
        2>"$scratch/cc" || { cat "$scratch/cc"; bad "COMPASS-SIG-$level build failed"; }
    out=$("$scratch/r$level" 2>/dev/null) || bad "COMPASS-SIG-$level run failed"
    echo "COMPASS-SIG-$level: $out"
    case "$out" in
        'control_resign_identical=1 faulted_sign_rc=0 faulted_verify=-1 messages_tried='*' s1_small_solution=1 s1_matches_secret=1 forged_verify=0') ;;
        *) bad "COMPASS-SIG-$level behaviour differs" ;;
    esac
    k1=$("$scratch/r$level" keygen 1 2>/dev/null) && k2=$("$scratch/r$level" keygen 2 2>/dev/null) ||
        bad "COMPASS-SIG-$level keygen run failed"
    echo "COMPASS-SIG-$level keygen, fresh process, seed 1: $k1"
    echo "COMPASS-SIG-$level keygen, fresh process, seed 2: $k2"
    case "$k1" in
        'faulted_keygen_rc=0 rebuilt_pk_match=1 rebuilt_sk_match=1 forged_verify=0 pk_fnv='*) ;;
        *) bad "COMPASS-SIG-$level keygen behaviour differs" ;;
    esac
    [ "$k1" = "$k2" ] || bad "COMPASS-SIG-$level faulted key depends on the DRBG seed"
    out=$("$scratch/r$level" keygen-after-key 2>/dev/null) || bad "COMPASS-SIG-$level keygen run failed"
    echo "COMPASS-SIG-$level keygen after an earlier key: $out"
    [ "$out" = 'faulted_keygen_rc=0 honest_key_not_rebuilt=1 rebuilt_pk_match=1 rebuilt_sk_match=1 forged_verify=0' ] ||
        bad "COMPASS-SIG-$level keygen-after-key behaviour differs"
done
echo 'ATTACK sign-06-8: at all four levels a key generation with one failed helper allocation returns a working key pair whose secret key is rebuilt from public data, and one faulted signature plus the honest one on the same message give s1; both recovered keys forge a fresh message CONFIRMED'
exit 0
