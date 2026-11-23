#!/bin/sh
# kem-31-7: theta_chain_comput_strategy allocates the step array of each
# dimension-two isogeny chain with malloc (theta_isogenies.c:1472 in the
# reference tree, :1486 in the optimized tree) and nothing frees it. Key
# generation computes two chains (pike_compressed.c:215,253) and decryption
# one (:968), so every kem_keygen leaks two arrays and every kem_dec one.
#
# Checks the allocation and its callers in both source trees, then builds the
# reference tree at NGCC_1/2/3 in a temporary directory with the uniform
# build's flags and no sanitizer, with malloc and free wrapped at link time.
# The driver runs two key generations, two encapsulations, two decapsulations
# and one decapsulation of a ciphertext with its last byte flipped, and counts
# the live heap blocks and bytes after each call. Expected: two more arrays
# per key generation, one per decapsulation (accepted or implicitly rejected)
# and none per encapsulation (control); freeing the recorded arrays leaves
# nothing live. Control: a fixed NGCC_1 build, with free(hd_isog.steps) added
# after the last use of each chain, leaves nothing live and still agrees. The
# optimized tree is also run at NGCC_1, with -DNDEBUG as its README builds it,
# when the CPU has BMI2 and ADX.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-31-7: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-31-7: $1"; exit 1; }

ref=kem-31/Implementations/Implementations
opt=kem-31/Implementations/Optimized_Implementation
[ -d "$ref" ] && [ -d "$opt" ] || bad "implementation trees not found"
driver="$here/reproduce_keygen_leak.c"
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM

src() { tr -d '\r' <"$1"; }
for t in "$ref" "$opt"; do
    n=1472
    [ "$t" = "$opt" ] && n=1486
    f=$t/src/hd/ref/hdx/theta_isogenies.c
    src "$f" | sed -n ${n}p | grep -Fxq '    out->steps = malloc((n - 1) * sizeof(theta_isogeny_t));' ||
        bad "$f:$n differs from the archive"
    p=$t/src/pike/ref/pikex_compressed/pike_compressed.c
    [ "$(src "$p" | grep -n 'theta_chain_comput_strategy(&hd_isog,' | cut -d: -f1 | tr '\n' ' ')" = '215 253 968 ' ] ||
        bad "$p: callers differ from the archive"
    [ "$(cat "$t"/src/hd/ref/hdx/*.c "$p" | grep -c 'free(')" = 0 ] || bad "$t: a free() was found in hd/ or pike_compressed.c"
    [ "$(src "$p" | sed -n '531p;532p;1028p;1029p' | tr -d ' \n')" = 'quat_left_ideal_finalize(&ideal);return1;ibz_finalize(&res);return1;' ] ||
        bad "$p: keygen or decrypt ends differ from the archive"
done
echo 'source: theta_isogenies.c:1472 (reference) and :1486 (optimized) malloc the step array; keygen calls it at pike_compressed.c:215,253 and decrypt at :968; no free() in either tree'

printf '#include <gmp.h>\nint main(void){mpz_t a;mpz_init(a);mpz_clear(a);return 0;}\n' >"$scratch/t.c"
gcc -o "$scratch/t" "$scratch/t.c" -lgmp >/dev/null 2>&1 && "$scratch/t" ||
    { echo 'SKIP kem-31-7: GMP unavailable (source checks passed)'; exit 77; }
withopt=0
case $(uname -m) in x86_64 | amd64)
    grep -qw bmi2 /proc/cpuinfo 2>/dev/null && grep -qw adx /proc/cpuinfo 2>/dev/null && withopt=1 ;;
esac

common='src/ngcc/common/KEM_AlgorithmInstance.c src/ngcc/common/drng.c src/ngcc/common/auxfunc.c
    src/common/generic/aes_c.c src/common/generic/fips202.c src/common/generic/pike_hash.c
    src/common/generic/tools.c src/intbig/ref/generic/intbig.c
    src/quaternion/ref/generic/algebra.c src/quaternion/ref/generic/ideal.c
    src/quaternion/ref/generic/dim4.c src/quaternion/ref/generic/dim2.c
    src/quaternion/ref/generic/integers.c src/quaternion/ref/generic/lattice.c
    src/quaternion/ref/generic/finit.c src/quaternion/ref/generic/printer.c
    src/quaternion/ref/generic/lll.c src/quaternion/ref/generic/matkermod.c
    src/klpt/ref/klptx/tools.c src/gf/ref/gfx/fp2.c src/gf/ref/gfx/mp.c
    src/ec/ref/pike_ecx/ec.c src/ec/ref/ecx/kps.c src/ec/ref/ecx/xisog.c src/ec/ref/ecx/xeval.c
    src/ec/ref/ecx/isog_chains.c src/ec/ref/pike_ecx/basis.c src/ec/ref/ecx/biextension.c
    src/hd/ref/hdx/hd.c src/hd/ref/hdx/theta_structure.c src/hd/ref/hdx/theta_isogenies.c
    src/id2iso/ref/id2isox/id2iso.c'
pc=src/pike/ref/pikex_compressed/pike_compressed.c
# the fix: free the step array before the second chain of keygen and at the end of keygen and decrypt
src "$ref/$pc" | awk 'NR == 253 || NR == 532 || NR == 1029 { print "    free(hd_isog.steps);" } { print }' \
    >"$scratch/pike_compressed_fixed.c"
