#!/bin/sh
# kem-15-7: kem_enc and kem_dec discard the declared public-key, secret-key
# and ciphertext lengths, so short inputs are read out of bounds.
#
# Checks the wrappers in all ten trees, then builds each with
# AddressSanitizer. An honest ciphertext copied into a heap buffer of exactly
# 1 byte, or one byte short, must give a heap-buffer-overflow in
# poly_unpack_and_decompress (symbol _poly_decompress) under kem_dec; an
# honest public key one byte short must give an out-of-bounds read under
# kem_enc. Controls: full-length inputs run cleanly and decapsulate to the
# encapsulated key. The AVX2 builds use -mavx2 -mbmi2 -mpopcnt -mpclmul
# without the Makefile's -march=native, under which this GCC's
# AddressSanitizer build faults in the ICCS hmac on any input. The FLIT512
# AVX2 tree is built with basemul_avx.c uninstrumented, because its stack
# writes (kem-15-5) stop a fully instrumented run in key generation.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-15-7: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-15-7: $1"; exit 1; }

impl='kem-15/Implementations'
[ -d "$impl" ] || bad "implementation tree not found"
driver="$here/reproduce_short_ciphertext.c"
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
ASAN_OPTIONS=detect_leaks=0
export ASAN_OPTIONS

printf 'int main(void){return 0;}\n' >"$scratch/t.c"
gcc -fsanitize=address -o "$scratch/t" "$scratch/t.c" >/dev/null 2>&1 && "$scratch/t" ||
    { echo 'SKIP kem-15-7: AddressSanitizer unavailable'; exit 77; }

ref='Reference_Implementation/FLIT128 Reference_Implementation/FLIT256 Reference_Implementation/FLIT512
    Additional_Implementation/FLIT_FIPS202_REF128 Additional_Implementation/FLIT_FIPS202_REF256'
opt='Optimized_Implementation/FLIT128 Optimized_Implementation/FLIT256 Optimized_Implementation/FLIT512
    Additional_Implementation/FLIT_FIPS202_OPT128 Additional_Implementation/FLIT_FIPS202_OPT256'
for t in $ref $opt; do
    w=$(tr -d '\r' <"$impl/$t/KEM_AlgorithmInstance.c") || bad "$t wrapper not found"
    for v in pk sk ct; do
        echo "$w" | grep -Eq "^[[:space:]]+\(void\)${v}_len_bytes;[[:space:]]*$" || bad "$t uses ${v}_len_bytes"
    done
done
echo 'source: all ten kem_enc/kem_dec wrappers discard pk_len_bytes, sk_len_bytes and ct_len_bytes'

avx2=0
grep -qw avx2 /proc/cpuinfo 2>/dev/null && grep -qw bmi2 /proc/cpuinfo 2>/dev/null &&
    grep -qw pclmulqdq /proc/cpuinfo 2>/dev/null && avx2=1
trees=$ref
[ $avx2 = 1 ] && trees="$ref $opt"
asan='-O1 -g -fsanitize=address -fno-omit-frame-pointer -std=c11 -w'
x='-mavx2 -mbmi2 -mpopcnt -mpclmul'
for t in $trees; do
    d="$impl/$t"
    o="$scratch/$(basename "$t")-$(echo "$t" | cut -c1-3)"
    src='KEM_AlgorithmInstance.c kem.c indcpa.c poly.c ntt.c decode.c packing.c reduce.c verify.c drng.c'
    case $t in
        Additional*) src="$src fips202.c rng.c symmetric-shake.c" ;;
        *) src="$src auxfunc.c" ;;
    esac
    case $t in
        Optimized_Implementation/FLIT512)
            (cd "$d" && gcc -O1 -g -std=c11 -w $x -I. -c -o "$o-basemul.o" basemul_avx.c &&
                gcc $asan $x -I. -o "$o" "$driver" $src ntt_avx.c consts.c "$o-basemul.o") >"$o.log" 2>&1 & ;;
        *OPT*|Optimized*)
            (cd "$d" && gcc $asan $x -I. -o "$o" "$driver" $src ntt_avx.c consts.c basemul_avx.c) >"$o.log" 2>&1 & ;;
        *)
            (cd "$d" && gcc $asan -I. -o "$o" "$driver" $src) >"$o.log" 2>&1 & ;;
    esac
done
wait

overread() { # binary mode len api
    "$1" "$2" "$3" >"$scratch/out" 2>&1 && return 1
    grep -Eq 'ERROR: AddressSanitizer: (heap-buffer-overflow|unknown-crash)' "$scratch/out" &&
        grep -q '^READ of size' "$scratch/out" &&
        grep -q "is located 0 bytes after $3-byte region" "$scratch/out" &&
        grep -q "in $4 .*KEM_AlgorithmInstance.c" "$scratch/out"
}
n=0
for t in $trees; do
    b="$scratch/$(basename "$t")-$(echo "$t" | cut -c1-3)"
    [ -x "$b" ] || { cat "$b.log"; bad "$t build failed"; }
    out=$("$b" ct full 2>&1) && ctl=$(echo "$out" | sed -n 's/^ct_len=\([0-9]*\) of .*/\1/p') &&
        case "$out" in *"dec=0 agrees=1") ;; *) false ;; esac || bad "$t ciphertext control: $out"
    out=$("$b" pk full 2>&1) && pkl=$(echo "$out" | sed -n 's/^pk_len=\([0-9]*\) of .*/\1/p') &&
        case "$out" in *"enc=0") ;; *) false ;; esac || bad "$t public-key control: $out"
    for len in 1 $((ctl - 1)); do
        overread "$b" ct $len kem_dec && grep -q 'AddressSanitizer: heap-buffer-overflow' "$scratch/out" &&
            grep -q 'in .*_poly_decompress .*poly.c' "$scratch/out" ||
            { cat "$scratch/out"; bad "$t: no overflow for a $len-byte ciphertext"; }
    done
    # the FLIT512 public key is read by one 3072-byte memcpy, which
    # AddressSanitizer reports as unknown-crash at the start of the region
    overread "$b" pk $((pkl - 1)) kem_enc ||
        { cat "$scratch/out"; bad "$t: no overflow for a $((pkl - 1))-byte public key"; }
    echo "$t: ct $ctl and pk $pkl bytes clean; 1 and $((ctl - 1))-byte ciphertexts and a $((pkl - 1))-byte public key read out of bounds"
    n=$((n + 1))
done
[ $avx2 = 1 ] || echo 'AVX2 trees not run: CPU lacks AVX2/BMI2/PCLMUL (source checks above still cover them)'
echo "$n builds: short ciphertexts and public keys are read out of bounds; full-length controls are clean"
echo "ATTACK kem-15-7: kem_enc and kem_dec ignore the declared input lengths and over-read short buffers: CONFIRMED"
exit 0
