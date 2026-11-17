#!/bin/sh
# hash-25-2: the reference TaiChi-1024 leaks its padded message buffer on every call
#
# Builds each reference instance with malloc and free wrapped at link time,
# calls CryptHash 100 times, and counts allocations still live afterwards.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-25-2: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED hash-25-2: $1"; exit 1; }
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
n=0
check() {   # instance digest-bits expected-live-blocks
    d="hash-25/TaiChi/Implementations/Reference_Implementation/$1"
    [ -d "$d" ] || bad "$d not found"
    gcc -O1 -w -I"$d" -Wl,--wrap=malloc,--wrap=free -o "$scratch/t" "$here/reproduce_leak.c" \
        "$d/CryptHash_AlgorithmInstance.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$1 build failed"; }
    out=$("$scratch/t" "$2") || bad "$1 run failed"
    echo "$1: $out"
    [ "$out" = "live_blocks_after_100_calls=$3" ] || bad "$1 expected $3 live blocks"
    n=$((n + 1))
}
check TaiChi-512 512 0
check TaiChi-768 768 0
check TaiChi-1024 1024 100
echo "ATTACK hash-25-2: the reference TaiChi-1024 leaks its padded message buffer on every call: CONFIRMED"
exit 0
