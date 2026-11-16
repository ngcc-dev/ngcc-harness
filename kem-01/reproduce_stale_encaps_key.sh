#!/bin/sh
# kem-01-5: Aigis-Enc+ mkem_enc discards the status of Hash2 and copies the
# uninitialised stack buffer kr into the shared secret and the coins. Called
# again from the same place, it returns the previous session's key. Hash2 is
# pseudohash at levels II and III; at level I it is sm3hash, which does not
# allocate, so level I is not affected.
#
# Checks mkem_enc and the Hash2 mapping in all twelve trees, then builds the
# reference and AVX2 trees with malloc wrapped so that exactly one allocation
# of the size Hash2's pseudohash requests fails. Alice encapsulates to
# Mallory, then to Bob with the failure: Mallory's own decapsulated key must
# equal the Alice-Bob key at II and III. Controls: no failure; the failure
# after the stack is overwritten; and level I, where no allocation fails.
set -u
here=$(cd "$(dirname "$0")" && pwd)
[ -d kem-01/Implementations ] || cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-01-5: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-01-5: $1"; exit 1; }

impl=kem-01/Implementations
for d in "$impl"/Reference_Implementation/Aigis-Enc+-* "$impl"/Optimized_Implementation/avx2/Aigis-Enc+-* \
         "$impl"/Additional_Implementation/neon/Aigis-Enc+-* "$impl"/Additional_Implementation/aarch64/Aigis-Enc+-*; do
    grep -q '^[ 	]*uint8_t  kr\[2\*SEED_BYTES\];' "$d/kem.c" &&
        grep -q '^[ 	]*Hash2(kr, buf, 2\*SEED_BYTES);' "$d/kem.c" &&
        grep -q '^[ 	]*memcpy(ss, kr, SEED_BYTES);' "$d/kem.c" &&
        grep -Fq '#define Hash2 hash512' "$d/api.h" && grep -Fq '#define Hash2 hash1024' "$d/api.h" &&
        grep -Fq 'pseudohash(512, in, inlen * 8, out);' "$d/hashkdf.c" ||
        bad "$d differs from the archive"
done
echo 'source: all twelve trees leave kr uninitialised and ignore Hash2 failures in mkem_enc'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
for tree in ref avx2; do
    for p in 1 2 3; do
        n=$(echo I II III | cut -d' ' -f$p)
        if [ $tree = ref ]; then
            d=$impl/Reference_Implementation/Aigis-Enc+-$n
            src='gen_a.c reduce.c kat_test/auxfunc.c kat_test/KEM_AlgorithmInstance.c kat_test/drng.c kat_test/rng.c'
            fl=
        else
            grep -qw avx2 /proc/cpuinfo 2>/dev/null || continue
            d=$impl/Optimized_Implementation/avx2/Aigis-Enc+-$n
            src='genmatrix.c auxfunc.c KEM_AlgorithmInstance.c drng.c rng.c'
            fl='-mavx2 -mbmi2 -mpopcnt -maes -Wno-incompatible-pointer-types'
        fi
        rm -f "$scratch/t"
        (cd "$d" && gcc -O2 $fl -w -I. -Ikat_test -DPARAMS=$p -DUSE_NICCS_API -Wl,--wrap=malloc -o "$scratch/t" \
            "$here/reproduce_stale_encaps_key.c" cbd.c fips202.c poly.c precomp.c ntt.c polyvec.c owcpa.c \
            kem.c verify.c hashkdf.c pack.c $src) 2>"$scratch/cc" ||
            { cat "$scratch/cc"; bad "$tree Aigis-Enc+-$n build failed"; }
        out=$("$scratch/t" 2>/dev/null) || bad "$tree Aigis-Enc+-$n run failed"
        echo "$tree Aigis-Enc+-$n: $(echo "$out" | tr '\n' ' ')"
        case "$out" in *'control: rc=0,0 alloc=0 mallory_has_M=1 mallory_predicts_AB=0 bob_agrees=1'*) ;; *) bad "$tree Aigis-Enc+-$n control differs" ;; esac
        if [ $p = 1 ]; then
            case "$out" in *'faulted: rc=0,0 alloc=0 mallory_has_M=1 mallory_predicts_AB=0 bob_agrees=1'*) ;; *) bad "$tree Aigis-Enc+-I unexpectedly faulted" ;; esac
        else
            case "$out" in *'faulted: rc=0,0 alloc='[1-9]*' mallory_has_M=1 mallory_predicts_AB=1 bob_agrees=0'*) ;; *) bad "$tree Aigis-Enc+-$n faulted run differs" ;; esac
            case "$out" in *'scrubbed: rc=0,0 alloc='[1-9]*' mallory_has_M=1 mallory_predicts_AB=0 bob_agrees=0'*) ;; *) bad "$tree Aigis-Enc+-$n scrubbed run differs" ;; esac
        fi
    done
done
echo 'ATTACK kem-01-5: at Aigis-Enc+-II and -III (reference and AVX2) a failed Hash2 allocation makes encapsulation return 0 with the previous session key, which an earlier recipient already holds: CONFIRMED'
exit 0
