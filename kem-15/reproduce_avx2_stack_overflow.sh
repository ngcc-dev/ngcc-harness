#!/bin/sh
# kem-15-5: the FLIT512 AVX2 base multiplication and inversion access their
# stack arrays out of bounds.
#
# Builds the archived FLIT512 AVX2 tree with AddressSanitizer and requires
# stack-buffer-overflow writes in both baseinv_avx_32x and basemul_avx_16x on
# the first key pair. With the six 8-element and two 2x8-element arrays
# enlarged, the same build must run cleanly. If an x86 clang is available, the
# unmodified tree built with clang must fail every round trip, and the enlarged
# build must not fail any.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-15-5: gcc unavailable'; exit 77; }
grep -q avx2 /proc/cpuinfo 2>/dev/null && grep -q bmi2 /proc/cpuinfo &&
    grep -q pclmulqdq /proc/cpuinfo ||
    { echo 'SKIP kem-15-5: AVX2/BMI2/PCLMUL CPU required'; exit 77; }

opt=kem-15/Implementations/Optimized_Implementation/FLIT512
arrays16='int16_t ea\[8\],oa\[8\],eb\[8\],ob\[8\],sa\[8\],sb\[8\];'
arrays32='int16_t e\[2\]\[8\], o\[2\]\[8\], esq\[2\]\[8\], osq\[2\]\[8\], norm\[2\]\[8\], ninv\[2\]\[8\];'
grep -q "$arrays16" "$opt/basemul_avx.c" && grep -q "$arrays32" "$opt/basemul_avx.c" &&
    grep -q 'for(int h=0;h<8;h+=4){' "$opt/basemul_avx.c" &&
    grep -q '_mm256_storeu_si256((__m256i \*)esq\[k\], ev);' "$opt/basemul_avx.c" ||
    { echo 'NOT CONFIRMED kem-15-5: archived FLIT512 AVX2 sources differ'; exit 1; }

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
driver=$PWD/kem-15/reproduce_avx2_roundtrip.c
src='kem.c ntt.c ntt_avx.c basemul_avx.c poly.c decode.c packing.c reduce.c verify.c auxfunc.c drng.c consts.c indcpa.c KEM_AlgorithmInstance.c'
isa='-mavx2 -mbmi2 -mpopcnt -mpclmul'
cp -R "$opt" "$scratch/shipped" && cp -R "$opt" "$scratch/enlarged" || exit 2
sed -i -e "s/$arrays16/int16_t ea[12],oa[12],eb[12],ob[12],sa[12],sb[12];/" \
    -e "s/$arrays32/int16_t e[2][8], o[2][8], esq[2][16], osq[2][16], norm[2][8], ninv[2][8];/" \
    "$scratch/enlarged/basemul_avx.c"

build() { # tree compiler flags output
    # shellcheck disable=SC2086
    (cd "$scratch/$1" && $2 $3 -std=c11 -w -I. -o "$scratch/$4" $src "$driver") 2>"$scratch/cc" ||
        { cat "$scratch/cc"; echo "NOT CONFIRMED kem-15-5: $4 build failed"; exit 1; }
}

asan='-O1 -g -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer'
build shipped gcc "$isa $asan" asan_shipped
build enlarged gcc "$isa $asan" asan_enlarged
ASAN_OPTIONS=detect_leaks=0:halt_on_error=0 "$scratch/asan_shipped" 1 >/dev/null 2>"$scratch/e"
for f in baseinv_avx_32x basemul_avx_16x; do
    grep -A2 '^WRITE of size' "$scratch/e" | grep -q "in $f " ||
        { cat "$scratch/e"; echo "NOT CONFIRMED kem-15-5: no out-of-bounds write in $f"; exit 1; }
done
grep -c 'stack-buffer-overflow' "$scratch/e" | sed 's/^/shipped gcc ASan: stack-buffer-overflow reports=/'
grep -A2 -E '^(READ|WRITE) of size' "$scratch/e" | grep -o -E '^(READ|WRITE) of size [0-9]+|in (basemul_avx_16x|baseinv_avx_32x) [^ ]*basemul_avx.c:[0-9]+' |
    paste - - | sed -e 's/in [a-z0-9_]* [^ ]*\/basemul_avx.c/basemul_avx.c/' -e 's/^/  /' | sort -u
out=$(ASAN_OPTIONS=detect_leaks=0 "$scratch/asan_enlarged" 20 2>&1) &&
    echo "$out" | grep -q '^runs=20 fails=0 ' ||
    { echo "$out"; echo 'NOT CONFIRMED kem-15-5: enlarged-array ASan control failed'; exit 1; }
echo "enlarged gcc ASan: $out"

clang=
for c in clang clang-22 clang-21 clang-20 clang-19 clang-18 clang-17 clang-16 clang-15 clang-14; do
    command -v "$c" >/dev/null 2>&1 || continue
    printf '#include <immintrin.h>\nint main(void){return _mm256_extract_epi16(_mm256_set1_epi16(0),0);}\n' > "$scratch/p.c"
    "$c" $isa -o "$scratch/p" "$scratch/p.c" 2>/dev/null && "$scratch/p" && { clang=$c; break; }
done
if [ -n "$clang" ]; then
    build shipped "$clang" "$isa -O2" clang_shipped
    build enlarged "$clang" "$isa -O2" clang_enlarged
    a=$("$scratch/clang_shipped" 200) && b=$("$scratch/clang_enlarged" 200) ||
        { echo 'NOT CONFIRMED kem-15-5: clang run failed'; exit 1; }
    echo "shipped $clang: $a"
    echo "enlarged $clang: $b"
    echo "$a" | grep -q '^runs=200 fails=200 ' && echo "$b" | grep -q '^runs=200 fails=0 ' ||
        { echo 'NOT CONFIRMED kem-15-5: clang round trips do not match'; exit 1; }
    note="; $clang build fails 200/200 round trips, 0/200 with the arrays enlarged"
else
    note="; x86 clang unavailable, compiler-dependence check skipped"
fi
echo "CONFIRMED kem-15-5: FLIT512 AVX2 basemul_avx_16x and baseinv_avx_32x write past their stack arrays$note"
exit 0
