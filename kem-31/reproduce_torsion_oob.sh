#!/bin/sh
# kem-31-6: the optimized tree's TORSION_ODD_PRIMEPOWERS table has four
# entries while ec_degree_odd_t has three elements, so the loop that zeroes
# one degree per table entry (id2iso.c:158-161) writes degree[3] in every key
# generation.
#
# Checks the table and array dimensions of both source trees at NGCC_1/2/3,
# then builds each tree at each level in a temporary directory with gcc
# -fsanitize=bounds (recovering) and runs one deterministic key generation,
# encapsulation and decapsulation. The driver wraps
# id2iso_ideal_to_isogeny_odd to pre-fill keygen's ec_isog_odd_t with 0xA5 and
# print the bytes after its degree array. Control: the reference tree, built
# with assertions enabled, reports nothing and leaves those bytes untouched.
# The optimized tree is built twice: with -DNDEBUG, as its README builds it,
# it must report index 3 at id2iso.c:161, zero the two bytes after the array
# and still produce the reference tree's keys, ciphertext and shared secret;
# with assertions enabled its own size check at id2iso.c:268 must abort key
# generation.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-31-6: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-31-6: $1"; exit 1; }

ref=kem-31/Implementations/Implementations
opt=kem-31/Implementations/Optimized_Implementation
[ -d "$ref" ] && [ -d "$opt" ] || bad "implementation trees not found"
driver="$here/reproduce_torsion_oob.c"
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM

src() { tr -d '\r' <"$1"; }
for t in "$ref" "$opt"; do
    n=3
    [ "$t" = "$opt" ] && n=4
    src "$t/src/ec/ref/include/ec.h" | grep -Fq 'typedef uint16_t ec_degree_odd_t[P_LEN + M_LEN];' ||
        bad "$t: ec_degree_odd_t differs from the archive"
    f=$t/src/id2iso/ref/id2isox/id2iso.c
    src "$f" | sed -n 152p | grep -Fq '#define NUMPP (sizeof(TORSION_ODD_PRIMEPOWERS) / sizeof(*TORSION_ODD_PRIMEPOWERS))' &&
        src "$f" | sed -n 158p | grep -Fq 'for (size_t i = 0; i < NUMPP; ++i)' &&
        src "$f" | sed -n 161p | grep -Fq '(*deg)[i] = 0;' &&
        src "$f" | sed -n 262p | grep -Fq 'id2iso_ideal_to_kernel_dlogs_odd(&vec, &isog->degree, lideal_input);' ||
        bad "$t: id2iso.c differs from the archive"
    for l in 1 2 3; do
        p=$t/src/precomp/ref/NGCC_$l
        src "$p/include/ec_params.h" | grep -q '^#define P_LEN 2 ' &&
            src "$p/include/ec_params.h" | grep -q '^#define M_LEN 1 ' || bad "$p: P_LEN + M_LEN is not 3"
        src "$p/include/torsion_constants.h" | grep -Fxq "extern const ibz_t TORSION_ODD_PRIMEPOWERS[$n];" &&
            [ "$(src "$p/torsion_constants.c" | grep -c "^const ibz_t TORSION_ODD_PRIMEPOWERS\[$n\] = ")" = 3 ] ||
            bad "$p: TORSION_ODD_PRIMEPOWERS does not have $n entries"
        [ $n = 3 ] || [ "$(src "$p/torsion_constants.c" |
            grep -c '^const ibz_t TORSION_ODD_PRIMEPOWERS\[4\] = .*(mp_limb_t\[\]) {0x1}}}};$')" = 3 ] ||
            bad "$p: the fourth table entry is not 1"
    done
done
echo 'source: ec_degree_odd_t has P_LEN + M_LEN = 3 elements in both trees; TORSION_ODD_PRIMEPOWERS has 3 entries in the reference tree and 4 (the last equal to 1) in the optimized tree at NGCC_1/2/3'

case $(uname -m) in
    x86_64 | amd64) ;;
    *) echo 'SKIP kem-31-6: the optimized tree needs x86-64 (source checks passed)'; exit 77 ;;
esac
grep -qw bmi2 /proc/cpuinfo 2>/dev/null && grep -qw adx /proc/cpuinfo 2>/dev/null ||
    { echo 'SKIP kem-31-6: CPU lacks BMI2/ADX, needed by the optimized tree (source checks passed)'; exit 77; }
printf '#include <gmp.h>\nint main(void){mpz_t a;mpz_init(a);mpz_clear(a);return 0;}\n' >"$scratch/t.c"
gcc -fsanitize=bounds -o "$scratch/t" "$scratch/t.c" -lgmp >/dev/null 2>&1 && "$scratch/t" ||
    { echo 'SKIP kem-31-6: gcc -fsanitize=bounds or GMP unavailable (source checks passed)'; exit 77; }

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
    src/id2iso/ref/id2isox/id2iso.c src/pike/ref/pikex_compressed/pike_compressed.c'
