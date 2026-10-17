#!/bin/sh
# kem-33-4: the optimized QUBE FO check compares only a prefix of u and v.
# Builds the submitted Optimized_Implementation (qube-1/3/4/5) as shared
# libraries in a temporary directory, then runs the ctypes check in
# kem-33/reproduce_opt_partial_compare.py against them, with the prebuilt
# reference libraries in kem-33/lib as the control.
set -u
cd "$(dirname "$0")/.." || exit 2

command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-33-4: gcc unavailable'; exit 77; }
command -v python3 >/dev/null 2>&1 || { echo 'SKIP kem-33-4: python3 unavailable'; exit 77; }
case "$(uname -m)" in x86_64|amd64) ;; *) echo 'SKIP kem-33-4: optimized tree is x86-64/AVX2 only'; exit 77 ;; esac
grep -q '\bavx2\b' /proc/cpuinfo 2>/dev/null || { echo 'SKIP kem-33-4: AVX2 unavailable'; exit 77; }
for s in 128 256 384 512; do
    [ -f "kem-33/lib/libqube-$s.so" ] || { echo "SKIP kem-33-4: build the reference control first (make -C kem-33)"; exit 77; }
done

scratch=$(mktemp -d) || exit 2
case "$scratch" in /tmp/tmp.*) ;; *) echo 'unexpected temporary path' >&2; exit 2 ;; esac
trap 'rm -rf "$scratch"' EXIT HUP INT TERM

cp -rL kem-33/Implementations/Optimized_Implementation "$scratch/opt" || exit 2

# Flags follow Optimized_Implementation/CMakeLists.txt (without -flto and
# -mtune=native); -w and -Wno-error=incompatible-pointer-types for GCC >= 14.
for p in 1 3 4 5; do
    (
        cd "$scratch/opt" || exit 2
        P=src/x86/qube-$p
        gcc -shared -fPIC -O3 -march=x86-64 -mavx2 -mavx -mbmi -mpclmul \
            -fomit-frame-pointer -std=c99 -D_GNU_SOURCE -w \
            -Wno-error=incompatible-pointer-types \
            -I"$P" -Isrc/common -Isrc/x86 -Ilib/fips202 -Ilib/api_pkc \
            -Ilib/bitpolymul_x86 -Ibenchmark \
            src/common/*.c src/x86/*.c lib/api_pkc/*.c lib/bitpolymul_x86/*.c \
            lib/fips202/fips202.c "$P"/*.c \
            -o "$scratch/libqube-opt-$p.so"
    ) || { echo "NOT CONFIRMED kem-33-4: optimized qube-$p failed to build" >&2; exit 1; }
done

python3 kem-33/reproduce_opt_partial_compare.py "$scratch"
