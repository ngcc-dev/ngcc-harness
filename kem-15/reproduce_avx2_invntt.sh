#!/bin/sh
# kem-15-4: the FLIT512 AVX2 inverse NTT overflows int16 and makes the
# optimized tree fail its own encapsulation/decapsulation round trip.
#
# Builds the archived FLIT512 reference and AVX2 trees with the archive's own
# compiler flags and runs the KAT seed stream through the archived API. The
# AVX2 tree must fail at least once, first at KAT-stream iteration 11058; the
# reference tree, and the AVX2 tree with only the scalar inverse NTT swapped
# in, must not fail. An instrumented AVX2 build must show a 16-bit wraparound
# during encapsulation in every failing round trip, and no failure after a
# wraparound during decapsulation. NGCC_LONG=1 runs 50,000 iterations and adds
# the FLIT128 and FLIT256 AVX2 trees as controls.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-15-4: gcc unavailable'; exit 77; }
grep -q avx2 /proc/cpuinfo 2>/dev/null && grep -q bmi2 /proc/cpuinfo &&
    grep -q pclmulqdq /proc/cpuinfo ||
    { echo 'SKIP kem-15-4: AVX2/BMI2/PCLMUL CPU required'; exit 77; }

base=kem-15/Implementations
ref=$base/Reference_Implementation/FLIT512
opt=$base/Optimized_Implementation/FLIT512
grep -q 'qm1 = Q-1 for conditional subtraction (sum < 2Q guaranteed in INTT)' "$opt/ntt_avx.c" &&
    grep -q '^    invntt_avx(r->coeffs, qdata);$' "$opt/poly.c" &&
    grep -q '^                rj = _mm256_add_epi16(t, rjlen);$' "$opt/ntt_avx.c" ||
    { echo 'NOT CONFIRMED kem-15-4: archived FLIT512 AVX2 sources differ'; exit 1; }

n=12000
[ "${NGCC_LONG:-0}" = 1 ] && n=50000
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
driver=$PWD/kem-15/reproduce_avx2_roundtrip.c
ref_flags='-O3 -march=native -fomit-frame-pointer -std=c11'
opt_flags='-mavx2 -mbmi2 -mpopcnt -mpclmul -march=native -mtune=native -O2 -fomit-frame-pointer -std=c11'
ref_src='kem.c ntt.c poly.c decode.c packing.c reduce.c verify.c auxfunc.c drng.c indcpa.c KEM_AlgorithmInstance.c'
opt_src='kem.c ntt.c ntt_avx.c basemul_avx.c poly.c decode.c packing.c reduce.c verify.c auxfunc.c drng.c consts.c indcpa.c KEM_AlgorithmInstance.c'

build() { # name source-dir flags sources
    cp -R "$2" "$scratch/$1" || exit 2
    case $1 in
        scalar) sed -i 's/^    invntt_avx(r->coeffs, qdata);$/    invntt_tomont(r->coeffs);/' "$scratch/$1/poly.c" ;;
        probe) sed -i -e '1i long ngcc_overflow;' \
            -e 's/^                rj = _mm256_add_epi16(t, rjlen);$/&\n                if (_mm256_movemask_epi8(_mm256_cmpeq_epi16(rj, _mm256_adds_epi16(t, rjlen))) != -1) ngcc_overflow++;/' \
            "$scratch/$1/ntt_avx.c" ;;
    esac
    # shellcheck disable=SC2086
    (cd "$scratch/$1" && gcc $3 -I. -o "$scratch/$1.bin" $4 "$driver") 2>"$scratch/cc" ||
        { cat "$scratch/cc"; echo "NOT CONFIRMED kem-15-4: $1 build failed"; exit 1; }
}
run() { # name -> sets fails first enc_wraps dec_wraps fail_without_enc_wrap dec_wrap_fails
    out=$("$scratch/$1.bin" "$n") || { echo "NOT CONFIRMED kem-15-4: $1 run failed"; exit 1; }
    eval "$(echo "$out" | tr ' ' '\n' | grep -E '^(fails|first|enc_wraps|dec_wraps|fail_without_enc_wrap|dec_wrap_fails)=-?[0-9]+$')"
    echo "$1: $out"
}

build reference "$ref" "$ref_flags" "$ref_src"
build avx2 "$opt" "$opt_flags" "$opt_src"
build scalar "$opt" "$opt_flags" "$opt_src"
build probe "$opt" "$opt_flags" "$opt_src"

run reference
[ "$fails" -eq 0 ] || { echo 'NOT CONFIRMED kem-15-4: reference tree failed a round trip'; exit 1; }
run avx2
avx2_fails=$fails
[ "$fails" -ge 1 ] && [ "$first" -eq 11058 ] ||
    { echo 'NOT CONFIRMED kem-15-4: AVX2 tree did not fail first at iteration 11058'; exit 1; }
run scalar
[ "$fails" -eq 0 ] || { echo 'NOT CONFIRMED kem-15-4: AVX2 tree with scalar inverse NTT failed'; exit 1; }
run probe
[ "$fails" -eq "$avx2_fails" ] && [ "$fail_without_enc_wrap" -eq 0 ] && [ "$dec_wrap_fails" -eq 0 ] ||
    { echo 'NOT CONFIRMED kem-15-4: failures not explained by encapsulation-side wraparound'; exit 1; }
probe_dec_wraps=$dec_wraps

if [ "${NGCC_LONG:-0}" = 1 ]; then
    for l in 128 256; do
        build "avx2_$l" "$base/Optimized_Implementation/FLIT$l" "$opt_flags" "$opt_src"
        run "avx2_$l"
        [ "$fails" -eq 0 ] || { echo "NOT CONFIRMED kem-15-4: FLIT$l AVX2 control failed a round trip"; exit 1; }
    done
    long_note="; FLIT128/256 AVX2 controls 0"
fi
echo "CONFIRMED kem-15-4: FLIT512 AVX2 failed $avx2_fails of $n own round trips, each after an int16 wraparound in invntt_avx during encapsulation; $probe_dec_wraps decapsulation-side wraparounds changed no key; reference and scalar-inverse controls 0${long_note:-}"
exit 0
