#!/bin/sh
# kem-23-4: Mito's implicit-rejection hash covers uninitialised stack bytes.
#
# Checks hash_j in the reference and optimized trees, then builds each of the
# nine reference sets and compares the hashed buffer size with the bytes
# written into it. A positive gap at every set confirms the finding. The driver
# also decapsulates one invalid ciphertext twice over different stack contents;
# whether the two rejection keys then differ depends on the compiler's stack
# layout and is reported, not required.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-23-4: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-23-4: $1"; exit 1; }

sets='1-128 1-256 1-512 1-E-128 1-E-256 1-E-512 2-E-128 2-E-256 2-E-512'
for tree in Reference Optimized; do
    for s in $sets; do
        f=kem-23/Implementations/${tree}_Implementation/Mito-$s/symmetric.c
        grep -Fq 'uint8_t in[SEED_BYTES + PARAM_K_BYTES + sizeof(ciphertext_kem_t)];' "$f" &&
            grep -Fq 'pseudohash(SHARED_SECRET_BYTES * 8, in, sizeof(in) * 8, output);' "$f" ||
            bad "$tree Mito-$s hash_j differs from the archive"
    done
done
echo 'source: all 18 trees size the hash_j input by sizeof(ciphertext_kem_t) and hash sizeof(in) bytes'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
unstable=0
for s in $sets; do
    d=kem-23/Implementations/Reference_Implementation/Mito-$s
    gcc -O2 -w -I"$d" -o "$scratch/t" kem-23/reproduce_reject_key_stack.c \
        $(ls "$d"/*.c | grep -v KAT_KEM.c) 2>"$scratch/cc" || { cat "$scratch/cc"; bad "Mito-$s build failed"; }
    out=$("$scratch/t") || bad "Mito-$s run failed"
    echo "Mito-$s: $out"
    case "$out" in *' gap=0 '*) bad "Mito-$s has no gap" ;; *'valid_stable=1 '*) ;; *) bad "Mito-$s valid path unstable" ;; esac
    case "$out" in *'reject_stable=0') unstable=$((unstable + 1)) ;; esac
done
echo "CONFIRMED kem-23-4: every set hashes 14 to 32 unwritten stack bytes into the rejection key; $unstable of 9 sets returned two different keys for one invalid ciphertext in this build"
exit 0