build() { # tree level output extra-flags...
    t=$1 l=$2 o=$3
    shift 3
    d=src/precomp/ref/NGCC_$l g=src/gf/ref/NGCC_$l x=
    case "$t-$l" in
        "$ref-1") gf="$g/fp_p479.c" ;;
        "$ref-2") gf="$g/fp_p765.c" ;;
        "$ref-3") gf="$g/fp_p1534.c" ;;
        "$opt-1") gf="$g/fp_p479.c $g/fp_p479_asm.S" ;;
        "$opt-2") gf="$g/fp_p765.c $g/fp_p765_asm.S" ;;
        "$opt-3") gf="$g/fp_p1534_opt.c $g/fp_p1534_opt_asm.S" ;;
    esac
    [ "$t" = "$opt" ] &&
        x='-DPIKE_NGCC1_OPTIMIZED_FIELD=1 -DPIKE_NGCC2_OPTIMIZED_FIELD=1 -DPIKE_NGCC3_OPTIMIZED_FIELD=1 -mbmi2 -madx'
    [ "$l" = 3 ] && x="$x -DPIKE_COMPRESSED_SHARED_SECRET_BYTES=64"
    s=
    for f in $common $gf $d/gf_constants.c $d/torsion_constants.c $d/endomorphism_action.c \
        $d/klpt_constants.c $d/quaternion_data.c; do
        s="$s $t/$f"
    done
    gcc -O1 -w -std=gnu11 -fno-omit-frame-pointer -fsanitize=bounds "$@" $x \
        -DALGORITHM_INSTANCE="\"NGCC-$l\"" -DNGCC_PIKE_LEVEL="$l" -DOUTPUT_BLANK_TEST_VECTORS=0 \
        -DPIKE_XOF_BACKEND=2 -DTARGET_AMD64 -DRADIX_64 -DTARGET_OS_UNIX \
        -I"$t/src/ngcc/common" -I"$t/include" -I"$t/src/common/generic/include" \
        -I"$t/src/intbig/ref/generic/include" -I"$t/src/quaternion/ref/generic/include" \
        -I"$t/$d/include" -I"$t/src/klpt/ref/include" -I"$t/src/gf/ref/include" \
        -I"$t/src/ec/ref/include" -I"$t/src/hd/ref/include" -I"$t/src/id2iso/ref/include" \
        -I"$t/src/pike/ref/include" -I"$t/src/pike/ref/pikex_compressed/include" \
        -Wl,--wrap=id2iso_ideal_to_isogeny_odd -o "$scratch/$o" "$driver" $s -lgmp >"$scratch/$o.log" 2>&1
}
for l in 1 2 3; do
    build "$ref" $l ref-$l &
    build "$opt" $l opt-$l -DNDEBUG &
    build "$opt" $l optassert-$l &
done
wait
for l in 1 2 3; do
    for b in ref-$l opt-$l optassert-$l; do
        [ -x "$scratch/$b" ] || { cat "$scratch/$b.log"; bad "$b build failed"; }
        (cd "$scratch" && ulimit -c 0 && "./$b" >"$b.out" 2>"$b.err"; echo $? >"$b.rc") 2>/dev/null &
    done
done
wait

ub='id2iso\.c:161:[0-9]*: runtime error: index 3 out of bounds for type'
sed -n "/$ub/{s/^/sanitizer: /p;q;}" "$scratch/opt-1.err"
for l in 1 2 3; do
    r=$scratch/ref-$l o=$scratch/opt-$l a=$scratch/optassert-$l
    rw=$(sed -n 1p "$r.out") rk=$(sed -n 2p "$r.out")
    ow=$(sed -n 1p "$o.out") ok=$(sed -n 2p "$o.out")
    [ "$(cat "$r.rc")" = 0 ] && ! grep -Eq 'runtime error|Assertion' "$r.err" ||
        { cat "$r.err"; bad "reference NGCC_$l: control run not clean"; }
    case "$rw" in "table=3 degree=3 "*" after=a5a5") ;; *) bad "reference NGCC_$l: $rw" ;; esac
    case "$rk" in "keygen=0 enc=0 dec=0 agree=1 "*) ;; *) bad "reference NGCC_$l: $rk" ;; esac
    [ "$(cat "$o.rc")" = 0 ] && [ "$(grep -c "$ub" "$o.err")" = 1 ] ||
        { cat "$o.err"; bad "optimized NGCC_$l: no out-of-bounds index reported at id2iso.c:161"; }
    case "$ow" in "table=4 degree=3 "*" after=0000") ;; *) bad "optimized NGCC_$l: $ow" ;; esac
    [ "$ok" = "$rk" ] || bad "optimized NGCC_$l output differs from the reference: $ok"
    [ "$(cat "$a.rc")" != 0 ] && grep -q "$ub" "$a.err" &&
        grep -q 'id2iso\.c:268: id2iso_ideal_to_isogeny_odd: Assertion' "$a.err" ||
        { cat "$a.err"; bad "optimized NGCC_$l: assertion-enabled build did not abort at id2iso.c:268"; }
    end=$(echo "$ow" | sed -n 's/.* degree_end=\([0-9]*\) .*/\1/p')
    size=$(echo "$ow" | sed -n 's/.* isog=\([0-9]*\) .*/\1/p')
    echo "NGCC-$l: reference clean, bytes after degree[] untouched; optimized zeroes bytes $end-$((size - 1)) of the $size-byte ec_isog_odd_t (degree[3], the tail padding) and matches the reference pk/sk/ct/ss; with assertions on it aborts at id2iso.c:268"
done
echo "ATTACK kem-31-6: optimized-tree key generation writes degree[3] of the three-element ec_degree_odd_t at NGCC-1/2/3; reference tree clean: CONFIRMED"
exit 0
