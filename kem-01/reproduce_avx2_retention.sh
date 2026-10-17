#!/bin/sh
# kem-01-1 scope extension: build the unmodified Aigis-Enc+ AVX2 trees in a
# temporary directory and sweep every single-bit ciphertext flip. A decapsulation
# that returns the challenge key for a modified ciphertext shows that implicit
# rejection is ineffective in the optimized code as well.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-01-1 AVX2: gcc unavailable'; exit 77; }
grep -qw avx2 /proc/cpuinfo 2>/dev/null || { echo 'SKIP kem-01-1 AVX2: CPU lacks AVX2'; exit 77; }
grep -qw bmi2 /proc/cpuinfo 2>/dev/null || { echo 'SKIP kem-01-1 AVX2: CPU lacks BMI2'; exit 77; }

base=kem-01/Implementations/Optimized_Implementation/avx2
[ -d "$base" ] || { echo "NOT CONFIRMED kem-01-1: missing $base"; exit 1; }

scratch=$(mktemp -d) || exit 2
case "$scratch" in /tmp/tmp.*|"${TMPDIR:-/tmp}"/tmp.*) ;; *) echo 'unexpected temporary path' >&2; exit 2 ;; esac
trap 'rm -rf "$scratch"' EXIT HUP INT TERM

cflags='-O2 -mavx2 -mbmi2 -mpopcnt -maes -std=c99 -w -Wno-incompatible-pointer-types -Wno-implicit-function-declaration -Wno-int-conversion'
srcs='cbd.c fips202.c fips202x4.c poly.c precomp.c ntt.c polyvec.c owcpa.c kem.c verify.c
      hashkdf.c genmatrix.c auxfunc.c KEM_AlgorithmInstance.c drng.c pack.c rng.c
      keccak4x/KeccakP-1600-times4-SIMD256.c'
status=0
for p in 1 2 3; do
    case $p in 1) dir=Aigis-Enc+-I ;; 2) dir=Aigis-Enc+-II ;; 3) dir=Aigis-Enc+-III ;; esac
    work=$scratch/$p
    cp -rL "$base/$dir" "$work" || exit 2
    rm -f "$work"/keccak4x/*.o                 # rebuild the shipped object from source
    cp kem-01/reproduce_avx2_retention.c "$work/driver.c" || exit 2
    # Confirm the defect line is present in the tree being run.
    grep -n 'cmov(buf, buf2, SEED_BYTES, fail)' "$work/kem.c" >/dev/null ||
        { echo "NOT CONFIRMED kem-01-1: AVX2 $dir kem.c differs"; exit 1; }
    ( cd "$work" && gcc $cflags -DPARAMS=$p -DUSE_NICCS_API -I. -Ikeccak4x \
        $srcs driver.c -o run ) || { echo "NOT CONFIRMED kem-01-1: AVX2 $dir build failed"; exit 1; }
    out=$("$work/run") || { echo "NOT CONFIRMED kem-01-1: AVX2 $dir driver failed"; exit 1; }
    echo "AVX2 $dir: $out"
    honest=$(printf '%s\n' "$out" | sed -n 's/.*honest=\([0-9]*\).*/\1/p')
    second=$(printf '%s\n' "$out" | sed -n 's/.*second_differs=\([0-9]*\).*/\1/p')
    keep=$(printf '%s\n' "$out" | sed -n 's/.*keep=\([0-9]*\).*/\1/p')
    total=$(printf '%s\n' "$out" | sed -n 's/.*total=\([0-9]*\).*/\1/p')
    if [ "$honest" = 1 ] && [ "$second" = 1 ] && [ "$keep" -gt 0 ] && [ "$keep" -lt "$total" ]; then
        echo "  control: honest decapsulation matches; second encapsulation key differs; $((total - keep)) flips change the key"
    else
        echo "  unexpected result for $dir"
        status=1
    fi
done
if [ "$status" -eq 0 ]; then
    echo 'CONFIRMED kem-01-1: all three AVX2 Aigis-Enc+ trees return the challenge key for most single-bit-modified ciphertexts'
    exit 0
fi
echo 'NOT CONFIRMED kem-01-1: AVX2 sweep did not show retention with passing controls'
exit 1
