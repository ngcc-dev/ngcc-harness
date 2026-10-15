#!/bin/sh
# Exercise exact-size and short-buffer calls against the frozen DOVE reference C.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-09-3: gcc unavailable'; exit 77; }

base=sign-09/Implementations
ref=$base/Digital_Signature-DOVE-x86-reference_implementation/API_PKC/Implementations/Reference_Implementation
opt=$base/Digital_Signature-DOVE-x86-performance_optimal_implementation/API_PKC/Implementations/Reference_Implementation
for variant in DOVE_classic_ref DOVE_pkc_skc_ref; do
    cmp "$ref/$variant/SIG_AlgorithmInstance.c" "$opt/$variant/SIG_AlgorithmInstance.c" >/dev/null || exit 1
done

scratch=$(mktemp -d) || exit 2
case "$scratch" in /tmp/tmp.*) ;; *) echo 'unexpected temporary path' >&2; exit 2 ;; esac
trap 'rm -f "$scratch/classic" "$scratch/seed"; rmdir "$scratch"' EXIT HUP INT TERM

build() {
    variant=$1
    output=$2
    dir=$ref/$variant
    gcc -g -O1 -fno-omit-frame-pointer -fsanitize=address -DDOVE_PARAM_SET=DOVE128 \
        -I"$dir" -I"$dir/uov" \
        sign-09/reproduce_input_lengths.c \
        "$dir/SIG_AlgorithmInstance.c" "$dir/auxfunc.c" "$dir/drng.c" \
        "$dir/gf256.c" "$dir/matrix.c" "$dir/uov/blas_matrix_ref.c" \
        -o "$output" || exit 1
}
build DOVE_classic_ref "$scratch/classic"
build DOVE_pkc_skc_ref "$scratch/seed"

check() {
    label=$1
    binary=$2
    mode=$3
    output=$(ASAN_OPTIONS=detect_leaks=0 "$binary" "$mode" 2>&1)
    rc=$?
    if [ "$rc" -eq 0 ] || ! printf '%s\n' "$output" | rg -q 'AddressSanitizer: heap-buffer-overflow'; then
        printf 'NOT REPRODUCED sign-09-3 (%s): rc=%s\n%s\n' "$label" "$rc" "$output" >&2
        exit 1
    fi
    printf 'CONFIRMED sign-09-3: %s\n' "$label"
}

check 'classic short public key' "$scratch/classic" short-pk
check 'classic short signature' "$scratch/classic" short-sn
check 'classic short secret key' "$scratch/classic" short-sk
check 'pkc_skc exact-size secret key' "$scratch/seed" seed-only-sk
