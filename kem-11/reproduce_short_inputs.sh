#!/bin/sh
# kem-11-4: kem_enc and kem_dec discard the declared public-key, secret-key
# and ciphertext lengths, so short inputs are read out of bounds; the
# optimized eta1 = 3 sampler also reads past its stack buffer on every call.
#
# Checks the wrappers in all eight trees, then builds each with
# AddressSanitizer. An honest ciphertext copied into a heap buffer of exactly
# 16 bytes, or one byte short, must give a heap-buffer-overflow in
# poly_decompress under kem_dec; an honest public key one byte short must give
# one under kem_enc. Controls: full-length inputs run cleanly and decapsulate
# to the encapsulated key. In the optimized trees, honest key generation under
# full instrumentation must read 16 bytes past a stack buffer in poly_cbd3_avx
# at COMPASS-KEM-128 and -384 (eta1 = 3) and run cleanly at -256 and -512; the
# input-length checks there use a build with cbd.c uninstrumented. The
# optimized builds use -mavx2 -mbmi2 -mpopcnt without the Makefile's
# -march=native, under which this GCC's AddressSanitizer build faults in the
# ICCS hmac on any input.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-11-4: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-11-4: $1"; exit 1; }

impl='kem-11/Implementations and Test_Vectors/Implementations'
[ -d "$impl" ] || bad "implementation tree not found"
driver="$here/reproduce_short_inputs.c"
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
ASAN_OPTIONS=detect_leaks=0
export ASAN_OPTIONS

printf 'int main(void){return 0;}\n' >"$scratch/t.c"
gcc -fsanitize=address -o "$scratch/t" "$scratch/t.c" >/dev/null 2>&1 && "$scratch/t" ||
    { echo 'SKIP kem-11-4: AddressSanitizer unavailable'; exit 77; }

load='        __m128i t = _mm_loadu_si128((__m128i*)&buf[12 * i / 16]); '
for t in Reference Optimized; do
    for l in 128 256 384 512; do
        d="$impl/${t}_Implementation/COMPASS-KEM-$l"
        w=$(tr -d '\r' <"$d/KEM_AlgorithmInstance.c")
        for v in pk sk ct; do
            echo "$w" | grep -Eq "^    \(void\)${v}_len_bytes;( //.*)?$" || bad "$t COMPASS-KEM-$l uses ${v}_len_bytes"
        done
        if [ $t = Optimized ]; then
            tr -d '\r' <"$d/cbd.c" | grep -Fxq "$load" &&
                tr -d '\r' <"$d/indcpa.c" | grep -Fq '    uint8_t buf0[COMPASS_KEM_ETA1*COMPASS_KEM_N/4];' ||
                bad "optimized COMPASS-KEM-$l sampler differs from the archive"
        fi
    done
done
echo 'source: all eight kem_enc/kem_dec wrappers discard pk_len_bytes, sk_len_bytes and ct_len_bytes'

avx2=0
grep -qw avx2 /proc/cpuinfo 2>/dev/null && grep -qw bmi2 /proc/cpuinfo 2>/dev/null && avx2=1
trees='Reference'
[ $avx2 = 1 ] && trees='Reference Optimized'
asan='-O1 -g -fsanitize=address -fno-omit-frame-pointer -w'
for t in $trees; do
    for l in 128 256 384 512; do
        d="$impl/${t}_Implementation/COMPASS-KEM-$l"
        o="$scratch/$t-$l"
        if [ $t = Reference ]; then
            (cd "$d" && gcc $asan -DSECURITY_LEVEL=$l -I. -o "$o" "$driver" KEM_AlgorithmInstance.c \
                kem.c indcpa.c polyvec.c poly.c ntt.c cbd.c reduce.c verify.c drng.c auxfunc.c \
                symmetric-shake.c) >"$o.log" 2>&1 &
        else
            src='KEM_AlgorithmInstance.c kem.c indcpa.c polyvec.c poly.c ntt.c reduce.c verify.c consts.c rejsample.c drng.c auxfunc.c symmetric-shake.c fq.S'
            x='-mavx2 -mbmi2 -mpopcnt'
            (cd "$d" && gcc $asan $x -DSECURITY_LEVEL=$l -I. -o "$o-all" "$driver" $src cbd.c &&
                gcc -O1 -g -w $x -DSECURITY_LEVEL=$l -I. -c -o "$o-cbd.o" cbd.c &&
                gcc $asan $x -DSECURITY_LEVEL=$l -I. -o "$o" "$driver" $src "$o-cbd.o") >"$o.log" 2>&1 &
        fi
    done
