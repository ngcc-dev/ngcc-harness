#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_dir="$root/Implementations and Test_Vectors/Implementations/Reference_Implementation/Rhyme-SHAKE/Rhyme-SHAKE-512"
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT

cc -O1 -g -fsanitize=address -fno-omit-frame-pointer -std=c99 \
    -DRHYME_MODE=512 -DRHYME_NO_AES \
    -I"$source_dir" -I"$source_dir/include" -I"$source_dir/src" \
    "$root/reproduce_encoder_bound.c" "$source_dir/src/encoding.c" \
    -o "$scratch/encoder"
ASAN_OPTIONS=detect_leaks=0 CONTROL=1 "$scratch/encoder" >"$scratch/control.out" 2>"$scratch/control.err"
grep -q 'z1\[0\]=0 encoded=' "$scratch/control.out"
if ASAN_OPTIONS=detect_leaks=0:exitcode=91 "$scratch/encoder" >"$scratch/attack.out" 2>"$scratch/attack.err"; then
    echo 'NOT CONFIRMED sign-22-5: boundary unexpectedly encoded' >&2
    exit 1
fi
grep -q 'AddressSanitizer: global-buffer-overflow' "$scratch/attack.err"
grep -q 'encoding.c:142' "$scratch/attack.err"
echo 'CONFIRMED sign-22-5: valid-bound response overreads the Rhyme-512 encoder table'
