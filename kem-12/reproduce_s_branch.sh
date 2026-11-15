#!/bin/sh
# kem-12-3: the reference CTL-512 multiplication skips zero coefficients of
# the secret binary polynomial s, so its instruction count follows s.
#
# Builds ctl_encrypt_3329 from the archived reference and optimized
# CTL-3329-2048 trees and counts executed instructions with callgrind for s of
# weight 0 and 2048. The reference count must grow by more than half; the
# optimized (NTT) count is the control and must stay within 1%. Both trees must
# return the same ciphertext.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-12-3: gcc unavailable'; exit 77; }
command -v valgrind >/dev/null 2>&1 || { echo 'SKIP kem-12-3: valgrind unavailable'; exit 77; }

base='kem-12/Implementations and Test_Vectors'
ref="$base/KEM-CTL-x86-Reference_Implementation/crypto_kem/Implementations/CTL-3329-2048"
grep -q 'if (b\[j\] == 0) {' "$ref/kem3329.c" ||
    { echo 'NOT CONFIRMED kem-12-3: reference kem3329.c differs from the archived multiplication'; exit 1; }

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
for tree in Reference Optimized; do
    dir="$base/KEM-CTL-x86-${tree}_Implementation/crypto_kem/Implementations/CTL-3329-2048"
    gcc -O2 -w -ffunction-sections -Wl,--gc-sections -I"$dir" -Iapi -o "$scratch/$tree" \
        kem-12/reproduce_s_branch.c "$dir/kem3329.c" -lm 2>"$scratch/cc" ||
        { cat "$scratch/cc"; echo "NOT CONFIRMED kem-12-3: $tree build failed"; exit 1; }
    for w in 0 2048; do
        valgrind --tool=callgrind --callgrind-out-file=/dev/null "$scratch/$tree" "$w" \
            >"$scratch/out" 2>"$scratch/err" ||
            { echo "NOT CONFIRMED kem-12-3: $tree run failed"; exit 1; }
        ir=$(sed -n 's/.*Collected : \([0-9]*\).*/\1/p' "$scratch/err")
        set -- $(cat "$scratch/out")
        [ "$2" = 1 ] && [ -n "$ir" ] || { echo "NOT CONFIRMED kem-12-3: $tree weight $w gave no result"; exit 1; }
        echo "$tree weight $w: $ir instructions, ciphertext digest $3"
        eval "ir_${tree}_$w=$ir; dg_${tree}_$w=$3"
    done
done
[ "$dg_Reference_0" = "$dg_Optimized_0" ] && [ "$dg_Reference_2048" = "$dg_Optimized_2048" ] ||
    { echo 'NOT CONFIRMED kem-12-3: reference and optimized ciphertexts differ'; exit 1; }
[ $((ir_Reference_2048 * 2)) -gt $((ir_Reference_0 * 3)) ] ||
    { echo 'NOT CONFIRMED kem-12-3: reference instruction count does not follow the weight of s'; exit 1; }
d=$((ir_Optimized_2048 - ir_Optimized_0)); [ "$d" -lt 0 ] && d=$((-d))
[ $((d * 100)) -lt "$ir_Optimized_0" ] ||
    { echo 'NOT CONFIRMED kem-12-3: optimized control varies by more than 1%'; exit 1; }
echo "CONFIRMED kem-12-3: reference CTL-512 encryption runs $ir_Reference_0 instructions for weight-0 s and $ir_Reference_2048 for weight 2048; the optimized tree stays within 1% and returns the same ciphertexts"
exit 0
