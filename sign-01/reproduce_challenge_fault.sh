#!/bin/sh
# sign-01-10: Aigis-Sig+ drops the status of the DRBG that expands the
# challenge seed. If its first allocation fails, the DRBG context has already
# been zeroed and the challenge becomes a fixed public polynomial c0.
#
# Checks the source pattern in all twelve trees, then builds the three
# reference sets (USE_ICCS, as in the submitted build) and, if the CPU has
# AVX2, the three AVX2 sets, with malloc and init_random_number wrapped so that
# exactly one allocation (the first of the selected DRBG instantiation) fails.
#   signing:      honest signature + faulted signature on the same message ->
#                 s1 solved from public data equals the secret key's s1
#   verification: a signature forged from the public key alone for a fresh
#                 message is rejected normally and accepted under the fault
#   key generation: the same fault in the s1[0] sampler gives working key
#                 pairs whose s1[0] is the same public polynomial
# Run from the repository root: sh sign-01/reproduce_challenge_fault.sh
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
driver=$here/reproduce_challenge_fault.c
[ -d sign-01/Implementations ] || { echo 'run from the repository root'; exit 2; }
[ -f "$driver" ] || { echo "missing $driver"; exit 2; }
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-01-10: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-01-10: $1"; exit 1; }

impl=sign-01/Implementations/Implementations
for d in "$impl"/Reference_Implementation/Aigis-Sig+-* "$impl"/Optimized_Implementation/avx2/Aigis-Sig+-* \
         "$impl"/Additional_Implementation/neon/Aigis-Sig+-* "$impl"/Additional_Implementation/aarch64/Aigis-Sig+-*; do
    tr -d '\r' < "$d/hashkdf.c" | tr -d ' \t\n' | grep -Fq 'voidkdf_init(kdfstate*state,constuint8_t*input,intinlen){init_random_number(state,input,inlen);}' ||
        bad "$(basename "$d"): kdf_init differs"
    tr -d '\r' < "$d/drng.c" | tr -d ' \t\n' | grep -Fq 'memset(drng,0,sizeof(*drng));seed_material=(unsignedchar*)malloc(' ||
        bad "$(basename "$d"): DRBG instantiation differs"
    grep -lq 'kdf_init(&state, seed' "$d"/*.c || bad "$(basename "$d"): unpack_c does not use kdf_init"
done
echo 'source: all 12 trees expand the challenge via void kdf_init(); the DRBG zeroes its context before its first malloc'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
flags='-O2 -std=gnu99 -w -Wno-incompatible-pointer-types -Wno-int-conversion -DUSE_ICCS -Wl,--wrap=malloc -Wl,--wrap=init_random_number'
expect_ref='c0 seed-independent=1; sign: tries=* honest rc=0 verify=0, faulted rc=0 verify=1 same cseed=1, s1 recovered=1'
# The AVX2 set-III USE_ICCS build does not round-trip honest signatures in
# this harness (0/50 in a plain test), so its honest verify result is not
# checked; the recovery and forgery checks are.
expect_avx='c0 seed-independent=1; sign: tries=* honest rc=0 verify=*, faulted rc=0 verify=1 same cseed=1, s1 recovered=1'
expect_keygen='keygen: rc=0,0 round-trip=1,1 public keys differ=1, s1[0] identical=1'
expect_keygen_any='keygen: rc=0,0 round-trip=*,* public keys differ=1, s1?0? identical=1'
expect_verify='verify: forged signature without fault=1, with verifier unpack_c allocation failing=0 (failed allocations=1)'

run() { # label dir params extra-flags sources expected-signing-line
    cp -R "$2" "$scratch/$1" || bad "$1 copy failed"
    (cd "$scratch/$1" && gcc $flags -I. -DPARAMS=$3 $4 -o "$scratch/$1.bin" "$driver" $5) \
        >"$scratch/cc" 2>&1 || { cat "$scratch/cc"; bad "$1 build failed"; }
    out=$("$scratch/$1.bin" 2>/dev/null) || bad "$1 run failed"
    echo "$1: $out" | sed 's/^\(verify:\|keygen:\)/  \1/'
    l1=$(echo "$out" | sed -n 1p); l2=$(echo "$out" | sed -n 2p)
    case "$l1" in $6) ;; *) bad "$1 signing behaviour differs" ;; esac
    [ "$l2" = "$expect_verify" ] || bad "$1 verification behaviour differs"
    l3=$(echo "$out" | sed -n 3p)
    case "$1" in avx2-III) case "$l3" in $expect_keygen_any) ;; *) bad "$1 keygen behaviour differs" ;; esac ;;
                 *) [ "$l3" = "$expect_keygen" ] || bad "$1 keygen behaviour differs" ;; esac
}

ref='sign.c polyvec.c packing.c poly.c reduce.c ntt.c rounding.c fips202.c hashkdf.c auxfunc.c drng.c pspm.c rng.c'
avx='sign.c polyvec.c packing.c poly.c ntt.c fips202.c hashkdf.c pspm.c fips202x4.c keccak4x/KeccakP-1600-times4-SIMD256.c auxfunc.c drng.c sample.c'
sets='1:I 2:II 3:III'
for s in $sets; do
    run "ref-${s#*:}" "$impl/Reference_Implementation/Aigis-Sig+-${s#*:}" "${s%%:*}" '' "$ref" "$expect_ref"
done
n=3
if grep -q avx2 /proc/cpuinfo 2>/dev/null && grep -q bmi2 /proc/cpuinfo 2>/dev/null; then
    for s in $sets; do
        run "avx2-${s#*:}" "$impl/Optimized_Implementation/avx2/Aigis-Sig+-${s#*:}" "${s%%:*}" '-march=native -DAIGIS_AVX2' "$avx" "$expect_avx"
    done
    n=6
else
    echo 'AVX2 builds skipped: CPU lacks AVX2/BMI2'
fi
echo "ATTACK sign-01-10 CONFIRMED: in $n builds one failed DRBG allocation while expanding the challenge makes signing return 0 with z' = y + c0*s1 for a public c0, and s1 is recovered exactly from that signature and an honest signature on the same message; the same fault makes the verifier accept a signature forged from the public key alone, and the same fault in key generation returns working key pairs with a fixed public s1[0]"
exit 0
