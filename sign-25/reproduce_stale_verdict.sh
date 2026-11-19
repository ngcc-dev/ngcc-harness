#!/bin/sh
# sign-25-1: the SQIsign2D2 verifier's verdict depends on stale stack state,
# in more instances than the harness's shared-library build shows.
#
# Builds all sixteen instances (eight reference directories, each with
# COMPRESSED=0 and 1) as static executables with the candidate's release
# flags -O2 -DNDEBUG and reproduce_stale_verdict.c, then runs each in fresh
# processes:
#   - a valid signature with one bit flipped, verified fresh, after verifying
#     the unmodified signature (primed), and after overwriting the stack
#     (scrubbed). Fresh and scrubbed must reject in every instance.
#   - after an honest verification of an unrelated signature: the signature on
#     m verified against another message m2, and an all-zero signature on m2.
#     Both must be rejected in a fresh state.
# It prints every verdict and requires Level1-eff and Level2-eff uncompressed
# to accept both the other-message and the all-zero signature when primed, and
# at least one compressed instance to accept the flipped signature when
# primed. Verdicts depend on stack layout and so on the compiler; this run
# used GCC 15. Runtime is about two minutes.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-25-1: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-25-1: $1"; exit 1; }
ref=sign-25/Implementations/Reference_Implementation
[ -d "$ref" ] || bad "implementation tree not found"
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
printf '#include <gmp.h>\nint main(void){mpz_t x; mpz_init(x); mpz_clear(x); return 0;}\n' >"$scratch/g.c"
gcc -o "$scratch/g" "$scratch/g.c" -lgmp >/dev/null 2>&1 || { echo 'SKIP sign-25-1: GMP unavailable'; exit 77; }

src='algebra.c auxfunc.c basis.c biextension.c dim2.c dim2id2iso.c dim4.c drng.c ec.c finit.c fp.c fp2.c
     hd.c id2iso.c ideal.c intbig.c integers.c isog_chains.c klptx.c lattice.c lll.c matkermod.c mp.c pack.c
     printer.c theta_isogenies.c theta_structure.c tools.c xeval.c xisog.c ec_arithmetic/endomorphism_action.c
     ec_arithmetic/gf_constants.c ec_arithmetic/quaternion_data.c ec_arithmetic/torsion_constants.c
     sqisign/keygen.c sqisign/sign.c sqisign/SIG_AlgorithmInstance.c'
insts='Level1-eff Level1-sec Level2-eff Level2-sec Level3-eff Level3-sec Level5-eff Level5-sec'
for i in $insts; do
    d="$ref/SQISign2Dsquare-$i"
    fp=$(cd "$d" && ls ec_arithmetic/fp_p*.c) || bad "$i prime-field source not found"
    for c in 0 1; do
        (cd "$d" && gcc -std=gnu99 -O2 -DNDEBUG -DCOMPRESSED=$c -w -Isqisign -I. -Iec_arithmetic/include \
            -o "$scratch/$i-$c" "$here/reproduce_stale_verdict.c" $src $fp -lgmp) >"$scratch/$i-$c.log" 2>&1 &
    done
done
wait
for i in $insts; do
    for c in 0 1; do
        b="$scratch/$i-$c"
        [ -x "$b" ] || { cat "$b.log"; bad "$i COMPRESSED=$c build failed"; }
        for mode in fresh primed scrubbed; do "$b" flip $mode >"$b.$mode" 2>&1 & done
        "$b" msg >"$b.msg" 2>&1 &
    done
done
wait

flipped=0; flipped_c=0
for i in $insts; do
    for c in 0 1; do
        b="$scratch/$i-$c"
        f=$(cat "$b.fresh"); p=$(cat "$b.primed"); s=$(cat "$b.scrubbed"); m=$(cat "$b.msg")
        [ "$f" = 'flip fresh honest=99 flipped=-1' ] || bad "$i COMPRESSED=$c fresh: $f"
        [ "$s" = 'flip scrubbed honest=99 flipped=-1' ] || bad "$i COMPRESSED=$c scrubbed: $s"
        case "$p" in 'flip primed honest=0 flipped='*) ;; *) bad "$i COMPRESSED=$c primed: $p" ;; esac
        case "$m" in 'msg fresh_other=-1 honest=0 primed_other='*' honest=0 primed_zero='*) ;;
            *) bad "$i COMPRESSED=$c message test: $m" ;; esac
        pv=${p##*=}
        if [ "$pv" = 0 ]; then flipped=$((flipped + 1)); [ $c = 1 ] && flipped_c=$((flipped_c + 1)); fi
        echo "$i COMPRESSED=$c: flipped bit fresh/primed/scrubbed = -1/$pv/-1; ${m#msg }"
        if [ $c = 0 ] && { [ $i = Level1-eff ] || [ $i = Level2-eff ]; }; then
            case "$m" in *'primed_other=0 honest=0 primed_zero=0') ;;
                *) bad "$i uncompressed did not accept the other-message and all-zero signatures when primed" ;; esac
        fi
    done
done
[ $flipped_c -ge 1 ] || bad "no compressed instance accepted a flipped signature when primed"
echo "$flipped of 16 instances accept a one-bit-modified signature right after verifying the original ($flipped_c compressed); every fresh and scrubbed run rejects"
echo "ATTACK sign-25-1: the SQIsign2D2 verdict depends on stale stack state; primed verifiers accept modified, other-message and all-zero signatures: CONFIRMED"
exit 0
