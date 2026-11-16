#!/bin/sh
# kem-07-5: BRQC drops the status of pseudohash and init_random_number, so a
# single failed allocation makes kem_enc or kem_keygen return 0 while the
# encryption coins or the secret key come from public data.
#
# Checks the unchecked calls in all six trees, then builds each tree with
# malloc wrapped so that exactly one allocation fails, and runs an attacker
# that uses only pk and ct:
#   enc G-hash   : theta = G(pk, m, salt) is not written and stays {0};
#                  the attacker re-encrypts with theta = 0 and decodes m.
#   enc coin-seed: the coin expander keeps its all-zero DRNG state.
#   keygen       : the sk seed expander keeps its all-zero DRNG state, so
#                  (x, y) are fixed public values; the attacker decrypts.
# Controls: the same attacker fails on unfaulted runs.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "${NGCC_ROOT:-$here/..}" || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-07-5: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-07-5: $1"; exit 1; }

impl='kem-07/Implementations'
for d in "$impl"/Reference_Implementation/BRQC-* "$impl"/Optimized_Implementation/BRQC-*; do
    grep -Fq 'uint8_t theta[SHA512_BYTES] = {0};' "$d/src/kem.c" &&
        grep -Fq '  pseudohash(512, G_in, (BRQC_PUBLIC_KEY_BYTES + BRQC_VEC_K_BYTES + BRQC_SALT_BYTES) * 8, theta);' "$d/src/kem.c" &&
        grep -Fq '  random_source_seed_with_len(&seedexpander, theta, SHA512_BYTES);' "$d/src/brqc.c" &&
        grep -Fq '  random_source_seed_with_len(&sk_seedexpander, sk_seed, SEEDEXPANDER_SEED_BYTES);' "$d/src/brqc.c" &&
        grep -Fq '  init_random_number(&o->ctx, seed, seed_len_bytes);' "$d/lib/random_source/random_source.c" &&
        grep -Fq 'return MEMORY_ALLOCATION_FAILED;' "$d/auxfunc.c" ||
        bad "$d differs from the archive"
done
echo 'source: all six trees ignore the status of the theta hash and of every seed-expander init'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
driver=$here/reproduce_zero_coins.c
for d in "$impl"/Reference_Implementation/BRQC-* "$impl"/Optimized_Implementation/BRQC-*; do
    name="$(basename "$(dirname "$d")" | cut -c1-3)-$(basename "$d")"
    rbc=$(ls -d "$d"/src/rbc-*) || bad "$name: no rbc directory"
    rbc=$(basename "$rbc")
    (cd "$d" && gcc -O2 -std=gnu99 -w -mavx2 -mpclmul -msse4.2 -I. -Isrc -I"src/$rbc" -Ilib/random_source \
        -Ilib/XKCP -Ilib/XKCP/opt64 -Ilib/wrapper -Wl,--wrap=malloc -o "$scratch/$name" "$driver" \
        "src/$rbc/rbc_elt.c" "src/$rbc/rbc_vec.c" "src/$rbc/rbc_poly.c" "src/$rbc/rbc_qre.c" "src/$rbc/rbc_vspace.c" \
        src/parsing.c src/qpoly.c src/gabidulin.c src/brqc.c src/kem.c KEM_BRQC-*.c auxfunc.c drng.c \
        lib/random_source/random_source.c lib/XKCP/KeccakHash.c lib/XKCP/KeccakSponge.c lib/XKCP/SimpleFIPS202.c \
        lib/XKCP/opt64/KeccakP-1600-opt64.c) 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$name build failed"; }
    out=$("$scratch/$name" 2>/dev/null) || bad "$name run failed"
    echo "$out" | sed "s/^/$name: /"
    for want in \
        'enc control: faults=0 kem_enc=0 owner_dec_matches=1 attacker_confirms_coins=0 attacker_ss_matches=0' \
        'enc control (zero-state guess): faults=0 kem_enc=0 owner_dec_matches=1 attacker_confirms_coins=0 attacker_ss_matches=0' \
        'enc G-hash alloc fails: faults=1 kem_enc=0 owner_dec_matches=0 attacker_confirms_coins=1 attacker_ss_matches=1' \
        'enc coin-seed alloc fails: faults=1 kem_enc=0 owner_dec_matches=0 attacker_confirms_coins=1 attacker_ss_matches=1' \
        'keygen control: faults=0 kem_keygen=0 owner_dec_matches=1 attacker_key_matches_pk=0 attacker_ss_matches=0'; do
        printf '%s\n' "$out" | grep -Fqx "$want" || bad "$name: missing '$want'"
    done
    printf '%s\n' "$out" | grep -Eqx 'keygen sk-seed alloc fails: faults=1 kem_keygen=0 owner_dec_matches=(0|-1) attacker_key_matches_pk=1 attacker_ss_matches=1' ||
        bad "$name: keygen case differs"
done
echo 'ATTACK kem-07-5: in all six trees one failed allocation makes kem_enc return 0 with zero/public coins, and kem_keygen return 0 with a public-derived secret key; from pk and ct alone the attacker recomputes the shared secret CONFIRMED'
exit 0