done
wait

overflow() { # binary mode len api
    "$1" "$2" "$3" >"$scratch/out" 2>&1 && return 1
    grep -q 'ERROR: AddressSanitizer: heap-buffer-overflow' "$scratch/out" &&
        grep -q "is located 0 bytes after $3-byte region" "$scratch/out" &&
        grep -q "in $4 .*KEM_AlgorithmInstance.c" "$scratch/out"
}
n=0
for t in $trees; do
    for l in 128 256 384 512; do
        b="$scratch/$t-$l"
        [ -x "$b" ] || { cat "$b.log"; bad "$t COMPASS-KEM-$l build failed"; }
        out=$("$b" ct full 2>&1) && ctl=$(echo "$out" | sed -n 's/^ct_len=\([0-9]*\) of .*/\1/p') &&
            case "$out" in *"dec=0 agrees=1") ;; *) false ;; esac || bad "$t COMPASS-KEM-$l ciphertext control: $out"
        out=$("$b" pk full 2>&1) && pkl=$(echo "$out" | sed -n 's/^pk_len=\([0-9]*\) of .*/\1/p') &&
            case "$out" in *"enc=0") ;; *) false ;; esac || bad "$t COMPASS-KEM-$l public-key control: $out"
        for len in 16 $((ctl - 1)); do
            overflow "$b" ct $len kem_dec && grep -q 'in .*poly_decompress .*poly.c' "$scratch/out" ||
                { cat "$scratch/out"; bad "$t COMPASS-KEM-$l: no overflow for a $len-byte ciphertext"; }
        done
        overflow "$b" pk $((pkl - 1)) kem_enc ||
            { cat "$scratch/out"; bad "$t COMPASS-KEM-$l: no overflow for a $((pkl - 1))-byte public key"; }
        msg="ct $ctl and pk $pkl bytes clean; 16 and $((ctl - 1))-byte ciphertexts and a $((pkl - 1))-byte public key read out of bounds"
        if [ $t = Optimized ]; then
            "$b-all" ct full >"$scratch/out" 2>&1
            case $l in
                128|384)
                    grep -q 'READ of size 16' "$scratch/out" && grep -q 'is located in stack' "$scratch/out" &&
                        grep -q 'in .*poly_cbd3_avx .*cbd.c:189' "$scratch/out" ||
                        { cat "$scratch/out"; bad "optimized COMPASS-KEM-$l: no stack over-read in poly_cbd3_avx"; }
                    msg="$msg; honest key generation reads 16 bytes across the end of a stack noise buffer (cbd.c:189)" ;;
                *)
                    grep -q '^dec=0 agrees=1$' "$scratch/out" && ! grep -q AddressSanitizer "$scratch/out" ||
                        { cat "$scratch/out"; bad "optimized COMPASS-KEM-$l: honest run not clean"; }
                    msg="$msg; honest run clean under full instrumentation" ;;
            esac
        fi
        echo "$t COMPASS-KEM-$l: $msg"
        n=$((n + 1))
    done
done
[ $avx2 = 1 ] || echo 'optimized trees not run: CPU lacks AVX2/BMI2 (source checks above still cover them)'
echo "$n builds: short ciphertexts and public keys are read out of bounds; full-length controls are clean"
echo "ATTACK kem-11-4: kem_enc and kem_dec ignore the declared input lengths and over-read short buffers: CONFIRMED"
exit 0
