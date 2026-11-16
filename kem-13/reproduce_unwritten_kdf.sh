#!/bin/sh
# kem-13-5: DKEM-512 drops the pseudohash status in its void KDF wrapper, so
# one failed allocation makes kem_enc return 0 without writing K, and makes
# kem_dec return the caller's (unwritten) buffer for an invalid ciphertext
# instead of the implicit-rejection key. Builds reference DKEM-512 with the
# archive's build.sh flags and malloc wrapped; checks the same KDF source in
# the reference, optimized and additional DKEM-512 trees.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "${NGCC_ROOT:-$here/..}" || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-13-5: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-13-5: $1"; exit 1; }
impl='kem-13/Implementations'
for d in "$impl"/Reference_Implementation/DKEM-512 "$impl"/Optimized_Implementation/DKEM-512 "$impl"/Additional_Implementation/*/DKEM-512; do
    grep -Fq 'static void dke_kem_kdf(uint8_t out[DKE_SSBYTES],' "$d/dkecca.c" &&
        grep -Fq '    dke_hash_extended(DKE_SSBYTES * 8, in, (unsigned long long)in_bytes * 8, out);' "$d/dkecca.c" &&
        grep -Fq '    dke_kem_kdf(ss, rej_buf, 2 * DKE_SSBYTES);' "$d/dkecca.c" &&
        grep -Fq '    return pseudohash(digest_len_bits, msg, msg_len_bits, digest);' "$d/dke_hash.c" ||
        bad "$d differs from the archive"
done
echo 'source: all four DKEM-512 trees route K and the rejection key through a void pseudohash wrapper'
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
d="$impl/Reference_Implementation/DKEM-512"
(cd "$d" && gcc -std=gnu99 -w -O2 -fcommon -DDKE_FORCE_SCALAR -DDKE_MODE=512 -DDKE_HASH=0 -DDKE_RANDOM=0 -I. \
    -Wl,--wrap=malloc -o "$scratch/dkem512" "$here/reproduce_unwritten_kdf.c" sm3.c dke_sm3.c dke_hash.c randombytes.c \
    randombytes_sys.c reduce.c ntt.c poly.c polyvec.c random_sampling.c dke_utils.c packing.c verify.c dkecpa.c \
    dkecca.c KEM_AlgorithmInstance.c drng.c auxfunc.c -lm) 2>"$scratch/cc" || { cat "$scratch/cc"; bad 'build failed'; }
out=$("$scratch/dkem512" 2>/dev/null) || bad 'run failed'
echo "$out" | sed 's/^/Ref-DKEM-512: /'
for want in \
    'enc control: faults=0 kem_enc=0 ss_all_zero=0' \
    'enc KDF alloc fails: faults=1 kem_enc=0 ss_all_zero=1' \
    'dec invalid ct control: faults=0 kem_dec=0 ss_all_zero=0' \
    'dec invalid ct rejection-KDF alloc fails: faults=1 kem_dec=0 ss_all_zero=1'; do
    printf '%s\n' "$out" | grep -Fqx "$want" || bad "missing '$want'"
done
echo 'ATTACK kem-13-5: one failed allocation makes DKEM-512 kem_enc return 0 with K unwritten (all-zero caller buffer) and kem_dec return the unwritten buffer instead of the rejection key for an invalid ciphertext CONFIRMED'
exit 0
