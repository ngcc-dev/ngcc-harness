#!/bin/sh
# sign-22-8: Rhyme-SM3 samples binary challenges but keeps weights sized for
# ternary challenges, so its challenge space is below every claimed level.
#
# 1. Source: all sixteen trees share one binary SampleChallenge (each of TAU
#    positions is set to 1, no sign bit), and the verifier re-derives c from
#    the transmitted c_tilde with it. Rhyme-SM3 params_tables.h sets
#    TAU = 23/44/77/87; Rhyme-SHAKE sets 30/58/113/115, the values of the
#    specification's Table 3 (physical p. 53).
# 2. Runtime: every tree's compiled sampler is run on 20,000 hashes and must
#    give {0,1} challenges of weight exactly TAU; log2 binomial(N,TAU) is
#    printed for each set.
# 3. Toy forgery: a copy of the reference Rhyme-SM3-128 sources in which only
#    RHYME_TAU is changed from 23 to 2 (binomial(256,2) = 2^14.99). Without
#    the signing key, the forger fixes a short z and a target challenge c*,
#    computes w = A z - q j c* mod 2q from the public key, and hashes fresh
#    messages until SampleChallenge(H(w, H(pk, m))) = c*. The unmodified
#    verifier must accept the result and reject it for another message.
#    Control: the unmodified TAU = 23 build finds no match in 100,000 trials.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-22-8: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-22-8: $1"; exit 1; }

impl='sign-22/Implementations and Test_Vectors/Implementations'
[ -d "$impl" ] || bad "implementation tree not found"
driver="$here/reproduce_challenge_space.c"
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM

avx2=0
grep -qw avx2 /proc/cpuinfo 2>/dev/null && avx2=1
trees='Reference'
[ $avx2 = 1 ] && trees='Reference Optimized'

# ---- 1. source
for t in Reference Optimized; do
    for f in SM3 SHAKE; do
        for l in 128 256 384 512; do
            d="$impl/${t}_Implementation/Rhyme-$f/Rhyme-$f-$l"
            s="$d/src/sampler.c"
            body=$(awk '/^void SampleChallenge\(/,/^}/' "$s" | tr -d '\r')
            printf '%s\n' "$body" | grep -Fxq '        c->coeffs[j] = 1;' || bad "$t $f-$l: challenge assignment differs"
            printf '%s\n' "$body" | grep -Eqw 'signs?|1 *- *2|= *-1' && bad "$t $f-$l: SampleChallenge has sign handling"
            grep -Fq 'SampleChallenge(&c, c_tilde);' "$d/src/sign.c" &&
                grep -Fq 'memcmp(c_tilde, c_tilde2, CTILDEBYTES)' "$d/src/sign.c" ||
                bad "$t $f-$l: verifier challenge path differs"
            taus=$(grep '^#define RHYME_TAU ' "$d/include/params_tables.h" | tr -d '\r' | awk '{printf "%s ", $3}')
            case $f in
                SM3) [ "$taus" = '23 44 77 87 ' ] || bad "$t $f-$l: TAU table is $taus" ;;
                SHAKE) [ "$taus" = '30 58 113 115 ' ] || bad "$t $f-$l: TAU table is $taus" ;;
            esac
        done
    done
done
echo 'source: 16 trees set each challenge position to 1 with no sign; verifier re-derives c from c_tilde;'
echo '        Rhyme-SM3 TAU = 23 44 77 87, Rhyme-SHAKE TAU = 30 58 113 115 (specification Table 3)'

# ---- 2. runtime alphabet in every tree
core='src/poly.c src/ntt.c src/ntt_tables.c src/sampler.c src/encoding.c src/packing.c
      src/zpntt.c src/symmetric-shake.c rhyme_xof.c src/keygen/kg_main.c src/keygen/kg_solver.c
      src/keygen/kg_zint.c src/keygen/kg_ntt.c src/keygen/kg_primes.c randombytes.c auxfunc.c drng.c'
build() { # dir tree family level output
    extra=''; flags=''
    case $3 in SM3) extra='sm3.c sm3_xof.c' ;; SHAKE) extra='src/fips202.c' ;; esac
    case "$2$3" in
        OptimizedSM3) extra="$extra simd/sm3_avx2.c"
            # sm3.c calls rhyme_sm3_hash_avx2 without a prototype (an error since GCC 14)
            flags='-mavx2 -DUSE_AVX2_SM3 -Wno-error=implicit-function-declaration' ;;
        OptimizedSHAKE) flags='-march=native -DUSE_DP1_CHECK' ;;
    esac
    (cd "$1" && gcc -O2 -std=c99 -w -I. -Iinclude -Isrc/keygen -DRHYME_NO_AES -DRHYME_MODE="$4" \
        -DOUTPUT_BLANK_TEST_VECTORS=0 $flags -o "$5" "$driver" $core $extra -lm) >"$5.log" 2>&1
}
for t in $trees; do
    for f in SM3 SHAKE; do
        for l in 128 256 384 512; do
            build "$impl/${t}_Implementation/Rhyme-$f/Rhyme-$f-$l" "$t" "$f" "$l" "$scratch/$t-$f-$l" &
        done
    done
