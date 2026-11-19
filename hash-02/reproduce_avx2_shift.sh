#!/bin/sh
# hash-02-7: the optimized AVX2 squeeze shifts a 32-bit word by 32, and
# AVX2 builds with clang return wrong, colliding digests.
#
# axis_soa32_extract_w0_seq32 (Optimized_Implementation/AXIS-*/axis_core.c
# :1874-1879) evaluates soa->w0h[reg_index] << (32U - start_bit), and every
# caller passes start_bit = AXIS_EXL_0 = 0. The helper is compiled whenever
# AVX2 is enabled and produces the digest of every byte-aligned message.
#
# Builds each optimized instance with the Makefile's "avx2" flags, once with
# gcc (control: must match the reference, built with its Makefile flags) and
# once with clang (must differ from the reference on every byte-aligned
# message, and give one digest for two different byte-aligned messages or
# different digests in two runs); the partial-byte message, which takes
# another path, must match in both. A gcc -O0 -mavx2 build is also compared
# and reported.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-02-7: gcc unavailable'; exit 77; }
CLANG=$(command -v clang-21 || command -v clang) || { echo 'SKIP hash-02-7: clang unavailable'; exit 77; }
grep -qw avx2 /proc/cpuinfo 2>/dev/null || { echo 'SKIP hash-02-7: no AVX2 CPU'; exit 77; }
bad() { echo "NOT CONFIRMED hash-02-7: $1"; exit 1; }
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
echo "clang: $("$CLANG" --version | head -n 1)"
AVX2="-std=c99 -O3 -flto -fomit-frame-pointer -frename-registers -mavx2 -mno-bmi -mno-bmi2 -mno-avx512f -DAXIS_OPT_LEVEL=2"
impl=hash-02/AXIS/Implementations
build() { # out dir compiler flags...
    o=$1; d=$2; shift 2
    "$@" -w -DDIG="$bits" -I"$d" -o "$scratch/$o" "$here/reproduce_avx2_shift.c" \
        "$d/CryptHash_AlgorithmInstance.c" "$d/axis_core.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$o AXIS-$bits build failed"; }
    "$scratch/$o" >"$scratch/$o.out" || bad "$o AXIS-$bits run failed"
}
for bits in 512 768 1024; do
    opt="$impl/Optimized_Implementation/AXIS-$bits"
    grep -Fq '(soa->w0h[reg_index] << (32U - start_bit))' "$opt/axis_core.c" || bad "AXIS-$bits source differs from the archive"
    build ref "$impl/Reference_Implementation/AXIS-$bits" gcc -std=c99 -O2
    build gcc_avx2 "$opt" gcc $AVX2
    build clang_avx2 "$opt" "$CLANG" $AVX2
    build gcc_O0 "$opt" gcc -std=c99 -O0 -mavx2
    cmp -s "$scratch/ref.out" "$scratch/gcc_avx2.out" || bad "AXIS-$bits: gcc avx2 build differs from the reference"
    for b in clang_avx2 gcc_O0; do
        diffs=$(paste -d ' ' "$scratch/ref.out" "$scratch/$b.out" | awk '$2 != $4 {n++} END {print n + 0}')
        tail_ok=$(paste -d ' ' "$scratch/ref.out" "$scratch/$b.out" | awk '$1 == "pattern8191" {print ($2 == $4)}')
        distinct=$(awk '$1 != "pattern8191" {print $2}' "$scratch/$b.out" | sort -u | wc -l | tr -d ' ')
        echo "  AXIS-$bits $b: $diffs of 4 byte-aligned digests differ from the reference; partial-byte digest matches: $tail_ok; distinct byte-aligned digests: $distinct of 4"
        [ "$diffs" -eq 4 ] && [ "$tail_ok" -eq 1 ] || bad "AXIS-$bits $b: unexpected comparison"
    done
    [ "$distinct" -ge 1 ] || bad "AXIS-$bits: no output"
    cdist=$(awk '$1 != "pattern8191" {print $2}' "$scratch/clang_avx2.out" | sort -u | wc -l | tr -d ' ')
    "$scratch/clang_avx2" >"$scratch/clang_avx2.again" || bad "AXIS-$bits clang rerun failed"
    if cmp -s "$scratch/clang_avx2.out" "$scratch/clang_avx2.again"; then rerun=same; else rerun=different; fi
    echo "  AXIS-$bits clang_avx2: a second run gives $rerun digests"
    [ "$cdist" -lt 4 ] || [ "$rerun" = different ] || bad "AXIS-$bits clang build: digests neither collide nor vary between runs"
    echo "  AXIS-$bits clang abc: $(awk '$1 == "abc" {print substr($2, 1, 32)}' "$scratch/clang_avx2.out")...  reference: $(awk '$1 == "abc" {print substr($2, 1, 32)}' "$scratch/ref.out")..."
done
echo "gcc avx2 builds match the reference; clang avx2 builds and gcc -O0 builds give wrong byte-aligned digests, and the clang builds collide"
echo "ATTACK hash-02-7: shift by 32 in the AVX2 squeeze makes clang builds with the Makefile AVX2 flags return wrong, colliding digests: CONFIRMED"
exit 0
