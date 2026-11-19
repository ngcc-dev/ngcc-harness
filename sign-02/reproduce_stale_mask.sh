#!/bin/sh
# sign-02-6: when the allocation inside BiT's seed_y derivation fails, signing
# returns a valid signature whose masks come from a stale seed_y, and the
# signing key follows from public data.
#
# Checks the void seed_y derivation in all six trees, then builds each
# reference level with malloc wrapped so that exactly one allocation fails:
# pseudoXOF's copy of secret_seed || rnd || mu (fixed size). Sixteen rounds of
# one ordinary signature followed at once by one faulty signature. From pk and
# the faulty signatures alone the driver solves for s0 (a) from one signature
# whose mask is the expansion of the all-zero seed left by the previous call's
# secure_zero, and (b) from two signatures with the same verifier commitment.
# Both must equal the key's s0 (single-signature route required at BiT-128 and
# BiT-256; at BiT-512 no faulty signature was accepted at the first round in
# this run). Controls: the ordinary signatures give no zero-seed mask and no
# equal commitments.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-02-6: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-02-6: $1"; exit 1; }

impl=sign-02/Implementations
for d in "$impl"/Reference_Implementation/BiT-* "$impl"/Optimized_Implementation/BiT-*-avx2; do
    grep -Fq 'bit_xof256_3(temp_seed_y, BIT_SEEDBYTES' "$d/sign.c" &&
        grep -Fq 'secure_zero(temp_seed_y, sizeof(temp_seed_y));' "$d/sign.c" &&
        grep -Fq '(void)pseudoXOF((unsigned long long)outlen * 8, msg, (unsigned long long)msg_len * 8, out);' "$d/symmetric.c" &&
        grep -Fq 'return MEMORY_ALLOCATION_FAILED;' "$d/auxfunc.c" ||
        bad "$(basename "$d") seed_y derivation differs from the archive"
done
echo 'source: all six trees derive seed_y with a void bit_xof256_3 that drops the pseudoXOF status, and zero seed_y on return'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
get() { printf '%s\n' "$1" | tr ' ' '\n' | sed -n "s/^$2=//p"; }
for level in 128 256 512; do
    d="$impl/Reference_Implementation/BiT-$level"
    (cd "$d" && gcc -O2 -std=c99 -w -I. -Wl,--wrap=malloc -o "$scratch/s$level" "$here/reproduce_stale_mask.c" \
        $(ls *.c | grep -v KAT_SIG.c)) 2>"$scratch/cc" || { cat "$scratch/cc"; bad "BiT-$level build failed"; }
    out=$("$scratch/s$level" 2>/dev/null) || bad "BiT-$level run failed"
    echo "BiT-$level: $out"
    [ "$(get "$out" reached)" = 16/16 ] && [ "$(get "$out" faulty_valid)" = 16 ] &&
        [ "$(get "$out" pair_s0_equal)" = 1 ] && [ "$(get "$out" equal_commitment_pairs)" -ge 1 ] &&
        [ "$(get "$out" ordinary_zero_seed_masks)" = 0 ] && [ "$(get "$out" ordinary_pairs)" = 0 ] ||
        bad "BiT-$level behaviour differs"
    case $level in 512) ;; *) [ "$(get "$out" single_s0_equal)" = 1 ] || bad "BiT-$level single-signature route differs" ;; esac
done
echo 'all three reference levels return valid signatures with stale masks when the seed_y allocation fails; s0 is recovered from pairs of them at every level and from a single one at BiT-128 and BiT-256; ordinary signatures show neither'
echo 'ATTACK sign-02-6: a failed seed_y allocation makes BiT signing reuse a stale mask seed and leak the signing key: CONFIRMED'
exit 0
