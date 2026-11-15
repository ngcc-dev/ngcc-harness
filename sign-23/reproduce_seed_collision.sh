#!/bin/sh
# sign-23-3 Shuttle: the lambda-bit masking seed seed_y exposes the signing key
# by seed collisions (Shuttle-128, within the 2^80 budget) or by a multi-target
# seed search (all three levels, about 2^lambda/Q guesses for Q signatures).
#
# Builds the submitted SHUTTLE-128 reference tree in a temporary copy, once
# with seed_y reduced to NGCC_SEED_BITS bits (the only change) and
# once unmodified (control). Runs (1) the collision route: public commitment
# matches -> s1 -> public completion -> fresh-message forgery; (2) the
# multi-target route: one pk-only test per seed guess against every signature
# (real enumeration at reduced width, the identity at full width) -> exact s1.
# Default 20 bits / 4096 signatures (256 for the multi-target table); NGCC_LONG=1
# runs 24 bits / 16384 signatures. The multi-target route is then repeated on
# the SHUTTLE-256 and SHUTTLE-512 reference trees with a 16-bit seed and 256
# signatures, together with the identity at their shipped 256- and 512-bit
# widths.
set -u
cd "$(dirname "$0")/.." || exit 2
id=sign-23-3
command -v gcc >/dev/null 2>&1 || { echo "SKIP $id: gcc unavailable"; exit 77; }

impl="sign-23/Implementation codes and test vectors/Shuttle 算法实现源代码及测试向量/Implementations"
ref="$impl/Reference_Implementation/SHUTTLE-128"
[ -f "$ref/polyvec.c" ] || { echo "NOT CONFIRMED $id: $ref missing"; exit 1; }
for tree in Reference_Implementation Optimized_Implementation Additional_Implementation; do
    for level in 128 256 512; do
        grep -q '^#define SEEDBYTES (LAMBDA / 8)' "$impl/$tree/SHUTTLE-$level/params.h" &&
        grep -q 'xof256_squeeze(&ctx, seedY, SEEDBYTES);' "$impl/$tree/SHUTTLE-$level/polyvec.c" ||
            { echo "NOT CONFIRMED $id: $tree SHUTTLE-$level seed_y width changed"; exit 1; }
    done
done
echo "source: seed_y = ExpandSigningSeeds(K||rnd||mu||kappa) squeezes SEEDBYTES = LAMBDA/8 = 16, 32 or 64 bytes in all nine trees"

if [ "${NGCC_LONG:-0}" = 1 ]; then bits=${NGCC_SEED_BITS:-24}; nsig=${NGCC_SIGNATURES:-16384}
else bits=${NGCC_SEED_BITS:-20}; nsig=${NGCC_SIGNATURES:-4096}; fi
mt=${NGCC_MT_SIGNATURES:-256}

tmp=$(mktemp -d) || exit 2
trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM
cp -rL "$ref" "$tmp/src" || exit 2
cp sign-23/reproduce_seed_collision.c "$tmp/src/ngcc_driver.c" || exit 2

# Insert the reduced-width truncation directly after seed_y is squeezed.
awk '
    { print }
    /xof256_squeeze\(&ctx, seedY, SEEDBYTES\);/ {
        print "#ifdef NGCC_SEED_BITS"
        print "    { /* reduced-width model only: keep NGCC_SEED_BITS bits of seed_y */"
        print "        unsigned kb = NGCC_SEED_BITS / 8, rb = NGCC_SEED_BITS % 8;"
        print "        if (rb) seedY[kb++] &= (uint8_t)((1u << rb) - 1);"
        print "        memset(seedY + kb, 0, SEEDBYTES - kb); }"
        print "#endif"
        n++
    }
    END { if (n != 1) exit 1 }
' "$ref/polyvec.c" >"$tmp/src/polyvec.c" || { echo "NOT CONFIRMED $id: patch anchor not found"; exit 1; }

