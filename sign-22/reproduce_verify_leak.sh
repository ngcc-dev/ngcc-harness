#!/bin/sh
# sign-22-10: Rhyme-SM3 verification leaks its SM3 XOF caches on every call.
#
# Builds the reference Rhyme-SM3 and Rhyme-SHAKE sets with AddressSanitizer's
# leak checker and runs one verification of a well-formed (c_tilde, z = 0)
# signature, using the driver of reproduce_short_pk.c. Every Rhyme-SM3 build
# must report leaked blocks allocated in the SM3 XOF (xof_generate); every
# Rhyme-SHAKE build is the control and must report none.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-22-10: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-22-10: $1"; exit 1; }

impl='sign-22/Implementations and Test_Vectors/Implementations/Reference_Implementation'
driver="$here/reproduce_short_pk.c"
[ -d "$impl" ] && [ -f "$driver" ] || bad 'implementation tree or driver not found'
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
printf 'int main(void){return 0;}\n' >"$scratch/t.c"
gcc -fsanitize=address -o "$scratch/t" "$scratch/t.c" >/dev/null 2>&1 &&
    ASAN_OPTIONS=detect_leaks=1 "$scratch/t" >/dev/null 2>&1 ||
    { echo 'SKIP sign-22-10: AddressSanitizer leak checking unavailable'; exit 77; }

core='SIG_AlgorithmInstance.c src/poly.c src/ntt.c src/ntt_tables.c src/sampler.c src/encoding.c
      src/packing.c src/sign.c src/zpntt.c src/symmetric-shake.c rhyme_xof.c src/keygen/kg_main.c
      src/keygen/kg_solver.c src/keygen/kg_zint.c src/keygen/kg_ntt.c src/keygen/kg_primes.c
      randombytes.c auxfunc.c drng.c'
for f in SM3 SHAKE; do
    case $f in SM3) extra='sm3.c sm3_xof.c' ;; SHAKE) extra='src/fips202.c' ;; esac
    for l in 128 256 384 512; do
        (cd "$impl/Rhyme-$f/Rhyme-$f-$l" && gcc -O1 -g -fsanitize=address -std=c99 -w -I. -Iinclude \
            -Isrc/keygen -DRHYME_NO_AES -DRHYME_MODE=$l -DOUTPUT_BLANK_TEST_VECTORS=0 \
            -o "$scratch/$f-$l" "$driver" $core $extra -lm) >"$scratch/$f-$l.log" 2>&1 &
    done
done
wait
for f in SM3 SHAKE; do
    for l in 128 256 384 512; do
        b="$scratch/$f-$l"
        [ -x "$b" ] || { cat "$b.log"; bad "Rhyme-$f-$l build failed"; }
        ASAN_OPTIONS=detect_leaks=1 "$b" keyless full >"$scratch/out" 2>&1
        rc=$?
        sum=$(grep -o 'SUMMARY: AddressSanitizer: [0-9]* byte(s) leaked in [0-9]* allocation(s)' "$scratch/out")
        if [ $f = SM3 ]; then
            [ $rc -ne 0 ] && [ -n "$sum" ] && grep -q 'in xof_generate' "$scratch/out" ||
                { cat "$scratch/out"; bad "Rhyme-SM3-$l: no leak in the SM3 XOF"; }
            echo "Rhyme-SM3-$l: one verification leaks: ${sum#SUMMARY: AddressSanitizer: }"
        else
            [ $rc -eq 0 ] && [ -z "$sum" ] || { cat "$scratch/out"; bad "Rhyme-SHAKE-$l control leaks"; }
            echo "Rhyme-SHAKE-$l: no leak (control)"
        fi
    done
done
echo "ATTACK sign-22-10: every Rhyme-SM3 verification leaks its SM3 XOF caches: CONFIRMED"
exit 0
