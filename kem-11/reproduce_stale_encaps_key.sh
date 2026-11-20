#!/bin/sh
# kem-11-3: COMPASS-KEM's crypto_kem_enc_derand discards the status of hash_g
# and copies the uninitialised stack buffer kr into the shared secret and the
# coins. Called again from the same place, it returns the previous session's
# key.
#
# Checks the uninitialised kr and the unchecked hash_g in all eight trees,
# then builds each with malloc wrapped so that exactly one allocation of the
# size pseudohash_512 requests for hash_g fails. Alice encapsulates to
# Mallory, then to Bob with the failure: Mallory's own decapsulated key must
# equal the Alice-Bob key. Controls: no failure, and the failure after the
# stack is overwritten between the sessions. Flags follow each tree's Makefile.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-11-3: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-11-3: $1"; exit 1; }

impl='kem-11/Implementations and Test_Vectors/Implementations'
[ -d "$impl" ] || bad "implementation tree not found"
for t in Reference Optimized; do
    for l in 128 256 384 512; do
        d="$impl/${t}_Implementation/COMPASS-KEM-$l"
        tr -d '\r' <"$d/kem.c" | grep -Fxq '  uint8_t kr[2*COMPASS_KEM_SYMBYTES];' &&
            tr -d '\r' <"$d/kem.c" | grep -Fxq '  hash_g(kr, buf, 2*COMPASS_KEM_SYMBYTES);' &&
            tr -d '\r' <"$d/kem.c" | grep -Fxq '  memcpy(ss,kr,COMPASS_KEM_SYMBYTES);' &&
            tr -d '\r' <"$d/symmetric.h" | grep -Fxq '  pseudohash(512, (const unsigned char *)(IN), (unsigned long long)(INBYTES) * 8, (OUT))' ||
            bad "$t COMPASS-KEM-$l differs from the archive"
        # the archived Makefile's level and flags; checked when present (the harness copy has none)
        if [ -f "$d/Makefile" ]; then
            grep -Fq -- "-DSECURITY_LEVEL=$l" "$d/Makefile" && grep -Fq -- '-O3 -fomit-frame-pointer' "$d/Makefile" ||
                bad "$t COMPASS-KEM-$l Makefile flags differ from the archived ones"
        fi
    done
done
echo 'source: all eight trees leave kr uninitialised and ignore hash_g failures in encapsulation'

avx2=0
grep -qw avx2 /proc/cpuinfo 2>/dev/null && grep -qw bmi2 /proc/cpuinfo 2>/dev/null && avx2=1
trees='Reference'
[ $avx2 = 1 ] && trees='Reference Optimized'
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
n=0
for t in $trees; do
    for l in 128 256 384 512; do
        d="$impl/${t}_Implementation/COMPASS-KEM-$l"
        if [ $t = Reference ]; then
            src='kem.c indcpa.c polyvec.c poly.c ntt.c cbd.c reduce.c verify.c drng.c auxfunc.c symmetric-shake.c'
            x=''
        else
            src='kem.c indcpa.c polyvec.c poly.c ntt.c cbd.c reduce.c verify.c consts.c rejsample.c drng.c auxfunc.c symmetric-shake.c fq.S'
            x='-mavx2 -mbmi2 -mpopcnt -march=native'
        fi
        (cd "$d" && gcc -O3 -fomit-frame-pointer -w $x -DSECURITY_LEVEL=$l -I. -Wl,--wrap=malloc \
            -o "$scratch/t" "$here/reproduce_stale_encaps_key.c" KEM_AlgorithmInstance.c $src) 2>"$scratch/cc" ||
            { cat "$scratch/cc"; bad "$t COMPASS-KEM-$l build failed"; }
        out=$("$scratch/t" 2>/dev/null) || bad "$t COMPASS-KEM-$l run failed"
        echo "$t COMPASS-KEM-$l: $(echo "$out" | tr '\n' ' ')"
        case "$out" in *'control: rc=0,0 alloc=0 mallory_has_M=1 mallory_predicts_AB=0 bob_agrees=1'*) ;; *) bad "$t COMPASS-KEM-$l control differs" ;; esac
        case "$out" in *'faulted: rc=0,0 alloc=1 mallory_has_M=1 mallory_predicts_AB=1 bob_agrees=0'*) ;; *) bad "$t COMPASS-KEM-$l faulted run differs" ;; esac
        case "$out" in *'scrubbed: rc=0,0 alloc=1 mallory_has_M=1 mallory_predicts_AB=0 bob_agrees=0'*) ;; *) bad "$t COMPASS-KEM-$l scrubbed run differs" ;; esac
        n=$((n + 1))
    done
done
[ $avx2 = 1 ] || echo 'optimized trees not run: CPU lacks AVX2/BMI2 (source checks above still cover them)'
echo "ATTACK kem-11-3: in $n COMPASS-KEM builds a failed hash_g allocation makes kem_enc return 0 with the previous session key, which an earlier recipient already holds: CONFIRMED"
exit 0
