#!/bin/sh
# kem-23-5: Mito's encapsulation discards the status of hash_g, so a failed
# hash leaves K_theta at its all-zero initial value and crypto_kem_enc returns
# 0 with an all-zero shared secret and all-zero encryption coins theta.
#
# Checks the zero-initialised K_theta and the void hash_g wrapper in all 18
# trees, then builds every reference and optimized set with malloc wrapped so
# that exactly one allocation of the size hash_g's pseudohash requests fails.
# Control: an unfaulted encapsulation yields a nonzero key that decapsulates.
# Faulted: rc is 0 and the key equals the 64 zero bytes an attacker predicts.
set -u
here=$(cd "$(dirname "$0")" && pwd)
[ -d kem-23/Implementations ] || cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-23-5: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-23-5: $1"; exit 1; }

sets='1-128 1-256 1-512 1-E-128 1-E-256 1-E-512 2-E-128 2-E-256 2-E-512'
for tree in Reference Optimized; do
    for s in $sets; do
        d=kem-23/Implementations/${tree}_Implementation/Mito-$s
        grep -Fq 'uint8_t K_theta[SHARED_SECRET_BYTES + SEED_BYTES] = { 0 };' "$d/mito_kem.c" &&
            grep -Fq 'hash_g(K_theta, hash_ek_kem, m, c_kem_t.salt);' "$d/mito_kem.c" &&
            grep -Fq 'memcpy(ss, K_theta, SHARED_SECRET_BYTES);' "$d/mito_kem.c" &&
            grep -Fq 'pseudohash((SEED_BYTES + SHARED_SECRET_BYTES) * 8, in, sizeof(in) * 8, output);' "$d/symmetric.c" &&
            grep -Fq 'return MEMORY_ALLOCATION_FAILED;' "$d/auxfunc.c" ||
            bad "$tree Mito-$s differs from the archive"
    done
done
echo 'source: all 18 trees zero-initialise K_theta and call hash_g through a void wrapper that drops pseudohash errors'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
for tree in Reference Optimized; do
    for s in $sets; do
        d=kem-23/Implementations/${tree}_Implementation/Mito-$s
        gcc -O2 -w -I"$d" -Wl,--wrap=malloc -o "$scratch/t" "$here/reproduce_zero_encaps_key.c" \
            $(ls "$d"/*.c | grep -v KAT_KEM.c) 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$tree Mito-$s build failed"; }
        out=$("$scratch/t" 2>/dev/null) || bad "$tree Mito-$s run failed"
        echo "$tree Mito-$s: $(echo "$out" | tr '\n' ' ')"
        case "$out" in
            *'control: rc=0 ss_zero=0 dec_agrees=1'*) ;;
            *) bad "$tree Mito-$s control differs" ;;
        esac
        case "$out" in
            *'faulted: alloc='*' rc=0 ss_zero=1 attacker_matches=1 dec_agrees=0'*) ;;
            *) bad "$tree Mito-$s faulted run differs" ;;
        esac
    done
done
echo 'ATTACK kem-23-5: in all 18 Mito builds a single failed hash_g allocation makes crypto_kem_enc return 0 with an all-zero shared secret: CONFIRMED'
exit 0
