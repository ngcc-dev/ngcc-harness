#!/bin/sh
# sign-25: SQIsign2D2 leaks heap memory on every sig_keygen, sig_sign and
# sig_verify call.
#   By source, in all 16 instances (reference and optimized): ibz_rand_interval
#   mallocs its limb buffer and never frees it (intbig.c); fp2_batched_inv
#   mallocs two arrays and fp2.c contains no free(); theta_chain_comput_strategy
#   mallocs out->steps and its work arrays (theta_isogenies.c) and contains no
#   free(). At run time (Level1-eff reference, the candidate's -O2 -DNDEBUG,
#   uncompressed), after one warm-up call each further call leaves a fixed
#   number of live blocks: the driver counts the candidate's own allocations
#   with --wrap and GMP's through mp_set_memory_functions. Verification leaks
#   the same amount on the accepting path and on a rejected bit-flipped
#   signature.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-25: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-25: $1"; exit 1; }
drv=$here/reproduce_leak.c
impl=sign-25/Implementations
n=0
for d in "$impl"/Reference_Implementation/SQISign2Dsquare-* "$impl"/Optimized_Implementation/SQISign2Dsquare-*; do
    r=$(sed -n '/^ibz_rand_interval(/,/^}/p' "$d/intbig.c")
    t=$(sed -n '/^theta_chain_comput_strategy(/,/^}/p' "$d/theta_isogenies.c")
    echo "$r" | grep -Fq 'mp_limb_t *r = (mp_limb_t *)malloc(sizeof(mp_limb_t)*len_limbs);' &&
        ! echo "$r" | grep -q 'free(' &&
        echo "$t" | grep -Fq 'out->steps = malloc(' && ! echo "$t" | grep -q 'free(' &&
        grep -Fq 'fp2_t *t1=(fp2_t*)malloc(sizeof(fp2_t)*len);' "$d/fp2.c" && ! grep -q 'free(' "$d/fp2.c" ||
        bad "$d differs from the archive"
    n=$((n + 1))
done
[ $n = 16 ] || bad "expected 16 instances, found $n"
echo 'source: all 16 instances malloc without free in ibz_rand_interval, fp2_batched_inv and theta_chain_comput_strategy'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
printf '#include <gmp.h>\nint main(void){mpz_t x; mpz_init(x); mpz_clear(x); return 0;}\n' >"$scratch/g.c"
gcc -o "$scratch/g" "$scratch/g.c" -lgmp 2>/dev/null || { echo 'SKIP sign-25: GMP unavailable'; exit 77; }
d=$impl/Reference_Implementation/SQISign2Dsquare-Level1-eff
src='algebra.c auxfunc.c basis.c biextension.c dim2.c dim2id2iso.c dim4.c drng.c ec.c finit.c fp.c fp2.c hd.c id2iso.c ideal.c intbig.c integers.c isog_chains.c klptx.c lattice.c lll.c matkermod.c mp.c pack.c printer.c theta_isogenies.c theta_structure.c tools.c xeval.c xisog.c ec_arithmetic/endomorphism_action.c ec_arithmetic/gf_constants.c ec_arithmetic/quaternion_data.c ec_arithmetic/torsion_constants.c sqisign/keygen.c sqisign/sign.c sqisign/SIG_AlgorithmInstance.c ec_arithmetic/fp_p255.c'
(cd "$d" && gcc -std=c99 -O2 -DNDEBUG -w -Isqisign -I. -Iec_arithmetic/include \
    -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free -o "$scratch/l" "$drv" $src -lgmp) >"$scratch/cc" 2>&1 ||
    { cat "$scratch/cc"; bad "Level1-eff build failed"; }
out=$("$scratch/l") || bad "Level1-eff run failed"
echo "$out" | sed 's/^/Level1-eff /'
echo "$out" | grep -q '^honest accepted 4/4, bit-flipped rejected 4/4$' || bad "verification controls differ"
for w in 'sig_keygen' 'sig_sign' 'sig_verify (honest)' 'sig_verify (bit-flipped)'; do
    y=$(echo "$out" | grep -F "$w:" | sed -n 's/.*total +\([0-9]*\) bytes$/\1/p')
    [ -n "$y" ] && [ "$y" -gt 100000 ] || bad "$w leaks less than 100 kB per call"
done
echo 'ATTACK sign-25-5: every SQIsign2D2 sig_keygen, sig_sign and sig_verify call leaks hundreds of kilobytes of heap: CONFIRMED'
exit 0
