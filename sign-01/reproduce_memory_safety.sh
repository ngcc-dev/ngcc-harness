#!/usr/bin/env bash
# Local ASan witnesses against the archived Aigis-Sig+-I reference source.
set -euo pipefail

root_dir=$(cd "$(dirname "$0")/.." && pwd)
src_dir="$root_dir/sign-01/Implementations/Implementations/Reference_Implementation/Aigis-Sig+-I"
temp_dir=$(mktemp -d -t ngcc-aigis-memory-XXXXXXXX)
cleanup() {
    rm -f "$temp_dir/extra" "$temp_dir/short" "$temp_dir/asan.log"
    rmdir "$temp_dir"
}
trap cleanup EXIT

sources=(sign polyvec packing poly reduce ntt rounding fips202 hashkdf auxfunc
         SIG_AlgorithmInstance drng pspm rng)
source_paths=()
for source in "${sources[@]}"; do
    source_paths+=("$src_dir/$source.c")
done
flags=(-O2 -g -w -Wno-error=incompatible-pointer-types -fsanitize=address
       -fno-omit-frame-pointer -DPARAMS=1 -DUSE_ICCS -I"$src_dir")

gcc "${flags[@]}" "${source_paths[@]}" "$root_dir/sign-01/reproduce_extra_unpack.c" -o "$temp_dir/extra"
gcc "${flags[@]}" "${source_paths[@]}" "$root_dir/sign-01/reproduce_short_key_read.c" -o "$temp_dir/short"

check() {
    local label=$1 binary=$2 expected=$3
    shift 3
    if ASAN_OPTIONS=detect_leaks=0 "$temp_dir/$binary" "$@" >"$temp_dir/asan.log" 2>&1; then
        echo "ATTACK $label NOT-CONFIRMED (unexpected success)"
        return 1
    fi
    if ! rg -q 'AddressSanitizer: stack-buffer-overflow' "$temp_dir/asan.log" ||
       ! rg -q "$expected" "$temp_dir/asan.log"; then
        echo "ATTACK $label NOT-CONFIRMED (unexpected diagnostic)"
        sed -n '1,35p' "$temp_dir/asan.log"
        return 1
    fi
    echo "ATTACK $label CONFIRMED"
}

check aigis-signing-extra-unpack extra polyvecl_uniform_gamma1
check aigis-short-secret-key short unpack_sk sign
check aigis-short-public-key short unpack_pk verify