s=$tmp/src
srcs=""
for f in SIG_AlgorithmInstance.c polyvec.c sampler.c sampler_u.c irs.c rounding.c packing.c poly.c \
         poly_ntt.c reduce.c rans.c approx_exp.c approx_log.c symmetric.c drng.c auxfunc.c ntt/q15361n256/ntt_ref.c; do
    srcs="$srcs $s/$f"
done
cflags="-O2 -std=gnu11 -fwrapv -fno-strict-aliasing -D_GNU_SOURCE -DSHUTTLE_MODE=128 -w -I$s -I$s/ntt/q15361n256"
# shellcheck disable=SC2086
gcc $cflags -DNGCC_SEED_BITS="$bits" -o "$tmp/reduced" "$s/ngcc_driver.c" $srcs -lm || exit 2
# shellcheck disable=SC2086
gcc $cflags -o "$tmp/full" "$s/ngcc_driver.c" $srcs -lm || exit 2

step() { label=$1; out=$2; shift 2
    echo "== $label =="
    "$@" >"$out" 2>&1 || { cat "$out"; echo "NOT CONFIRMED $id: driver failed ($label)"; exit 1; }
    cat "$out"
}
step "collision route, reduced-width seed_y: $bits bits, $nsig signatures" "$tmp/c1" "$tmp/reduced" collide "$nsig"
step "control: shipped 128-bit seed_y, same key and DRBG stream, $nsig signatures" "$tmp/c2" "$tmp/full" collide "$nsig"
step "multi-target route, reduced-width seed_y: $bits bits, $mt signatures" "$tmp/m1" "$tmp/reduced" mt "$mt"
step "multi-target identity at the shipped 128-bit width, 4 signatures" "$tmp/m2" "$tmp/full" mt 4

get() { grep "^RESULT $2" "$1" | tr ' ' '\n' | sed -n "s/^$3=//p"; }
m=$(get "$tmp/c1" matches= matches); c=$(get "$tmp/c1" matches= completed)
fv=$(get "$tmp/c1" matches= forgery); cv=$(get "$tmp/c1" matches= changed)
eqk=$(get "$tmp/c1" matches= s1_equal); nm=$(get "$tmp/c1" nonmatching_pair completion)
fm=$(get "$tmp/c2" matches= matches)
id1=$(sed -n 's/^RESULT mt_identity=//p' "$tmp/m1"); id2=$(sed -n 's/^RESULT mt_identity=//p' "$tmp/m2")
mg=$(get "$tmp/m1" mt_enum guesses); mf=$(get "$tmp/m1" mt_enum found)
mc=$(get "$tmp/m1" mt_enum completion); mfv=$(get "$tmp/m1" mt_enum forgery)
mcv=$(get "$tmp/m1" mt_enum changed); meq=$(get "$tmp/m1" mt_enum s1_equal)

python3 - "$nsig" "$bits" "$m" "$c" <<'EOF' || true
import math, sys
q, w, m, c = (int(x) for x in sys.argv[1:5])
print(f"measured: {m} matches ({q*(q-1)/2/2**w:.1f} expected seed collisions), {c} completed keys")
f = 0.91      # conservative usable fraction (20 of 21 matches completed at 24 bits; d0 must be invertible mod q)
lam = lambda Q: f * Q * Q / 2**129
print(f"128-bit model with usable fraction {f}: P[success at 2^64] = {1-math.exp(-lam(2**64)):.3f}; "
      f"50% at 2^{64 + 0.5*math.log2(2*math.log(2)/f):.2f}; expected usable pairs at 2^80 = 2^{math.log2(lam(2**80)):.2f}; "
      f"multi-target search with Q = 2^64 signatures: about 2^{128-64} seed guesses")
EOF

if ! { [ "${c:-0}" -ge 1 ] && [ "$fv" = 0 ] && [ "$cv" != 0 ] && [ "$eqk" = 1 ] && [ "${nm:-0}" != 0 ] &&
   [ "${fm:-1}" = 0 ] && [ "$id1" = 1 ] && [ "$id2" = 1 ] && [ "${mf:--1}" -ge 0 ] &&
   [ "$mc" = 0 ] && [ "$mfv" = 0 ] && [ "$mcv" != 0 ] && [ "$meq" = 1 ]; }; then
    echo "NOT CONFIRMED $id: matches=$m completed=$c forgery=$fv changed=$cv s1=$eqk nonmatch=$nm full=$fm id=$id1/$id2 mt=$mf/$mc/$mfv/$mcv/$meq"
    exit 1
