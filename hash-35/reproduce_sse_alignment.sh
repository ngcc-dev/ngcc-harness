#!/bin/sh
# hash-35-4: the Wish x86 implementations load message and padding blocks
# with _mm_load_si128, which faults on addresses that are not 16-byte aligned
# when built with the shipped -march=x86-64 -maes flags (legacy SSE).
#
# All four x86 wish.c files define LOAD(p) as _mm_load_si128 (wish.c:17) and
# apply it to the caller's message, msg + 64*i or msg + 128*i (Wish512 :157
# resource, :163 performance; Wish1024 :181, :187), and to the stack array
# padded_block, which has no alignment attribute (Wish512 :161, Wish1024 :185
# resource; :167, :191 performance).
#
# Each directory is built with exactly its archived Makefile's CFLAGS (gcc) and run on
# messages at 64-byte aligned addresses plus an offset, each call in a child:
#  - resource-optimized: an aligned 24-bit message must crash (padded_block
#    lands at an address that is 8 mod 16);
#  - performance-optimized: an aligned one-block message must match the
#    reference, the same message at offset 1 and a two-block message at
#    offset 8 must crash;
#  - reference (control): every case completes with the same digest.
# If clang is installed the x86 directories are also built with clang and the
# Makefile flags, and an unaligned one-block message must crash there too.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-35-4: gcc unavailable'; exit 77; }
[ "$(uname -m)" = x86_64 ] && grep -qw aes /proc/cpuinfo 2>/dev/null || { echo 'SKIP hash-35-4: needs an x86-64 CPU with AES-NI'; exit 77; }
bad() { echo "NOT CONFIRMED hash-35-4: $1"; exit 1; }
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
impl=hash-35/Wish/Implementations
build() { # out dir bits compiler flags...
    o=$1; d=$2; b=$3; shift 3
    grep -q 'define LOAD(p) _mm_load_si128' "$d/wish.c" 2>/dev/null || [ "$o" = ref ] || bad "$d differs from the archive"
    "$@" -w -DDIG="$b" -I"$d" -o "$scratch/$o" "$here/reproduce_sse_alignment.c" \
        "$d/CryptHash_AlgorithmInstance.c" "$d/wish.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$o Wish$b build failed"; }
}
# the archived Makefiles' CFLAGS; checked against a Makefile when one is
# present (the harness copy has none)
mkflags() {
    case $1 in
        *_resource_optimized) echo '-Os -march=x86-64 -maes -flto -fomit-frame-pointer -std=c99 -Wpedantic -Wall -Wextra' ;;
        *_performance_optimized) echo '-O3 -march=x86-64 -maes -mtune=native -flto -fomit-frame-pointer -std=c99 -Wpedantic -Wall -Wextra' ;;
    esac
}
for d in "$impl"/Additional_Implementation/Wish*_x86_resource_optimized "$impl"/Optimized_Implementation/Wish*_x86_performance_optimized; do
    [ -f "$d/Makefile" ] || continue
    [ "$(sed -n 's/^CFLAGS *= *//p' "$d/Makefile" | head -n 1 | tr -s ' ' | sed 's/ *$//')" = "$(mkflags "$d")" ] ||
        bad "$d/Makefile CFLAGS differ from the archived flags"
done
expect() { # binary offset len pattern
    out=$("$scratch/$1" "$2" "$3") || bad "$1: driver failed"
    echo "  $1: $out"
    case "$out" in *"$4"*) ;; *) bad "$1 offset $2 length $3: expected $4" ;; esac
}
CLANG=$(command -v clang-21 || command -v clang || true)
for b in 512 1024; do
    blk=$((b)); two=$((2 * b + 8))
    ref="$impl/Reference_Implementation/Wish${b}_reference"
    perf="$impl/Optimized_Implementation/Wish${b}_x86_performance_optimized"
    res="$impl/Additional_Implementation/Wish${b}_x86_resource_optimized"
    build ref "$ref" $b gcc -std=c99 -O2
    build perf "$perf" $b gcc $(mkflags "$perf")
    build res "$res" $b gcc $(mkflags "$res")
    echo "Wish$b: performance CFLAGS = $(mkflags "$perf"); resource CFLAGS = $(mkflags "$res")"
    want=$("$scratch/ref" 0 $blk | sed 's/.*digest //')
    expect ref 1 $blk "digest $want"
    expect ref 8 $two "digest"
    expect res 0 24 "killed by SIGSEGV"
    expect perf 0 $blk "digest $want"
    expect perf 1 $blk "killed by SIGSEGV"
    expect perf 8 $two "killed by SIGSEGV"
    if [ -n "$CLANG" ]; then
        build cperf "$perf" $b "$CLANG" $(mkflags "$perf")
        build cres "$res" $b "$CLANG" $(mkflags "$res")
        expect cres 0 24 "digest"
        expect cres 3 $blk "killed by SIGSEGV"
        expect cperf 1 $blk "killed by SIGSEGV"
    fi
done
echo "with the shipped flags the resource-optimized x86 builds crash on aligned honest calls and all x86 builds crash on unaligned messages of a block or more; the reference does not"
echo "ATTACK hash-35-4: aligned SSE loads on caller and stack buffers crash the Wish x86 implementations: CONFIRMED"
exit 0
