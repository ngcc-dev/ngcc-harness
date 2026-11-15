#!/bin/sh
# kex-03-11: the K2K-PLAC512 implicit-rejection key hashes only the first 584
# ciphertext bytes, because a byte count is passed to pseudoXOF as a bit count.
#
# Checks the call in the reference and optimized PLAC512 and PLAC512Star trees,
# then builds the unmodified reference K2K-PLAC512 sources and
# decapsulates two different invalid ciphertexts that agree on bytes 0..583.
# They must give the same rejection key; flipping byte 583 must change it.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kex-03-11: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kex-03-11: $1"; exit 1; }

for tree in Reference Optimized; do
    for v in CreTAKE-K2K-PLAC512 CreTAKE-K2K-PLAC512Star; do
        f=kex-03/Implementations/${tree}_Implementation/CreTAKE512/$v/twokem.c
        grep -Fq 'unsigned char buf_rej[KEM_REJECT_SEED_BYTES + 2 * PKE_CIPHERTEXT_BYTES];' "$f" &&
            grep -Fq 'pseudoXOF(SS_KEY_BYTES*8, buf_rej, sizeof(buf_rej), ss_reject)' "$f" ||
            bad "$tree $v differs from the archived rejection-key call"
    done
done
echo 'source: reference and optimized PLAC512/PLAC512Star pass sizeof(buf_rej) = 5184 as a bit length (648 bytes)'

ref=kex-03/Implementations/Reference_Implementation
d=$ref/CreTAKE512/CreTAKE-K2K-PLAC512
k=$ref/Common/primitives/kem/POLARLAC-512
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
gcc -std=c99 -O2 -w -DBIT_USE_SHAKE=0 -DRL_KEM_USE_CONJ_NTT_REJECTION=0 -I"$d" -I"$k" -o "$scratch/t" \
    kex-03/reproduce_reject_key_prefix.c "$d/twokem.c" "$d/twopke.c" "$d/auxfunc.c" "$d/drng.c" \
    "$d/secure_bzero.c" "$k/KEM_AlgorithmInstance.c" "$k/pke.c" "$k/polar.c" "$k/poly.c" "$k/ntt.c" \
    "$k/fft.c" "$k/sample.c" "$k/symmetric.c" -lm 2>"$scratch/cc" ||
    { cat "$scratch/cc"; bad 'build failed'; }
out=$("$scratch/t") || bad 'run failed'
echo "K2K-PLAC512: $out"
[ "$out" = 'ct_bytes=4608 honest_ok=1 distinct_ciphertexts=1 same_reject_key=1 rejected=1 byte583_changes_key=1' ] ||
    bad 'behaviour differs'
echo 'CONFIRMED kex-03-11: two different invalid ciphertexts agreeing on bytes 0..583 share one rejection key; byte 583 still affects it'
exit 0
