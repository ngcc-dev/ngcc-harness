#!/bin/sh
# sign-17-4: a failed message-sized allocation makes the reference sig_sign
# return success with an empty signature.
#   Source: in the three reference trees crypto_sign_signature discards the
#   status of crypto_sign_signature_internal; in the three AVX2 trees it
#   returns it.
#   Runtime: each tree is built from its unmodified sources with
#   reproduce_sign_alloc_false_success.c. A 64 MiB message is signed once
#   normally and once with RLIMIT_AS capped 32 MiB above the current address
#   space, so the signer's copy of the message cannot be allocated. No
#   allocator interposition. Reference: rc 0, length 0, buffer untouched.
#   AVX2 (if the CPU has AVX2): rc -4.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-17-4: gcc unavailable'; exit 77; }
[ "$(uname -s)" = Linux ] || { echo 'SKIP sign-17-4: needs /proc/self/status (Linux)'; exit 77; }
bad() { echo "NOT CONFIRMED sign-17-4: $1"; exit 1; }
impl="sign-17/Implementations and Test_Vectors/Implementations"
[ -d "$impl/Reference_Implementation/OPSsig-128" ] || bad "tree not found"

for s in 128 256 512; do
    f="$impl/Reference_Implementation/OPSsig-$s/sign.c"
    tr -d '\r' <"$f" | sed -n '/^int crypto_sign_signature(/,/^}/p' |
        grep -q '^  crypto_sign_signature_internal(' || bad "OPSsig-$s reference checks the internal status"
    f="$impl/Optimized_Implementation/OPSsig-$s-avx2/sign.c"
    tr -d '\r' <"$f" | sed -n '/^int crypto_sign_signature(/,/^}/p' |
        grep -q 'return crypto_sign_signature_internal(' || bad "OPSsig-$s-avx2 discards the internal status"
done
echo 'source: 3/3 reference trees discard the status of crypto_sign_signature_internal; 3/3 AVX2 trees return it'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
refsrc='sign.c mult.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c auxfunc.c drng.c SIG_AlgorithmInstance.c'
avxsrc='sign.c packing.c polyvec.c poly.c rejsample.c ntt.c pointwise.S consts.c reduce.c rounding.c sm3x4.c auxfunc.c drng.c SIG_AlgorithmInstance.c'

run() { # label dir flags sources expected-rc
    (cd "$2" && gcc $3 -std=gnu99 -w -I. -DOPS_LABEL="\"$1\"" -o "$scratch/p" "$here/reproduce_sign_alloc_false_success.c" $4) \
        >"$scratch/cc" 2>&1 || { tail -20 "$scratch/cc"; bad "$1 build failed"; }
    out=$("$scratch/p") || { echo "$out"; bad "$1 driver failed"; }
    line=$(echo "$out" | grep '^RESULT ') || { echo "$out"; bad "$1: no result"; }
    set -- "$1" "$5" $(echo "$line" | sed 's/^RESULT [^ ]* //; s/[a-z]*=//g; s#/# #g')
    # $3..$5 short rc/len/verify, $6..$8 big rc/len/verify, $9.. limited rc/len/untouched/verify
    [ "$3" = 0 ] && [ "$5" = 0 ] && [ "$6" = 0 ] && [ "$8" = 0 ] || { echo "$out"; bad "$1: honest signing controls failed"; }
    [ "$9" = "$2" ] || { echo "$out"; bad "$1: capped signing returned ${9}, expected $2"; }
    if [ "$2" = 0 ]; then
        [ "${10}" = 0 ] && [ "${11}" = 1 ] && [ "${12}" != 0 ] ||
            { echo "$out"; bad "$1: capped signing did not return an empty, unwritten signature"; }
        echo "$1: short and 64 MiB messages sign and verify (length $4); with the copy unallocatable sig_sign returns 0, length 0, buffer unwritten, verify ${12}"
    else
        echo "$1: same controls pass; with the copy unallocatable sig_sign returns $9"
    fi
}

for s in 128 256 512; do
    run "OPSsig-$s" "$impl/Reference_Implementation/OPSsig-$s" "-O2" "$refsrc" 0
done
if grep -qw avx2 /proc/cpuinfo 2>/dev/null; then
    run "OPSsig-128-avx2" "$impl/Optimized_Implementation/OPSsig-128-avx2" "-O2 -march=x86-64 -mavx2 -mpopcnt" "$avxsrc" -4
else
    echo 'OPSsig-128-avx2: not run (CPU lacks AVX2)'
fi
echo 'ATTACK sign-17-4: when the message copy cannot be allocated, the reference sig_sign reports success with an empty signature: CONFIRMED'
exit 0