build() { # tree level output pike_compressed.c extra-flags...
    t=$1 l=$2 o=$3 p=$4
    shift 4
    d=src/precomp/ref/NGCC_$l g=src/gf/ref/NGCC_$l x=
    case "$t-$l" in
        "$ref-1") gf="$g/fp_p479.c" ;;
        "$ref-2") gf="$g/fp_p765.c" ;;
        "$ref-3") gf="$g/fp_p1534.c" ;;
        "$opt-1") gf="$g/fp_p479.c $g/fp_p479_asm.S" ;;
    esac
    [ "$t" = "$opt" ] &&
        x='-DPIKE_NGCC1_OPTIMIZED_FIELD=1 -DPIKE_NGCC2_OPTIMIZED_FIELD=1 -DPIKE_NGCC3_OPTIMIZED_FIELD=1 -mbmi2 -madx'
    [ "$l" = 3 ] && x="$x -DPIKE_COMPRESSED_SHARED_SECRET_BYTES=64"
    s=
    for f in $common $gf $d/gf_constants.c $d/torsion_constants.c $d/endomorphism_action.c \
        $d/klpt_constants.c $d/quaternion_data.c; do
        s="$s $t/$f"
    done
    s="$s $p"
    gcc -O2 -w -std=gnu11 -fno-strict-aliasing -fwrapv -fno-omit-frame-pointer -D_GNU_SOURCE "$@" $x \
        -DALGORITHM_INSTANCE="\"NGCC-$l\"" -DNGCC_PIKE_LEVEL="$l" -DOUTPUT_BLANK_TEST_VECTORS=0 \
        -DPIKE_XOF_BACKEND=2 -DTARGET_AMD64 -DRADIX_64 -DTARGET_OS_UNIX \
        -I"$t/src/ngcc/common" -I"$t/include" -I"$t/src/common/generic/include" \
        -I"$t/src/intbig/ref/generic/include" -I"$t/src/quaternion/ref/generic/include" \
        -I"$t/$d/include" -I"$t/src/klpt/ref/include" -I"$t/src/gf/ref/include" \
        -I"$t/src/ec/ref/include" -I"$t/src/hd/ref/include" -I"$t/src/id2iso/ref/include" \
        -I"$t/src/pike/ref/include" -I"$t/src/pike/ref/pikex_compressed/include" \
        -Wl,--wrap=malloc,--wrap=free,--wrap=theta_chain_comput_strategy \
        -o "$scratch/$o" "$driver" $s -lgmp >"$scratch/$o.log" 2>&1
}
runs='ref-1 ref-2 ref-3 fix-1'
[ $withopt = 1 ] && runs="$runs opt-1"
for b in $runs; do
    case $b in
        ref-*) build "$ref" "${b#ref-}" "$b" "$ref/$pc" ;;
        fix-*) build "$ref" "${b#fix-}" "$b" "$scratch/pike_compressed_fixed.c" ;;
        opt-*) build "$opt" "${b#opt-}" "$b" "$opt/$pc" -DNDEBUG ;;
    esac &
done
wait
for b in $runs; do
    [ -x "$scratch/$b" ] || { cat "$scratch/$b.log"; bad "$b build failed"; }
    (cd "$scratch" && ulimit -c 0 && "./$b" >"$b.out" 2>"$b.err"; echo $? >"$b.rc") 2>/dev/null &
done
wait

for b in $runs; do
    case $b in
        *-1) k=159 z=4632 ;;
        *-2) k=257 z=7512 ;;
        *-3) k=511 z=15000 ;;
    esac
    a=$((k * z)) m=1
    arr="array=$k*$z=$a"
    [ "$(cat "$scratch/$b.rc")" = 0 ] || { cat "$scratch/$b.err"; bad "$b: driver failed"; }
    case $b in fix-*) a=0 m=0 ;; esac
    {
        echo "$arr"
        echo "keygen rc=0 live=$((2 * m)) bytes=$((2 * a)) steps=$((2 * m)) gmp=1"
        echo "keygen rc=0 live=$((4 * m)) bytes=$((4 * a)) steps=$((4 * m)) gmp=2"
        echo "enc rc=0 live=$((4 * m)) bytes=$((4 * a)) steps=$((4 * m)) gmp=2"
        echo "enc rc=0 live=$((4 * m)) bytes=$((4 * a)) steps=$((4 * m)) gmp=2"
        echo "dec rc=0 live=$((5 * m)) bytes=$((5 * a)) steps=$((5 * m)) gmp=2"
        echo "dec rc=0 live=$((6 * m)) bytes=$((6 * a)) steps=$((6 * m)) gmp=2"
        echo "agree=1"
        echo "dec-flipped rc=-1 live=$((7 * m)) bytes=$((7 * a)) steps=$((7 * m)) gmp=2"
        echo "freed $((7 * m)) arrays: live=0 bytes=0 untracked_frees=0"
    } >"$scratch/$b.want"
    cmp -s "$scratch/$b.out" "$scratch/$b.want" || { cat "$scratch/$b.out"; bad "$b: live heap differs from the expected growth"; }
    case $b in
        fix-*) echo "NGCC-${b#*-} reference with free(hd_isog.steps) added: no live blocks after any call and the shared secrets agree (control)"
            continue ;;
        ref-*) tree=reference ;;
        *) tree=optimized ;;
    esac
    echo "NGCC-${b#*-} $tree: each kem_keygen leaves 2 more live blocks ($((2 * a)) bytes), each kem_enc none, each kem_dec 1 ($a bytes), also when it rejects; all are step arrays of $k x $z bytes, and freeing them leaves nothing live; key generation also leaves one GMP block"
done
[ $withopt = 1 ] || echo 'optimized tree not run: needs x86-64 with BMI2 and ADX'
echo "ATTACK kem-31-7: every kem_keygen leaks two theta-chain step arrays and every kem_dec one, at NGCC-1/2/3: CONFIRMED"
exit 0
