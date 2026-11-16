#!/bin/sh
# kem-36-10: TRIKE drops the status of pseudohash, init_random_number and its
# own calloc, so a single failed allocation makes kem_keygen, kem_enc or
# kem_dec return 0 with a secret that is computable from public data.
#
# Checks the unchecked calls in all eight trees, then builds each tree (default
# CMake options) with malloc and calloc wrapped so that exactly one allocation
# fails, and runs an attacker that uses only pk and ct:
#   enc ss-hash      : K = 0.
#   enc err-hash     : c2 carries m in the clear.
#   enc errvec       : e = 0 (calloc) or e from the all-zero DRNG (seed alloc).
#   dec ss-hash      : K = 0 for an arbitrary ciphertext.
#   keygen           : h0, h1, h2 from the all-zero DRNG; the attacker rebuilds
#                      a working secret key from pk alone.
# Controls: the same attacker fails on unfaulted runs.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "${NGCC_ROOT:-$here/..}" || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-36-10: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-36-10: $1"; exit 1; }

impl='kem-36/Implementations and Test_Vectors/Implementations'
for d in "$impl"/Reference_Implementation/TRIKE-* "$impl"/Optimized_Implementation/TRIKE-*; do
    k="$d/src/KEM_AlgorithmInstance.c"; s="$d/src/sample.c"
    grep -Fq '	uint8_t temp[64] = {0};' "$k" &&
        grep -Fq '	pseudohash(512, input, input_len, temp);' "$k" &&
        grep -Fq '    if (hash_input == NULL)return;' "$s" &&
        grep -Fq '    init_random_number(&local_drng, seed, M_SIZE_BYTES);' "$s" &&
        grep -Fq '    init_random_number(&local_drng, hash_input, M_SIZE_BYTES + R_SIZE_BYTES);' "$s" &&
        grep -Fq 'return MEMORY_ALLOCATION_FAILED;' "$d/ICCS/auxfunc.c" ||
        bad "$d differs from the archive"
done
echo 'source: all eight trees ignore the hash, DRNG-seeding and error-vector allocation status'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
driver=$here/reproduce_fail_open.c
for d in "$impl"/Reference_Implementation/TRIKE-* "$impl"/Optimized_Implementation/TRIKE-*; do
    name="$(basename "$(dirname "$d")" | cut -c1-3)-$(basename "$d")"
    (cd "$d" && gcc -O2 -std=gnu11 -w -IICCS -Isrc -Wl,--wrap=malloc,--wrap=calloc -o "$scratch/$name" "$driver" \
        ICCS/auxfunc.c ICCS/drng.c src/KEM_AlgorithmInstance.c src/decoder.c src/gf2x.c src/sample.c -lm) \
        2>"$scratch/cc" || { cat "$scratch/cc"; bad "$name build failed"; }
    out=$("$scratch/$name" 2>/dev/null) || bad "$name run failed"
    echo "$out" | sed "s/^/$name: /"
    for want in \
        'enc control: faults=0 kem_enc=0 ss_is_zero=0 owner_dec_matches=1 attacker_hypothesis=0 attacker_ss_matches=0' \
        'enc ss-hash alloc fails: faults=1 kem_enc=0 ss_is_zero=1 owner_dec_matches=0 attacker_hypothesis=0 attacker_ss_matches=0' \
        'enc err-hash alloc fails: faults=1 kem_enc=0 ss_is_zero=0 owner_dec_matches=0 attacker_hypothesis=1 attacker_ss_matches=1' \
        'enc errvec calloc fails: faults=1 kem_enc=0 ss_is_zero=0 owner_dec_matches=0 attacker_hypothesis=2 attacker_ss_matches=1' \
        'enc errvec seed alloc fails: faults=1 kem_enc=0 ss_is_zero=0 owner_dec_matches=0 attacker_hypothesis=3 attacker_ss_matches=1' \
        'dec control (garbage ct): faults=0 kem_dec=0 ss_is_zero=0' \
        'dec ss-hash alloc fails (garbage ct): faults=1 kem_dec=0 ss_is_zero=1' \
        'keygen control: faults=0 kem_keygen=0 owner_dec_matches=1 attacker_key_matches_pk=0 attacker_ss_matches=0' \
        'keygen sk-seed alloc fails: faults=1 kem_keygen=0 owner_dec_matches=1 attacker_key_matches_pk=1 attacker_ss_matches=1'; do
        printf '%s\n' "$out" | grep -Fqx "$want" || bad "$name: missing '$want'"
    done
done
echo 'ATTACK kem-36-10: in all eight trees one failed allocation makes kem_enc return 0 with K = 0 or with m recoverable from ct, kem_dec return K = 0 for any ciphertext, and kem_keygen return a working key pair whose secret key the attacker rebuilds from pk CONFIRMED'
exit 0