done
mkdir "$scratch/toy" || exit 2
cp -R "$impl/Reference_Implementation/Rhyme-SM3/Rhyme-SM3-128/." "$scratch/toy/" || exit 2
sed 's/^#define RHYME_TAU 23\([^0-9]*\)$/#define RHYME_TAU 2\1/' \
    "$scratch/toy/include/params_tables.h" >"$scratch/pt" && mv "$scratch/pt" "$scratch/toy/include/params_tables.h"
build "$scratch/toy" Reference SM3 128 "$scratch/toy-SM3-128" &
wait

for t in $trees; do
    for f in SM3 SHAKE; do
        for l in 128 256 384 512; do
            b="$scratch/$t-$f-$l"
            [ -x "$b" ] || { cat "$b.log"; bad "$t $f-$l build failed"; }
            out=$("$b" alphabet 20000) || bad "$t $f-$l: $out"
            case "$f-$l" in
                SM3-128) want='N=256 TAU=23 samples=20000 non_binary=0 wrong_weight=0 log2C=108.08' ;;
                SM3-256) want='N=512 TAU=44 samples=20000 non_binary=0 wrong_weight=0 log2C=212.46' ;;
                SM3-384) want='N=512 TAU=77 samples=20000 non_binary=0 wrong_weight=0 log2C=308.40' ;;
                SM3-512) want='N=1024 TAU=87 samples=20000 non_binary=0 wrong_weight=0 log2C=425.00' ;;
                SHAKE-128) want='N=256 TAU=30 samples=20000 non_binary=0 wrong_weight=0 log2C=129.74' ;;
                SHAKE-256) want='N=512 TAU=58 samples=20000 non_binary=0 wrong_weight=0 log2C=256.81' ;;
                SHAKE-384) want='N=512 TAU=113 samples=20000 non_binary=0 wrong_weight=0 log2C=385.30' ;;
                SHAKE-512) want='N=1024 TAU=115 samples=20000 non_binary=0 wrong_weight=0 log2C=514.33' ;;
            esac
            [ "$out" = "mode=$l $want" ] || bad "$t $f-$l: $out"
            echo "$t Rhyme-$f-$l: $out"
        done
    done
done
[ $avx2 = 1 ] || echo 'optimized trees not run: CPU lacks AVX2 (source checks above still cover them)'
echo 'Rhyme-SM3 shortfall: 128-108.08=19.92, 256-212.46=43.54, 384-308.40=75.60, 512-425.00=87.00 bits'

# ---- 3. toy forgery and control
diff -r "$impl/Reference_Implementation/Rhyme-SM3/Rhyme-SM3-128" "$scratch/toy" >"$scratch/diff"
[ "$(grep -c '^[<>]' "$scratch/diff")" = 2 ] && grep -q '^> #define RHYME_TAU 2' "$scratch/diff" ||
    bad "toy copy differs in more than RHYME_TAU"
[ -x "$scratch/toy-SM3-128" ] || { cat "$scratch/toy-SM3-128.log"; bad "toy build failed"; }
echo 'toy: reference Rhyme-SM3-128 with only "#define RHYME_TAU 23" changed to 2'
out=$("$scratch/toy-SM3-128" forge 2000000) || bad "toy forgery: $out"
echo "toy:     $out"
case "$out" in
    'mode=128 TAU=2 log2C=14.99 honest_verify=0 honest_weight=2 trials='*' forged_verify=0 other_message_verify=-1') ;;
    *) bad "toy forgery not accepted" ;;
esac
out=$("$scratch/Reference-SM3-128" forge 100000); rc=$?
echo "control: $out"
[ $rc = 3 ] && [ "$out" = 'mode=128 TAU=23 log2C=108.08 honest_verify=0 honest_weight=23 trials=100000 forged=none' ] ||
    bad "unmodified TAU=23 control differs"
echo "ATTACK sign-22-8: Rhyme-SM3 binary challenges keep ternary weights (2^108.08, 2^212.46, 2^308.40, 2^425.00 challenges; generic forgery demonstrated at toy TAU=2): CONFIRMED"
exit 0