fi

# Multi-target route on the higher levels: same driver and patch, level-specific build.
hbits=${NGCC_HIGH_SEED_BITS:-16}
high=""
for lq in 256:q61441n512 512:q59393n1024; do
    level=${lq%:*}; qset=${lq#*:}
    s=$tmp/src$level
    cp -rL "$impl/Reference_Implementation/SHUTTLE-$level" "$s" || exit 2
    cp sign-23/reproduce_seed_collision.c "$s/ngcc_driver.c" || exit 2
    cp "$tmp/src/polyvec.c" "$s/polyvec.c" || exit 2     # polyvec.c is identical in the three trees
    cmp -s "$ref/polyvec.c" "$impl/Reference_Implementation/SHUTTLE-$level/polyvec.c" ||
        { echo "NOT CONFIRMED $id: SHUTTLE-$level polyvec.c differs from SHUTTLE-128"; exit 1; }
    srcs=""
    for f in SIG_AlgorithmInstance.c polyvec.c sampler.c sampler_u.c irs.c rounding.c packing.c poly.c \
             poly_ntt.c reduce.c rans.c approx_exp.c approx_log.c symmetric.c drng.c auxfunc.c ntt/$qset/ntt_ref.c; do
        srcs="$srcs $s/$f"
    done
    cflags="-O2 -std=gnu11 -fwrapv -fno-strict-aliasing -D_GNU_SOURCE -DSHUTTLE_MODE=$level -w -I$s -I$s/ntt/$qset"
    # shellcheck disable=SC2086
    gcc $cflags -DNGCC_SEED_BITS="$hbits" -o "$tmp/reduced$level" "$s/ngcc_driver.c" $srcs -lm || exit 2
    # shellcheck disable=SC2086
    gcc $cflags -o "$tmp/full$level" "$s/ngcc_driver.c" $srcs -lm || exit 2
    step "SHUTTLE-$level multi-target route, reduced-width seed_y: $hbits bits, $mt signatures" "$tmp/h1" "$tmp/reduced$level" mt "$mt"
    step "SHUTTLE-$level multi-target identity at the shipped $level-bit width, 4 signatures" "$tmp/h2" "$tmp/full$level" mt 4
    hid=$(sed -n 's/^RESULT mt_identity=//p' "$tmp/h2")
    hg=$(get "$tmp/h1" mt_enum guesses); hf=$(get "$tmp/h1" mt_enum found)
    hc=$(get "$tmp/h1" mt_enum completion); hfv=$(get "$tmp/h1" mt_enum forgery)
    hcv=$(get "$tmp/h1" mt_enum changed); heq=$(get "$tmp/h1" mt_enum s1_equal)
    if ! { [ "$hid" = 1 ] && [ "${hf:--1}" -ge 0 ] && [ "$hc" = 0 ] && [ "$hfv" = 0 ] && [ "$hcv" != 0 ] && [ "$heq" = 1 ]; }; then
        echo "NOT CONFIRMED $id: SHUTTLE-$level id=$hid mt=$hf/$hc/$hfv/$hcv/$heq"
        exit 1
    fi
    high="$high SHUTTLE-$level after $hg guesses;"
done
echo "multi-target model: about 2^lambda/Q seed guesses for Q signatures, so 2^64, 2^192 and 2^448 at Q = 2^64"

echo "CONFIRMED $id: at $bits-bit seed_y, $c/$m commitment matches and a multi-target hit after $mg guesses each gave the exact Shuttle-128 s1 and an accepted fresh-message forgery; changed message, non-matching pair and flipped seed rejected; full-width control 0 matches, full-width identity holds. At $hbits-bit seed_y the multi-target route also recovered s1 and forged on$high their full-width identities hold"
exit 0
