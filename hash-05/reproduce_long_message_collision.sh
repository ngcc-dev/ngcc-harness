#!/bin/sh
# hash-05-6: uHash casts 64-bit message lengths to 32-bit int, so messages of
# 4 GiB or more are silently truncated and collide.
#
# Padding() and uHash512/768/1024 compute row, actual_len_bytes and
# msg_len_bytes through (int) casts (reference CryptHash_AlgorithmInstance.c
# :450-453, :492-494, :652-654, :847-849; optimized :6242-6245, :6378-6383,
# :6545-6548). At 128 + 2^35 bits (uHash-768, uHash-1024) and 128 + 3*2^35
# bits (uHash-512) every count wraps to its value for a 128-bit message, so
# only bytes 0..15 are read. The driver hashes two messages of that length
# that differ in bytes 16..79, in a middle byte and in the last byte, and the
# 16-byte message with the same prefix: all three digests must be equal.
# Control: two 1 KiB messages differing in bytes 16..79 must differ. It also
# hashes an all-zero 2^34-bit (2 GiB) message in a child process, which must
# die with SIGSEGV in the reference implementation.
#
# The long messages are sparse MAP_NORESERVE mappings; runtime is a few seconds.
# The optimized trees are Windows AVX2 sources; they are built with stand-in
# windows.h/intrin.h headers and only on a CPU with AVX2.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-05-6: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED hash-05-6: $1"; exit 1; }
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
impl=$(find hash-05/uHash/Implementations -type d -name Reference_Implementation | head -n 1)
[ -n "$impl" ] || bad "reference tree not found"
impl=${impl%/Reference_Implementation}

mkdir "$scratch/stub"
cat >"$scratch/stub/windows.h" <<'EOF'
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>
#include <x86intrin.h>
#define uint64_t uhash_u64_t /* the source re-typedefs uint64_t */
#define _ReadWriteBarrier() __asm__ __volatile__("" ::: "memory")
#define nullptr NULL
EOF
printf '#include <immintrin.h>\n' >"$scratch/stub/intrin.h"

trees=Reference
if grep -qw avx2 /proc/cpuinfo 2>/dev/null; then trees="Reference Optimized"; else echo 'no AVX2 CPU: optimized tree skipped'; fi

n=0
for tree in $trees; do
    for bits in 512 768 1024; do
        d="$impl/${tree}_Implementation/uHash-$bits"
        src="$d/CryptHash_AlgorithmInstance.c"
        grep -q 'msg_len_bytes = (int)(msg_len_bits / 8);' "$src" || bad "$tree uHash-$bits source differs from the archive"
        flags=""
        [ "$tree" = Optimized ] && flags="-mavx2 -I$scratch/stub"
        gcc -O2 -w $flags -DDIG="$bits" -I"$d" -o "$scratch/t" "$here/reproduce_long_message_collision.c" "$src" \
            2>"$scratch/cc" || { cat "$scratch/cc"; bad "$tree uHash-$bits build failed"; }
        "$scratch/t" collide >"$scratch/out"
        rc=$?
        sed "s/^/  $tree: /" "$scratch/out"
        [ $rc -eq 77 ] && { echo "SKIP hash-05-6: sparse mapping refused"; exit 77; }
        [ $rc -eq 0 ] || bad "$tree uHash-$bits run failed"
        grep -qx 'same_length_collision=1 cross_length_collision=1 control_1KiB_differ=1' "$scratch/out" ||
            bad "$tree uHash-$bits: no collision"
        if [ "$tree" = Reference ]; then
            out=$("$scratch/t" crash) || bad "$tree uHash-$bits crash run failed"
            echo "  $tree: $out"
            case "$out" in *SIGSEGV*) ;; *) bad "$tree uHash-$bits: no crash at 2^34 bits" ;; esac
        fi
        n=$((n + 1))
    done
done
echo "$n instances: two different same-length long messages and the 128-bit prefix share one digest; the 1 KiB control differs; the reference crashes on a 2^34-bit message"
echo "ATTACK hash-05-6: 32-bit length truncation gives same-length collisions for 4 GiB (uHash-768/1024) and 12 GiB (uHash-512) messages: CONFIRMED"
exit 0
